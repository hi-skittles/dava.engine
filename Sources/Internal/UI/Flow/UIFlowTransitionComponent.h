#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
/**
    Component configures possible transitions rules from current state to
    another states by specified events.

    Each rule contains event name, which should be processed by this rule,
    type of processing and processing argument depends by selected type.

    Available types are:
        `Activate` (argument is path to state which should been activate),
        `Deactivate` (argument is path to state with should been deactivate),
        `ActivateBackground` (same as `Activate` but loading step will be
            executed in other thread),
        `DeactivateBackground` (same as `Deactivate` but unloading step will
            be executed in other thread),
        `SendEvent` (argument is new event which should been fire),
        `HistoryBack` (without arguments).
*/
class UIFlowTransitionComponent final : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFlowTransitionComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFlowTransitionComponent);

public:
    /** Describes availabe commands in transitions rules. */
    enum TransitionCommand
    {
        ACTIVATE_STATE = 0, //<! Activate state with specified path.
        ACTIVATE_STATE_BACKGROUND, //<! Activate state with specified path in background.
        DEACTIVATE_STATE, //<! Deactivate state with specified path.
        DEACTIVATE_STATE_BACKGROUND, //<! Deactivate state with specified path in background.
        PRELOAD_STATE, //<! Preload state with special path.
        PRELOAD_STATE_BACKGROUND, //<! Preload state with special path in background.
        SEND_EVENT, //<! Send new specified event.
        HISTORY_BACK //<! Go back in history stack.
    };

    /** Describes transition rule. */
    struct TransitionRule
    {
        /** Checked event name. */
        FastName event;
        /** Transition operation. */
        TransitionCommand operation = ACTIVATE_STATE;
        /** Path to the state for activation/deactivation commands. */
        String statePath;
        /** New event for send event command. */
        FastName sendEvent;
    };

    /** Type of transitions rules storage. */
    using TransitionMap = Vector<TransitionRule>;

    /** Default constructor. */
    UIFlowTransitionComponent();
    /** Copy constructor. */
    UIFlowTransitionComponent(const UIFlowTransitionComponent& src);
    /** Deleted assign operator. */
    UIFlowTransitionComponent& operator=(const UIFlowTransitionComponent&) = delete;

    UIFlowTransitionComponent* Clone() const override;

    /** Return transitions rules storage. */
    const TransitionMap& GetTransitionMap() const;
    /** Setup transitions rules storage. */
    void SetTransitionMap(const TransitionMap& transitions);
    /** Return transitions rules storage as string. */
    String GetTransitionMapAsString() const;
    /** Setup transitions rules storage from string. */
    void SetTransitionMapFromString(const String& transitions);

private:
    ~UIFlowTransitionComponent() override;

    TransitionMap transitionMap;
};

inline const UIFlowTransitionComponent::TransitionMap& UIFlowTransitionComponent::GetTransitionMap() const
{
    return transitionMap;
}
}
