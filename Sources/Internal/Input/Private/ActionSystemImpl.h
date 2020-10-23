#pragma once

#include "Input/ActionSystem.h"
#include "Input/InputEvent.h"

namespace DAVA
{
namespace Private
{
// Comparators for BoundActionSet's multisets
// Since we need to keep them in order

struct DigitalBindingCompare final
{
    bool operator()(const DigitalBinding& first, const DigitalBinding& second) const;
};

struct AnalogBindingCompare final
{
    bool operator()(const AnalogBinding& first, const AnalogBinding& second) const;
};

// ActionSet + devices it's bound to
// Use multiset since we need to process bindings with bigger number of buttons first
// (CTRL + SPACE should be checked before SPACE, otherwise we might end up with SPACE action being triggered even though CTRL is also pressed)
struct BoundActionSet final
{
    String name;
    Vector<uint32> devices;
    std::multiset<DigitalBinding, DigitalBindingCompare> digitalBindings;
    std::multiset<AnalogBinding, AnalogBindingCompare> analogBindings;
};

struct InternalDigitalActionState
{
    // 'active' is true if digital elements for digital action are in required state
    bool active = false;
    // 'action' is needed to store and emit signals in 'OnUpdate' method
    Action action;
};

struct InternalAnalogActionState
{
    // 'active' is true if digital elements for analog action are in required state
    bool active = false;

    AnalogBinding::eAnalogStateType stateType = AnalogBinding::eAnalogStateType::ABSOLUTE_STATE;

    FastName actionId;
    AnalogElementState currentElementState;
    AnalogElementState previousElementState;
};

class ActionSystemImpl final
{
public:
    ActionSystemImpl(ActionSystem* actionSystem);
    ~ActionSystemImpl();

    void BindSet(const ActionSet& set, Vector<uint32> devices);
    void UnbindAllSets();

    bool GetDigitalActionState(FastName actionId) const;
    AnalogActionState GetAnalogActionState(FastName actionId) const;

private:
    void OnUpdate(float32 elapsedTime);
    bool CheckDigitalStates(const Array<eInputElements, ActionSystem::MAX_DIGITAL_STATES_COUNT>& digitalElements, const Array<DigitalElementState, ActionSystem::MAX_DIGITAL_STATES_COUNT>& digitalStates, const Vector<uint32>& devices);
    bool CompareDigitalStates(const DigitalElementState& requiredState, const DigitalElementState& state);
    InputDevice* GetDeviceForAnalogElement(eInputElements analogElementId, const Vector<uint32>& devices);

private:
    ActionSystem* actionSystem;
    Vector<BoundActionSet> boundSets;
    UnorderedMap<FastName, InternalDigitalActionState> digitalActionsStates;
    UnorderedMap<FastName, InternalAnalogActionState> analogActionsStates;

    // Sets to skip bindings with same elements.
    // For example, we have two bindings:
    // - Shift + space (binding1)
    // - Space (binding2)
    // Binding1 will be processed first and if triggered, binding2 should be skipped
    Set<eInputElements> elementsUsedInDigitalBindings;
    Set<eInputElements> elementsUsedInAnalogBindings;
};

} // namespace Private
} // namespace DAVA
