#include "Input/InputBindingListener.h"

#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Debug/DVAssert.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
void InputBindingListener::Listen(eInputBindingListenerModes mode, Function<void(bool, const Vector<InputEvent>&)> callback)
{
    Listen(mode, callback, 0, eInputDeviceTypes::CLASS_ALL);
}

void InputBindingListener::Listen(eInputBindingListenerModes mode, Function<void(bool, const Vector<InputEvent>&)> callback, uint32 deviceId)
{
    Listen(mode, callback, deviceId, eInputDeviceTypes::CLASS_ALL);
}

void InputBindingListener::Listen(eInputBindingListenerModes mode, Function<void(bool, const Vector<InputEvent>&)> callback, eInputDeviceTypes deviceTypesMask)
{
    Listen(mode, callback, 0, deviceTypesMask);
}

void InputBindingListener::Listen(eInputBindingListenerModes mode, Function<void(bool, const Vector<InputEvent>&)> callback, uint32 deviceId, eInputDeviceTypes deviceTypesMask)
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(callback != nullptr);

    StopListening();

    currentMode = mode;
    currentCallback = callback;
    currentDeviceId = deviceId;
    currentDeviceTypesMask = deviceTypesMask;

    inputHandlerToken = GetEngineContext()->inputSystem->AddHandler(deviceTypesMask, MakeFunction(this, &InputBindingListener::OnInputEvent));
}

bool InputBindingListener::OnInputEvent(const InputEvent& e)
{
    // If we're restricted to the specific device,
    // and this event is from different one - ignore it
    if (currentDeviceId > 0 && e.device->GetId() != currentDeviceId)
    {
        return false;
    }

    // Ignore keyboard char events
    if (e.deviceType == eInputDevices::KEYBOARD && e.keyboardEvent.charCode > 0)
    {
        return false;
    }

    bool finishedListening = false;
    bool cancelled = false;
    bool addEventToResult = false;

    const InputElementInfo elementInfo = GetInputElementInfo(e.elementId);
    if (elementInfo.type == eInputElementTypes::DIGITAL)
    {
        // TODO: handle cancelling on gamepads
        if (e.elementId == eInputElements::KB_ESCAPE)
        {
            finishedListening = true;
            cancelled = true;
            addEventToResult = false;
        }
        else
        {
            const bool isSystemKey = IsKeyboardSystemInputElement(e.elementId);

            // Ignore system keys
            if (!isSystemKey)
            {
                const bool isModifierKey = IsKeyboardModifierInputElement(e.elementId);

                // If a button has been pressed
                if (e.digitalState.IsJustPressed())
                {
                    if (currentMode == eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS)
                    {
                        // Ignore modifiers
                        if (!isModifierKey)
                        {
                            addEventToResult = true;
                            finishedListening = true;
                        }
                    }
                    else if (currentMode == eInputBindingListenerModes::DIGITAL_SINGLE_WITH_MODIFIERS)
                    {
                        addEventToResult = true;

                        // Stop listening when first non-modifier key is pressed
                        if (!isModifierKey)
                        {
                            finishedListening = true;
                        }
                    }
                    else if (currentMode == eInputBindingListenerModes::DIGITAL_MULTIPLE_ANY)
                    {
                        addEventToResult = true;
                    }
                }
                else if (e.digitalState.IsJustReleased())
                {
                    finishedListening = true;
                }
            }
        }
    }
    else
    {
        if (currentMode == eInputBindingListenerModes::ANALOG)
        {
            addEventToResult = true;
            finishedListening = true;
        }
    }

    bool handled = false;

    // Add to result list if needed
    if (addEventToResult)
    {
        handled = true;
        result.push_back(e);
    }

    // If we're finished listening, invoke callback and reset state
    if (finishedListening)
    {
        currentCallback(cancelled, result);
        StopListening();
    }

    return handled;
}

bool InputBindingListener::IsListening() const
{
    DVASSERT(Thread::IsMainThread());

    return inputHandlerToken > 0;
}

void InputBindingListener::StopListening()
{
    DVASSERT(Thread::IsMainThread());

    if (IsListening())
    {
        GetEngineContext()->inputSystem->RemoveHandler(inputHandlerToken);
        inputHandlerToken = 0;

        result.clear();
    }
}

} // namespace DAVA
