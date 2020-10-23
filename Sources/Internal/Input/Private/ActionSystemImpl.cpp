#include "Input/Private/ActionSystemImpl.h"
#include "Input/InputSystem.h"
#include "Engine/Engine.h"
#include "DeviceManager/DeviceManager.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
namespace Private
{
// Calculates number of states in array which are not empty
// Used for sorting
static size_t GetNonEmptyStatesCount(const Array<eInputElements, ActionSystem::MAX_DIGITAL_STATES_COUNT>& array)
{
    size_t result = 0;
    for (const eInputElements elementId : array)
    {
        if (elementId != eInputElements::NONE)
        {
            ++result;
        }
    }

    return result;
}

bool DigitalBindingCompare::operator()(const DigitalBinding& first, const DigitalBinding& second) const
{
    return GetNonEmptyStatesCount(first.digitalElements) > GetNonEmptyStatesCount(second.digitalElements);
}

bool AnalogBindingCompare::operator()(const AnalogBinding& first, const AnalogBinding& second) const
{
    return GetNonEmptyStatesCount(first.digitalElements) > GetNonEmptyStatesCount(second.digitalElements);
}

ActionSystemImpl::ActionSystemImpl(ActionSystem* actionSystem)
    : actionSystem(actionSystem)
{
    Engine::Instance()->update.Connect(this, &ActionSystemImpl::OnUpdate);
}

ActionSystemImpl::~ActionSystemImpl()
{
    Engine::Instance()->update.Disconnect(this);
}

void ActionSystemImpl::BindSet(const ActionSet& set, Vector<uint32> devices)
{
    DVASSERT(Thread::IsMainThread());

    // Check if there are sets which are already bound to any of these devices
    // Unbind them from these devices if there are
    if (devices.size() > 0)
    {
        auto iter = boundSets.begin();
        while (iter != boundSets.end())
        {
            BoundActionSet& boundSet = *iter;

            // If set is bound to specific devices
            if (boundSet.devices.size() > 0)
            {
                // Unbind it
                for (const uint32 deviceId : devices)
                {
                    boundSet.devices.erase(std::remove(boundSet.devices.begin(), boundSet.devices.end(), deviceId), boundSet.devices.end());
                }

                // If it is not bound to any devices anymore - remove it from the list
                if (boundSet.devices.size() == 0)
                {
                    iter = boundSets.erase(iter);
                    continue;
                }
            }

            ++iter;
        }
    }

    for (const auto& digitalBinding : set.digitalBindings)
    {
        bool noneElementProcessed = false;
        for (size_t i = 0; i < digitalBinding.digitalElements.size(); ++i)
        {
            if (digitalBinding.digitalElements[i] != eInputElements::NONE)
            {
                DVASSERT(!noneElementProcessed, "eInputElements::NONE in the middle of the digital elements array.");
                DVASSERT(digitalBinding.digitalStates[i] != DigitalElementState::Released(), "Do you really want to bind an action on key release? Your desires are ... unconventional.");
            }
            else
            {
                noneElementProcessed = true;
            }
        }

        DVASSERT(digitalBinding.digitalElements[0] != eInputElements::NONE, "Array of digital elements can't be empty for digital binding.");

        InternalDigitalActionState internalDigitalActionState;

        internalDigitalActionState.action.actionId = digitalBinding.actionId;
        internalDigitalActionState.action.analogState = digitalBinding.outputAnalogState;

        digitalActionsStates[digitalBinding.actionId] = internalDigitalActionState;
    }

    for (const auto& analogBinding : set.analogBindings)
    {
        bool noneElementProcessed = false;
        for (size_t i = 0; i < analogBinding.digitalElements.size(); ++i)
        {
            if (analogBinding.digitalElements[i] != eInputElements::NONE)
            {
                DVASSERT(!noneElementProcessed, "eInputElements::NONE in the middle of the digital elements array.");
                DVASSERT(analogBinding.digitalStates[i] != DigitalElementState::Released(), "Do you really want to bind an action on key release? Your desires are ... unconventional.");
            }
            else
            {
                noneElementProcessed = true;
            }
        }

        DVASSERT(analogBinding.analogElementId != eInputElements::NONE);

        InternalAnalogActionState internalAnalogActionState;

        internalAnalogActionState.actionId = analogBinding.actionId;
        internalAnalogActionState.stateType = analogBinding.analogStateType;

        InputDevice* device = GetDeviceForAnalogElement(analogBinding.analogElementId, devices);

        DVASSERT(device != nullptr);

        AnalogElementState initialState = device->GetAnalogElementState(analogBinding.analogElementId);
        internalAnalogActionState.previousElementState = initialState;
        internalAnalogActionState.currentElementState = initialState;

        analogActionsStates[analogBinding.actionId] = internalAnalogActionState;
    }

    BoundActionSet boundSet;
    boundSet.name = set.name;
    boundSet.digitalBindings.insert(set.digitalBindings.begin(), set.digitalBindings.end());
    boundSet.analogBindings.insert(set.analogBindings.begin(), set.analogBindings.end());
    boundSet.devices = devices;

    boundSets.emplace_back(std::move(boundSet));
}

void ActionSystemImpl::UnbindAllSets()
{
    DVASSERT(Thread::IsMainThread());

    boundSets.clear();
    analogActionsStates.clear();
    digitalActionsStates.clear();
}

bool ActionSystemImpl::GetDigitalActionState(FastName actionId) const
{
    DVASSERT(Thread::IsMainThread());

    auto digitalActionStateIter = digitalActionsStates.find(actionId);

    DVASSERT(digitalActionStateIter != digitalActionsStates.end());

    const InternalDigitalActionState& digitalActionState = digitalActionStateIter->second;
    return digitalActionState.active;
}

AnalogActionState ActionSystemImpl::GetAnalogActionState(FastName actionId) const
{
    DVASSERT(Thread::IsMainThread());

    auto analogActionStateIter = analogActionsStates.find(actionId);

    DVASSERT(analogActionStateIter != analogActionsStates.end());

    const InternalAnalogActionState& analogActionState = analogActionStateIter->second;

    AnalogElementState analogElementState = analogActionState.currentElementState;

    if (analogActionState.stateType == AnalogBinding::eAnalogStateType::RELATIVE_STATE)
    {
        analogElementState.x -= analogActionState.previousElementState.x;
        analogElementState.y -= analogActionState.previousElementState.y;
        analogElementState.z -= analogActionState.previousElementState.z;
    }

    return AnalogActionState(analogActionState.active, analogElementState);
}

// Helper function to check if specified states are active
bool ActionSystemImpl::CheckDigitalStates(const Array<eInputElements, ActionSystem::MAX_DIGITAL_STATES_COUNT>& elements, const Array<DigitalElementState, ActionSystem::MAX_DIGITAL_STATES_COUNT>& states, const Vector<uint32>& devices)
{
    DeviceManager* deviceManager = GetEngineContext()->deviceManager;

    for (size_t i = 0; i < ActionSystem::MAX_DIGITAL_STATES_COUNT; ++i)
    {
        eInputElements elementId = elements[i];

        // If it's an empty state, break
        if (elementId == eInputElements::NONE)
        {
            break;
        }

        const DigitalElementState requiredState = states[i];
        bool requiredStateMatches = false;

        for (const uint32 deviceId : devices)
        {
            InputDevice* device = deviceManager->GetInputDevice(deviceId);
            if (device != nullptr)
            {
                if (device->IsElementSupported(elementId))
                {
                    const DigitalElementState state = device->GetDigitalElementState(elementId);
                    if (CompareDigitalStates(requiredState, state))
                    {
                        requiredStateMatches = true;
                        break;
                    }
                }
            }
        }

        if (!requiredStateMatches)
        {
            // At least one control is not in the state which is required, stop
            return false;
        }
    }

    return true;
}

bool ActionSystemImpl::CompareDigitalStates(const DigitalElementState& requiredState, const DigitalElementState& state)
{
    // If an action is bound to JustPressed or JustReleased - they should match exactly for an action to be triggered only once
    // Otherwise just check for 'pressed' flag

    if (requiredState.IsJustPressed())
    {
        return state.IsJustPressed();
    }
    else if (requiredState.IsJustReleased())
    {
        return state.IsJustReleased();
    }
    else
    {
        return requiredState.IsPressed() == state.IsPressed();
    }
}

InputDevice* ActionSystemImpl::GetDeviceForAnalogElement(eInputElements analogElementId, const Vector<uint32>& devices)
{
    DeviceManager* deviceManager = GetEngineContext()->deviceManager;

    for (const uint32 deviceId : devices)
    {
        InputDevice* device = deviceManager->GetInputDevice(deviceId);
        if (device != nullptr)
        {
            if (device->IsElementSupported(analogElementId))
            {
                return device;
            }
        }
    }

    return nullptr;
}

void ActionSystemImpl::OnUpdate(float32 elapsedTime)
{
    // TODO: handle multi window case (check pinning on focused window)
    Window* primaryWindow = Engine::Instance()->PrimaryWindow();
    bool isPinningEnabled = primaryWindow->GetCursorCapture() == eCursorCapture::PINNING;

    for (const BoundActionSet& setBinding : boundSets)
    {
        for (const DigitalBinding& digitalBinding : setBinding.digitalBindings)
        {
            bool skipBinding = false;

            for (const eInputElements element : digitalBinding.digitalElements)
            {
                if (elementsUsedInDigitalBindings.find(element) != elementsUsedInDigitalBindings.end())
                {
                    skipBinding = true;
                    break;
                }
            }

            auto digitalActionStateIter = digitalActionsStates.find(digitalBinding.actionId);

            DVASSERT(digitalActionStateIter != digitalActionsStates.end());

            InternalDigitalActionState& digitalActionState = digitalActionStateIter->second;
            digitalActionState.active = false;

            if (skipBinding)
            {
                continue;
            }

            bool active = CheckDigitalStates(digitalBinding.digitalElements, digitalBinding.digitalStates, setBinding.devices);

            if (active)
            {
                digitalActionState.active = true;

                for (const eInputElements element : digitalBinding.digitalElements)
                {
                    if (element == eInputElements::NONE)
                    {
                        break;
                    }
                    elementsUsedInDigitalBindings.insert(element);
                }
            }
        }

        for (const AnalogBinding& analogBinding : setBinding.analogBindings)
        {
            auto analogActionStateIter = analogActionsStates.find(analogBinding.actionId);

            DVASSERT(analogActionStateIter != analogActionsStates.end());

            InternalAnalogActionState& analogActionState = analogActionStateIter->second;
            analogActionState.active = false;

            InputDevice* device = GetDeviceForAnalogElement(analogBinding.analogElementId, setBinding.devices);

            DVASSERT(device != nullptr);

            analogActionState.previousElementState = analogActionState.currentElementState;
            analogActionState.currentElementState = device->GetAnalogElementState(analogBinding.analogElementId);

            // Handle pinning
            if (analogBinding.analogElementId == eInputElements::MOUSE_POSITION)
            {
                if (isPinningEnabled)
                {
                    analogActionState.previousElementState = { 0.f, 0.f, 0.f };

                    if (analogActionState.stateType == AnalogBinding::eAnalogStateType::ABSOLUTE_STATE)
                    {
                        analogActionState.currentElementState = { 0.f, 0.f, 0.f };
                    }
                }
            }

            bool skipBinding = false;

            // Skip binding without digital elements if there is active binding with same analogElementId
            if (analogBinding.digitalElements[0] == eInputElements::NONE)
            {
                if (elementsUsedInAnalogBindings.find(analogBinding.analogElementId) != elementsUsedInAnalogBindings.end())
                {
                    continue;
                }
            }

            for (const eInputElements element : analogBinding.digitalElements)
            {
                if (elementsUsedInAnalogBindings.find(element) != elementsUsedInAnalogBindings.end())
                {
                    skipBinding = true;
                    break;
                }
            }

            if (skipBinding)
            {
                continue;
            }

            bool active = CheckDigitalStates(analogBinding.digitalElements, analogBinding.digitalStates, setBinding.devices);

            if (active)
            {
                analogActionState.active = true;

                for (const eInputElements element : analogBinding.digitalElements)
                {
                    if (element == eInputElements::NONE)
                    {
                        break;
                    }
                    elementsUsedInAnalogBindings.insert(element);
                }

                elementsUsedInAnalogBindings.insert(analogBinding.analogElementId);
            }
        }
    }

    elementsUsedInDigitalBindings.clear();
    elementsUsedInAnalogBindings.clear();

    // We need to process all active actions after checking states to allow calls to getters
    // (GetDigitalActionState, GetAnalogActionState) from 'ActionTriggered' signal handlers

    // Trigger all active digital actions
    for (const auto& digitalStatesMapPair : digitalActionsStates)
    {
        const InternalDigitalActionState& digitalActionState = digitalStatesMapPair.second;
        if (digitalActionState.active)
        {
            actionSystem->ActionTriggered.Emit(digitalActionState.action);
        }
    }

    // Trigger all active and changed in the current frame analog actions
    for (auto& analogStatesMapPair : analogActionsStates)
    {
        InternalAnalogActionState& analogActionState = analogStatesMapPair.second;
        if (analogActionState.active)
        {
            if (analogActionState.currentElementState.x == analogActionState.previousElementState.x &&
                analogActionState.currentElementState.y == analogActionState.previousElementState.y &&
                analogActionState.currentElementState.z == analogActionState.previousElementState.z)
            {
                continue;
            }

            Action action;
            action.actionId = analogActionState.actionId;
            action.analogState = analogActionState.currentElementState;

            if (analogActionState.stateType == AnalogBinding::eAnalogStateType::RELATIVE_STATE)
            {
                action.analogState.x -= analogActionState.previousElementState.x;
                action.analogState.y -= analogActionState.previousElementState.y;
                action.analogState.z -= analogActionState.previousElementState.z;
            }

            actionSystem->ActionTriggered.Emit(action);
        }
    }
}

} // namespace Private
} // namespace DAVA
