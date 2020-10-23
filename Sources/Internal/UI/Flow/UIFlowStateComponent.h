#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"
#include "UI/Flow/UIFlowTransitionEffectConfig.h"

namespace DAVA
{
/**
    Marks that control is UI Flow state.

    To setup this component (and state) user should select type of state
    and type of history behaviors (see `State types rules`).
    Also user can setup with services will be created with activation on this
    state, and which events will be sent to systems if it state has been
    activated or deactivated. To activate service you should write service
    alias (for searching its service in controllers) and native reflection
    permanent name of cpp type. After activation state specified service will
    be available by `context.getService('alias')`.
*/
class UIFlowStateComponent final : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFlowStateComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFlowStateComponent);

public:
    /** Types of Flow states. */
    enum StateType
    {
        STATE_GROUP = 0, //<! Marks what state is a group.
        STATE_SINGLE, //<! Marks what state is a single state.
        STATE_MULTIPLE //<! Marks what state is a multiple state.
    };

    /** Types of history behaviors. */
    enum HistoryType
    {
        HISTORY_NONE = 0, //<! Disable history for this state.
        HISTORY_MULTIPLE, //<! Multiple instances of this state in history stack.
        HISTORY_SINGLE, //<! Single instance of this state in history stack.
        HISTORY_SINGLE_SIBLING, //<! Single instans of this state ot all his siblings in history stack.
        HISTORY_ONLY_TOP //<! Single instance of this state only on top of history stack.
    };

    /** Describes link between alias and reflected name of inheritor of UIFlowService. */
    struct ServiceDescriptor
    {
        String name; //<! Alias name.
        String nativeType; //<! Reflected type name.
    };

    /** Default constructor. */
    UIFlowStateComponent();
    /** Copy constructor. */
    UIFlowStateComponent(const UIFlowStateComponent& src);
    /** Deleted assign operator. */
    UIFlowStateComponent& operator=(const UIFlowStateComponent&) = delete;

    UIFlowStateComponent* Clone() const override;

    /** Return type of the state. */
    StateType GetStateType() const;
    /** Setup type of the state. */
    void SetStateType(StateType type);
    /** Return type of history behavior. */
    HistoryType GetHistoryType() const;
    /** Setup type of history behavior. */
    void SetHistoryType(HistoryType type);
    /** Return list of UI flow services descriptors. */
    const Vector<ServiceDescriptor>& GetServices() const;
    /** Setup list of UI flow services descriptors. */
    void SetServices(const Vector<ServiceDescriptor>& services);
    /** Return list of Events after state's activation. */
    const Vector<FastName>& GetActivateEvents() const;
    /** Setup list of Events after state's activation. */
    void SetActivateEvents(const Vector<FastName>& events);
    /** Return list of Events after state's deactivation. */
    const Vector<FastName>& GetDeactivateEvents() const;
    /** Setup list of Events after state's deactivation. */
    void SetDeactivateEvents(const Vector<FastName>& events);

    void SetEffectConfig(const UIFlowTransitionEffectConfig& config);
    const UIFlowTransitionEffectConfig& GetEffectConfig() const;

    void SetEffectIn(UIFlowTransitionEffectConfig::Effect effect);
    UIFlowTransitionEffectConfig::Effect GetEffectIn() const;
    void SetEffectOut(UIFlowTransitionEffectConfig::Effect effect);
    UIFlowTransitionEffectConfig::Effect GetEffectOut() const;
    void SetEffectDuration(float32 duration);
    float32 GetEffectDuration() const;

    String GetServicesAsString() const;
    void SetServicesFromString(const String& servicesString);
    String GetActivateEventsAsString() const;
    void SetActivateEventsFromString(const String& eventsString);
    String GetDeactivateEventsAsString() const;
    void SetDeactivateEventsFromString(const String& eventsString);

private:
    ~UIFlowStateComponent() override;

    StateType stateType = STATE_GROUP;
    HistoryType historyType = HISTORY_NONE;
    Vector<ServiceDescriptor> services;
    Vector<FastName> activateEvents;
    Vector<FastName> deactivateEvents;
    UIFlowTransitionEffectConfig effectConfig;
};

inline UIFlowStateComponent::StateType UIFlowStateComponent::GetStateType() const
{
    return stateType;
}

inline UIFlowStateComponent::HistoryType UIFlowStateComponent::GetHistoryType() const
{
    return historyType;
}

inline const Vector<UIFlowStateComponent::ServiceDescriptor>& UIFlowStateComponent::GetServices() const
{
    return services;
}

inline const Vector<FastName>& UIFlowStateComponent::GetActivateEvents() const
{
    return activateEvents;
}

inline const Vector<FastName>& UIFlowStateComponent::GetDeactivateEvents() const
{
    return deactivateEvents;
}

inline const UIFlowTransitionEffectConfig& UIFlowStateComponent::GetEffectConfig() const
{
    return effectConfig;
}
}
