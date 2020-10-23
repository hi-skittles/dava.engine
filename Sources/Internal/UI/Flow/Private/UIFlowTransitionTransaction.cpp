#include "UI/Flow/Private/UIFlowTransitionTransaction.h"
#include "Concurrency/Thread.h"
#include "Engine/EngineContext.h"
#include "Job/JobManager.h"
#include "Render/2D/Sprite.h"
#include "UI/Flow/Private/UIFlowTransitionEffect.h"
#include "UI/Flow/Private/UIFlowUtils.h"
#include "UI/Flow/UIFlowStateComponent.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/Flow/UIFlowViewSystem.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenshoter.h"

namespace DAVA
{
namespace UIFlowTransitionTransactionDetails
{
static const uint32 LOADING_THREAD_STACK_SIZE = 1024 * 1024;
}

UIFlowTransitionTransaction::UIFlowTransitionTransaction(UIFlowStateComponent* activateState, UIFlowStateComponent* deactivateState, bool background, bool onlyLoad)
    : skipHistory(onlyLoad) // Only-load transactions doesn't append to history
    , onlyLoad(onlyLoad)
    , activateState(activateState)
    , deactivateState(deactivateState)
    , isBackgroundLoading(background)
{
}

UIFlowStateComponent* UIFlowTransitionTransaction::GetActivateState() const
{
    return activateState;
}

UIFlowStateComponent* UIFlowTransitionTransaction::GetDeactivateState() const
{
    return deactivateState;
}

void UIFlowTransitionTransaction::SetEffectConfig(const UIFlowTransitionEffectConfig& config)
{
    effect = std::make_unique<UIFlowTransitionEffect>(config);
}

UIFlowTransitionEffect* UIFlowTransitionTransaction::GetEffect() const
{
    return effect.get();
}

bool UIFlowTransitionTransaction::IsSkipHistory() const
{
    return skipHistory;
}

bool UIFlowTransitionTransaction::IsBackgroundLoading() const
{
    return isBackgroundLoading;
}

bool UIFlowTransitionTransaction::IsEmpty() const
{
    return activateState == nullptr && deactivateState == nullptr;
}

bool UIFlowTransitionTransaction::IsNew() const
{
    return state == State::Begin;
}

void UIFlowTransitionTransaction::SetSkipHistory(bool value)
{
    skipHistory = value;
}

bool UIFlowTransitionTransaction::IsOnlyLoad() const
{
    return onlyLoad;
}

void UIFlowTransitionTransaction::Clean()
{
    state = State::Begin;
    effect = nullptr;
    activateQueue.clear();
    deactivateQueue.clear();
    activateIt = activateQueue.begin();
    deactivateIt = deactivateQueue.rbegin();
    thread = nullptr;
}

bool UIFlowTransitionTransaction::IsFinished() const
{
    return state == State::Finish;
}

bool UIFlowTransitionTransaction::IsAnimating() const
{
    return state == State::Animate;
}

void UIFlowTransitionTransaction::BuildTransaction(UIFlowStateSystem* system)
{
    activateQueue.clear();
    deactivateQueue.clear();

    if (!onlyLoad && activateState != nullptr && activateState->GetStateType() == UIFlowStateComponent::STATE_GROUP)
    {
        // Search near child State
        activateState = UIFlowUtils::FindNearChildSingleState(activateState);
        DVASSERT(activateState);
    }

    if (!system->IsStateInitted(deactivateState))
    {
        deactivateState = nullptr;
    }

    if (onlyLoad ? system->IsStateLoaded(activateState) : system->IsStateActive(activateState))
    {
        activateState = nullptr;
    }

    if (deactivateState != nullptr)
    {
        deactivateQueue.push_back(deactivateState);
        UIFlowUtils::BuildParentsQueue(deactivateQueue); // Collect parents
        UIFlowUtils::RemoveSharingParentsFromQueue(system, deactivateQueue); // Remove sharing parents
        UIFlowUtils::BuildActivatedQueue(system, deactivateQueue); // Collect children
    }

    if (activateState != nullptr)
    {
        switch (activateState->GetStateType())
        {
        case UIFlowStateComponent::STATE_MULTIPLE:
        {
            activateQueue.push_back(activateState);
            UIFlowUtils::BuildParentsQueue(activateQueue); // Collect parents
            UIFlowUtils::RemoveActiveParentsFromQueue(system, activateQueue); // Remove already activated parents
            //TODO: remove only loaded states for only-load mode
            break;
        }
        case UIFlowStateComponent::STATE_SINGLE:
        {
            // Deactivation (not for only-load)
            if (!onlyLoad && system->GetCurrentSingleState())
            {
                deactivateState = system->GetCurrentSingleState(); // Store deactivate state for reverse transaction
                deactivateQueue.push_back(deactivateState);
                UIFlowUtils::BuildParentsQueue(deactivateQueue); // Collect parents
                UIFlowUtils::BuildActivatedQueue(system, deactivateQueue); // Collect children
            }
            UIFlowUtils::RemoveDuplicates(deactivateQueue);

            // Activation
            activateQueue.push_back(activateState);
            UIFlowUtils::BuildParentsQueue(activateQueue); // Collect parents

            // Cleanup queues
            // Remove activate states from deactivate queue
            deactivateQueue.remove_if([&](UIFlowStateComponent* s) {
                return std::find(activateQueue.begin(), activateQueue.end(), s) != activateQueue.end();
            });
            // Remove active states from activate queue (for only-load remove loaded states)
            activateQueue.remove_if([&](UIFlowStateComponent* s) {
                return onlyLoad ? system->IsStateLoaded(s) : system->IsStateActive(s);
            });
            break;
        }
        default:
        {
            DVASSERT(false);
            break;
        }
        }
    }

    if (!onlyLoad && effect == nullptr) // not for only-load mode
    {
        // Create effect from states configurations
        UIFlowTransitionEffectConfig config;
        if (activateState)
        {
            config.effectIn = activateState->GetEffectConfig().effectIn;
            config.duration = activateState->GetEffectConfig().duration;
        }
        if (deactivateState)
        {
            config.effectOut = deactivateState->GetEffectConfig().effectOut;
            config.duration = std::max(deactivateState->GetEffectConfig().duration, config.duration);
        }
        if (config.effectIn != UIFlowTransitionEffectConfig::Effect::None || config.effectOut != UIFlowTransitionEffectConfig::Effect::None)
        {
            SetEffectConfig(config);
        }
    }

    deactivateIt = deactivateQueue.rbegin();
    activateIt = activateQueue.begin();
}

void UIFlowTransitionTransaction::ApplyStepForeground(UIFlowStateSystem* system, bool autoNext /* = false */)
{
    do
    {
        switch (state)
        {
        case State::Begin:
            state = State::Build;
            break;
        case State::Build:
            BuildTransaction(system);
            if (IsEmpty())
            {
                state = State::Finish;
            }
            else if (!onlyLoad && effect)
            {
                state = State::PrevScreenshot;
            }
            else
            {
                state = State::Deactivate;
            }
            break;
        case State::PrevScreenshot:
            DVASSERT(effect);
            PreparePrevScreenshot(system);
            state = State::Deactivate;
            break;
        case State::Deactivate:
            if (deactivateIt != deactivateQueue.rend())
            {
                system->StartDeactivation(*deactivateIt);
                deactivateIt++;
            }
            else
            {
                deactivateIt = deactivateQueue.rbegin();
                state = State::Unload;
            }
            break;
        case State::Unload:
            if (deactivateIt != deactivateQueue.rend())
            {
                system->ProcessDeactivation(*deactivateIt);
                deactivateIt++;
            }
            else
            {
                deactivateIt = deactivateQueue.rbegin();
                state = State::Release;
            }
            break;
        case State::Release:
            if (deactivateIt != deactivateQueue.rend())
            {
                system->FinishDeactivation(*deactivateIt);
                deactivateIt++;
            }
            else
            {
                state = State::Init;
            }
            break;
        case State::Init:
            if (activateIt != activateQueue.end())
            {
                system->StartActivation(*activateIt);
                activateIt++;
            }
            else
            {
                activateIt = activateQueue.begin();
                state = State::Load;
            }
            break;
        case State::Load:
            if (activateIt != activateQueue.end())
            {
                system->ProcessActivation(*activateIt);
                activateIt++;
            }
            else
            {
                if (onlyLoad)
                {
                    state = State::Finish;
                }
                else
                {
                    activateIt = activateQueue.begin();
                    state = State::Activate;
                }
            }
            break;
        case State::Activate:
            if (activateIt != activateQueue.end())
            {
                system->FinishActivation(*activateIt);
                activateIt++;
            }
            else
            {
                if (effect)
                {
                    state = State::NextScreenshot;
                }
                else
                {
                    state = State::Finish;
                }
            }
            break;
        case State::NextScreenshot:
            DVASSERT(effect);
            PrepareNextScreenshot(system);
            for (UIFlowStateComponent* state : activateQueue)
            {
                system->HideStateView(state);
            }
            effect->Start();
            state = State::Animate;
            break;
        case State::Animate:
            return; // Wait animation end
        case State::Finish:
            if (effect)
            {
                for (UIFlowStateComponent* state : activateQueue)
                {
                    system->ShowStateView(state);
                }
            }
            system->FinishTransaction(this);
            return;
        }
    } while (autoNext);
}

void UIFlowTransitionTransaction::ApplyStepBackground(UIFlowStateSystem* system, bool autoNext /* = false */)
{
    if (thread)
    {
        Thread::Sleep(30);
        if (autoNext)
        {
            ApplyStepBackground(system, true);
        }
        return;
    }

    do
    {
        switch (state)
        {
        case State::Begin:
            state = State::Build;
            break;
        case State::Build:
            BuildTransaction(system);
            if (IsEmpty())
            {
                state = State::Finish;
            }
            else if (!onlyLoad && effect)
            {
                state = State::PrevScreenshot;
            }
            else
            {
                state = State::Deactivate;
            }
            break;
        case State::PrevScreenshot:
            DVASSERT(effect);
            PreparePrevScreenshot(system);
            state = State::Deactivate;
            break;
        case State::Deactivate:
            if (deactivateIt != deactivateQueue.rend())
            {
                system->StartDeactivation(*deactivateIt);
                deactivateIt++;
            }
            else
            {
                deactivateIt = deactivateQueue.rbegin();
                state = State::Unload;
            }
            break;
        case State::Unload:
            // TODO: Think about unloading resources in another thread
            if (deactivateIt != deactivateQueue.rend())
            {
                thread = StartThread([=]() {
                    system->ProcessDeactivation(*deactivateIt);
                    deactivateIt++;
                    if (autoNext)
                    {
                        GetEngineContext()->jobManager->CreateMainJob([=]() {
                            ApplyStepBackground(system, true);
                        });
                    }
                    thread = nullptr;
                });
                return; // break loop for thread
            }
            else
            {
                deactivateIt = deactivateQueue.rbegin();
                state = State::Release;
            }
            break;
        case State::Release:
            if (deactivateIt != deactivateQueue.rend())
            {
                system->FinishDeactivation(*deactivateIt);
                deactivateIt++;
            }
            else
            {
                state = State::Init;
            }
            break;
        case State::Init:
            if (activateIt != activateQueue.end())
            {
                system->StartActivation(*activateIt);
                activateIt++;
            }
            else
            {
                activateIt = activateQueue.begin();
                state = State::Load;
            }
            break;
        case State::Load:
            if (activateIt != activateQueue.end())
            {
                thread = StartThread([=]() {
                    system->ProcessActivation(*activateIt);
                    activateIt++;
                    if (autoNext)
                    {
                        GetEngineContext()->jobManager->CreateMainJob([=]() {
                            ApplyStepBackground(system, true);
                        });
                    }
                    thread = nullptr;
                });
                return; // break loop for thread
            }
            else
            {
                if (onlyLoad)
                {
                    state = State::Finish;
                }
                else
                {
                    activateIt = activateQueue.begin();
                    state = State::Activate;
                }
            }
            break;
        case State::Activate:
            if (activateIt != activateQueue.end())
            {
                system->FinishActivation(*activateIt);
                activateIt++;
            }
            else
            {
                if (effect)
                {
                    state = State::NextScreenshot;
                }
                else
                {
                    state = State::Finish;
                }
            }
            break;
        case State::NextScreenshot:
            DVASSERT(effect);
            PrepareNextScreenshot(system);
            for (UIFlowStateComponent* state : activateQueue)
            {
                system->HideStateView(state);
            }
            effect->Start();
            state = State::Animate;
            break;
        case State::Animate:
            return; // Wait animation end
        case State::Finish:
            if (effect)
            {
                for (UIFlowStateComponent* state : activateQueue)
                {
                    system->ShowStateView(state);
                }
            }
            system->FinishTransaction(this);
            return;
        }
    } while (autoNext);
}

void UIFlowTransitionTransaction::Apply(UIFlowStateSystem* system)
{
    if (isBackgroundLoading)
    {
        ApplyStepBackground(system, true);
    }
    else
    {
        ApplyStepForeground(system, true);
    }
}

void UIFlowTransitionTransaction::Reverse()
{
    std::swap(activateState, deactivateState);
    std::swap(activateQueue, deactivateQueue);
    activateIt = activateQueue.begin();
    deactivateIt = deactivateQueue.rbegin();
    if (effect)
    {
        effect->Reverse();
    }
}

void UIFlowTransitionTransaction::ChangeActivationBranch(const UIFlowTransitionTransaction& patch)
{
    activateState = patch.activateState;
    auto origIt = activateQueue.rbegin();
    auto patchIt = patch.activateQueue.rbegin();
    for (; origIt != activateQueue.rend() && patchIt != patch.activateQueue.rend(); ++origIt, ++patchIt)
    {
        *origIt = *patchIt;
    }
}

RefPtr<Thread> UIFlowTransitionTransaction::StartThread(const Function<void()>& fn)
{
    RefPtr<Thread> t(Thread::Create(fn));
    t->SetStackSize(UIFlowTransitionTransactionDetails::LOADING_THREAD_STACK_SIZE);
    t->Start();
    return t;
}

void UIFlowTransitionTransaction::PreparePrevScreenshot(UIFlowStateSystem* system)
{
    UIRenderSystem* renderSystem = system->GetScene()->GetSystem<UIRenderSystem>();
    UIScreen* screen = system->GetScene()->GetScreen();
    effect->MakePrevShot(renderSystem, screen);
}

void UIFlowTransitionTransaction::PrepareNextScreenshot(UIFlowStateSystem* system)
{
    UIRenderSystem* renderSystem = system->GetScene()->GetSystem<UIRenderSystem>();
    UIScreen* screen = system->GetScene()->GetScreen();
    effect->MakeNextShot(renderSystem, screen);
}

void UIFlowTransitionTransaction::FinishAnimation(UIFlowStateSystem* system)
{
    state = State::Finish;
    ApplyStepForeground(system); // Apply last step
}
}
