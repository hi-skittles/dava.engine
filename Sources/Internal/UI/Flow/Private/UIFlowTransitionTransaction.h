#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Base/List.h"
#include "Concurrency/Thread.h"
#include "Functional/Function.h"
#include "UI/Flow/Private/UIFlowTransitionEffect.h"

namespace DAVA
{
class Thread;
class Sprite;
class UIFlowStateComponent;
class UIFlowStateSystem;
class UIFlowTransitionEffect;

/**
    This class store information about activating and deactivating states
    and controll process of activation and deactivation.
*/
class UIFlowTransitionTransaction final
{
public:
    /** Constructor with specified states to activation and/or deactivation.
        Any of param can be nullptr. */
    UIFlowTransitionTransaction(UIFlowStateComponent* activateState, UIFlowStateComponent* deactivateState, bool background = false, bool onlyLoad = false);

    /** Get pointer to state that should be activated. */
    UIFlowStateComponent* GetActivateState() const;
    /** Get pointer to state that should be deactivated. */
    UIFlowStateComponent* GetDeactivateState() const;

    void SetEffectConfig(const UIFlowTransitionEffectConfig& config);
    UIFlowTransitionEffect* GetEffect() const;

    /** Return flag for storing this transaction in history stack. */
    bool IsSkipHistory() const;
    /** Setup flag for storing this transaction in history stack. */
    void SetSkipHistory(bool value);

    /** Return true if loading and unloading steps in this transaction will be
        run in background. */
    bool IsBackgroundLoading() const;
    /** Return flag that this transaction only loads state. */
    bool IsOnlyLoad() const;
    /** Return true if this transaction hasn't target states. */
    bool IsEmpty() const;
    /** Return true if this transaction just created and not applied yet. */
    bool IsNew() const;
    /** Return true if transaction was done. */
    bool IsFinished() const;
    /** Return true if transaction is animating now. */
    bool IsAnimating() const;

    /** Build activating and deactivating queues across targat stataes. */
    void BuildTransaction(UIFlowStateSystem* system);
    /** Swap target activating and deactivating states. */
    void Reverse();
    /** Clean activating and deactivating queues. */
    void Clean();

    /** Apply this transaction to Flow states system. */
    void Apply(UIFlowStateSystem* system);

    /** Change target activating state and part of activating queue to new
        target state. */
    void ChangeActivationBranch(const UIFlowTransitionTransaction& patch);

private:
    enum class State : int32
    {
        Begin = 0,
        Build,
        PrevScreenshot,
        Deactivate,
        Unload,
        Release,
        Init,
        Load,
        NextScreenshot,
        Animate,
        Activate,
        Finish
    };
    State state = State::Begin;

    bool skipHistory = false;
    bool onlyLoad = false;
    UIFlowStateComponent* activateState = nullptr;
    UIFlowStateComponent* deactivateState = nullptr;
    List<UIFlowStateComponent*> activateQueue;
    List<UIFlowStateComponent*> deactivateQueue;
    List<UIFlowStateComponent*>::iterator activateIt;
    List<UIFlowStateComponent*>::reverse_iterator deactivateIt;

    bool isBackgroundLoading = false;
    RefPtr<Thread> thread;

    std::unique_ptr<UIFlowTransitionEffect> effect;

    /** Makes applying step in main thread. */
    void ApplyStepForeground(UIFlowStateSystem* system, bool autoNext = false);
    /** Makes applying step in background thread. */
    void ApplyStepBackground(UIFlowStateSystem* system, bool autoNext = false);
    /** Creates and starts background loading thread. */
    RefPtr<Thread> StartThread(const Function<void()>& fn);

    void PreparePrevScreenshot(UIFlowStateSystem* system);
    void PrepareNextScreenshot(UIFlowStateSystem* system);
    void FinishAnimation(UIFlowStateSystem* system);

    friend class UIFlowTransitionAnimationSystem;
};
}
