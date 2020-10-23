#pragma once

#include "Base/FastName.h"
#include "Base/UnordererMap.h"
#include "Base/List.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIFlowContext;
class UIFlowStateComponent;
class UIFlowTransitionTransaction;
class UIFlowViewSystem;

/**
    Manage all UIFlowStateComponents and control Flow active states and history stack.
*/
class UIFlowStateSystem final : public UISystem
{
public:
    /** Default constructor. */
    UIFlowStateSystem();
    /** Default destructor. */
    ~UIFlowStateSystem() override;

    /** Return pointer to UIFlowStateComponent with specified path.
        Support relative path from current active Single state and from Flow
        root state. */
    UIFlowStateComponent* FindStateByPath(const String& statePath) const;

    /** Return pointer to UIControl with root state. */
    UIControl* GetFlowRoot() const;
    /** Setup UIControl with root state. */
    void SetFlowRoot(UIControl* flowRoot);
    /** Return pointer to UIFlowContext instance. */
    UIFlowContext* GetContext() const;
    /** Return pointer to top transition transaction. */
    UIFlowTransitionTransaction* GetTopTransitionTransaction() const;

    /** Put in transitions queue next specified state for activation with
        specified flag of background loading resources. */
    void ActivateState(UIFlowStateComponent* state, bool background);
    /** Put in transitions queue next specified state for deactivation with
        specified flag of background unloading resources. */
    void DeactivateState(UIFlowStateComponent* state, bool background);
    /** Put in transitions queue next specified state for preload with
        specified flag of background loading resources. */
    void PreloadState(UIFlowStateComponent* state, bool background);

    /** Put in transitions queue root state for deactivateion.
        Work only in main thread. */
    void DeactivateAllStates();
    /** Immediately deactivate all states, clean history and root state. */
    void HardReset();
    /** Put in transitions queue reverted last transition from history stack. */
    void HistoryBack();

    /** Return current active Single state. */
    UIFlowStateComponent* GetCurrentSingleState() const;
    /** Return list of current active Multiple states. */
    const Vector<UIFlowStateComponent*>& GetCurrentMultipleStates() const;
    /** Check that specified state is initted. */
    bool IsStateInitted(UIFlowStateComponent* state) const;
    /** Check that specified state is loaded. */
    bool IsStateLoaded(UIFlowStateComponent* state) const;
    /** Check that specified state is activated. */
    bool IsStateActive(UIFlowStateComponent* state) const;
    /** Check that any transitions exist in transitions queue. */
    bool HasTransitions() const;

    /** Process top transition from queue. */
    void ApplyTransition();

    /** Process transitions rules with specified event. */
    bool ProcessEvent(const FastName& eventName);

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

private:
    /** Describe link between registered UIFlowStateComponent and it activation flag. */
    struct StateLink
    {
        enum Status
        {
            Off = 0,
            Initted,
            Loaded,
            Activated
        };

        StateLink() = default;
        StateLink(UIFlowStateComponent* state)
            : state(state)
        {
        }

        UIFlowStateComponent* state = nullptr;
        Status status = Status::Off;
    };

    std::unique_ptr<UIFlowContext> context;
    UnorderedMap<UIFlowStateComponent*, StateLink> links;
    UIFlowStateComponent* currentState = nullptr;
    Vector<UIFlowStateComponent*> currentMultipleStates;
    List<std::unique_ptr<UIFlowTransitionTransaction>> transitionTransactions;
    List<std::unique_ptr<UIFlowTransitionTransaction>> historyTransactions;

    /** Add new link with specified UIFlowStateComponent to system. */
    void AddStateLink(UIFlowStateComponent* component);
    /** Remove link with specified UIFlowStateComponent to system. */
    void RemoveStateLink(UIFlowStateComponent* component);

    /** First step of state activation.
        Includes context setup, controller initialization. */
    void StartActivation(UIFlowStateComponent* state);
    /** Second step of state activation.
        Includes loading view and loading resources from controller.
        Can be executed not in main thread. */
    void ProcessActivation(UIFlowStateComponent* state);
    /** Final step of state activation.
        Includes activation view (append it to specified container in UI hierarchy)
        and activate controller. */
    void FinishActivation(UIFlowStateComponent* state);

    /** First step of state deactivation.
        Includes controller deactivation and removing state view from container. */
    void StartDeactivation(UIFlowStateComponent* state);
    /** Second step of state deactivation.
        Includes unloading resources from controller and releasing view.
        Can be executed not in main thread. */
    void ProcessDeactivation(UIFlowStateComponent* state);
    /** Final step of state deactivation.
        Includes controller releasing and deactivation of UIFlowServices from context. */
    void FinishDeactivation(UIFlowStateComponent* state);

    /** Hide state's view root control. */
    void HideStateView(UIFlowStateComponent* state);

    /** Show state's view root control. */
    void ShowStateView(UIFlowStateComponent* state);

    /** Finish current transition and pop it from transitions queue. */
    void FinishTransaction(const UIFlowTransitionTransaction* transaction);

    /** Put transition to queue. */
    void PutTransitionTransaction(std::unique_ptr<UIFlowTransitionTransaction> transaction);
    /** Put transition to history stack. */
    void PutHistoryTransaction(std::unique_ptr<UIFlowTransitionTransaction> transaction);

    friend class UIFlowTransitionTransaction;
};

inline UIFlowStateComponent* UIFlowStateSystem::GetCurrentSingleState() const
{
    return currentState;
}

inline const Vector<UIFlowStateComponent*>& UIFlowStateSystem::GetCurrentMultipleStates() const
{
    return currentMultipleStates;
}
}