#include "UI/Flow/UIFlowStateSystem.h"
#include "Base/List.h"
#include "Base/TemplateHelpers.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "Logger/Logger.h"
#include "UI/DefaultUIPackageBuilder.h"
#include "UI/Events/UIEventsSingleComponent.h"
#include "UI/Flow/Private/UIFlowTransitionTransaction.h"
#include "UI/Flow/Private/UIFlowUtils.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/Flow/UIFlowController.h"
#include "UI/Flow/UIFlowControllerComponent.h"
#include "UI/Flow/UIFlowControllerSystem.h"
#include "UI/Flow/UIFlowService.h"
#include "UI/Flow/UIFlowStateComponent.h"
#include "UI/Flow/UIFlowTransitionComponent.h"
#include "UI/Flow/UIFlowViewComponent.h"
#include "UI/Flow/UIFlowViewSystem.h"
#include "UI/UIControl.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIControlSystem.h"

namespace DAVA
{
static void StateLog(const String& msg, UIFlowStateComponent* state)
{
    Logger::Debug("%s [%s]", msg.c_str(), (state != nullptr && state->GetControl() != nullptr ? state->GetControl()->GetName().c_str() : "null"));
}

UIFlowStateSystem::UIFlowStateSystem()
    : context(std::make_unique<UIFlowContext>())
{
}

UIFlowStateSystem::~UIFlowStateSystem() = default;

UIFlowStateComponent* UIFlowStateSystem::FindStateByPath(const String& statePath) const
{
    UIControl* flowRoot = GetScene()->GetFlowRoot();
    if (flowRoot)
    {
        UIControl* stateControl = nullptr;

        // TODO: think about search in current State
        // Search from current Single State
        if (currentState)
        {
            if (statePath.empty())
            {
                stateControl = currentState->GetControl();
            }
            else
            {
                stateControl = currentState->GetControl()->FindByPath(statePath);
            }
        }

        // Search from root State
        if (stateControl == nullptr)
        {
            if (statePath.empty())
            {
                stateControl = flowRoot;
            }
            else
            {
                stateControl = flowRoot->FindByPath(statePath);
            }
        }

        if (stateControl)
        {
            return stateControl->GetComponent<UIFlowStateComponent>();
        }
    }
    return nullptr;
}

UIControl* UIFlowStateSystem::GetFlowRoot() const
{
    return GetScene()->GetFlowRoot();
}

void UIFlowStateSystem::SetFlowRoot(UIControl* flowRoot)
{
    if (GetFlowRoot() != flowRoot)
    {
        HardReset();
        GetScene()->SetFlowRoot(flowRoot);
    }
}

UIFlowContext* UIFlowStateSystem::GetContext() const
{
    return context.get();
}

UIFlowTransitionTransaction* UIFlowStateSystem::GetTopTransitionTransaction() const
{
    return transitionTransactions.empty() ? nullptr : transitionTransactions.front().get();
}

void UIFlowStateSystem::RegisterControl(UIControl* control)
{
    UIFlowStateComponent* c = control->GetComponent<UIFlowStateComponent>();
    if (c)
    {
        AddStateLink(c);
    }
}

void UIFlowStateSystem::UnregisterControl(UIControl* control)
{
    UIFlowStateComponent* c = control->GetComponent<UIFlowStateComponent>();
    if (c)
    {
        RemoveStateLink(c);
    }
}

void UIFlowStateSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowStateComponent>())
    {
        UIFlowStateComponent* c = DynamicTypeCheck<UIFlowStateComponent*>(component);
        AddStateLink(c);
    }
}

void UIFlowStateSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIFlowStateComponent>())
    {
        UIFlowStateComponent* c = DynamicTypeCheck<UIFlowStateComponent*>(component);
        RemoveStateLink(c);
    }
}

void UIFlowStateSystem::ActivateState(UIFlowStateComponent* state, bool background)
{
    if (state)
    {
        PutTransitionTransaction(std::make_unique<UIFlowTransitionTransaction>(state, nullptr, background));
    }
}

void UIFlowStateSystem::DeactivateState(UIFlowStateComponent* state, bool background)
{
    if (state)
    {
        PutTransitionTransaction(std::make_unique<UIFlowTransitionTransaction>(nullptr, state, background));
    }
}

void UIFlowStateSystem::PreloadState(UIFlowStateComponent* state, bool background)
{
    if (state)
    {
        PutTransitionTransaction(std::make_unique<UIFlowTransitionTransaction>(state, nullptr, background, true));
    }
}

void UIFlowStateSystem::DeactivateAllStates()
{
    UIControl* rootCtrl = GetFlowRoot();
    if (rootCtrl)
    {
        DeactivateState(rootCtrl->GetComponent<UIFlowStateComponent>(), false);
    }
}

void UIFlowStateSystem::HardReset()
{
    DVASSERT(!HasTransitions());
    DeactivateAllStates();
    ApplyTransition();
    DVASSERT(currentState == nullptr);
    DVASSERT(currentMultipleStates.empty());
    DVASSERT(transitionTransactions.empty());
    historyTransactions.clear();
}

void UIFlowStateSystem::Process(float32 elapsedTime)
{
    if (HasTransitions())
    {
        ApplyTransition();
    }
}

bool UIFlowStateSystem::IsStateInitted(UIFlowStateComponent* state) const
{
    auto findIt = links.find(state);
    if (findIt != links.end())
    {
        return findIt->second.status >= StateLink::Initted;
    }
    return false;
}

bool UIFlowStateSystem::IsStateLoaded(UIFlowStateComponent* state) const
{
    auto findIt = links.find(state);
    if (findIt != links.end())
    {
        return findIt->second.status >= StateLink::Loaded;
    }
    return false;
}

bool UIFlowStateSystem::IsStateActive(UIFlowStateComponent* state) const
{
    auto findIt = links.find(state);
    if (findIt != links.end())
    {
        return findIt->second.status >= StateLink::Activated;
    }
    return false;
}

bool UIFlowStateSystem::HasTransitions() const
{
    return !transitionTransactions.empty();
}

void UIFlowStateSystem::ApplyTransition()
{
    if (!transitionTransactions.empty())
    {
        std::unique_ptr<UIFlowTransitionTransaction>& top = transitionTransactions.front();
        if (top->IsNew())
        {
            top->Apply(this);
        }
    }
}

bool UIFlowStateSystem::ProcessEvent(const FastName& eventName)
{
    UIControl* root = GetScene()->GetFlowRoot();
    if (root == nullptr)
    {
        return false;
    }

    List<UIFlowStateComponent*> eventQueue;
    UIFlowStateComponent* state = root->GetComponent<UIFlowStateComponent>();
    eventQueue.push_back(state);
    UIFlowUtils::BuildActivatedQueue(this, eventQueue);

    bool processed = false;
    for (auto rit = eventQueue.rbegin(); rit != eventQueue.rend() && !processed; ++rit)
    {
        UIFlowStateComponent* state = *rit;
        UIControl* stateControl = state->GetControl();

        // Process controller
        UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
        UIFlowControllerComponent* controllerComponent = stateControl->GetComponent<UIFlowControllerComponent>();
        if (controllerComponent && controllerSys)
        {
            UIFlowController* controller = controllerSys->GetController(controllerComponent);
            if (controller)
            {
                processed = controller->ProcessEvent(eventName);
                if (processed)
                {
                    break;
                }
            }
        }

        // Process transitions
        UIFlowTransitionComponent* trans = stateControl->GetComponent<UIFlowTransitionComponent>();
        if (trans)
        {
            const UIFlowTransitionComponent::TransitionMap& transitions = trans->GetTransitionMap();
            for (const UIFlowTransitionComponent::TransitionRule& rule : transitions)
            {
                if (rule.event != eventName)
                {
                    continue;
                }

                switch (rule.operation)
                {
                case UIFlowTransitionComponent::ACTIVATE_STATE:
                case UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND:
                {
                    UIControl* findStateControl = stateControl->FindByPath(rule.statePath);
                    if (findStateControl)
                    {
                        ActivateState(findStateControl->GetComponent<UIFlowStateComponent>(), rule.operation == UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND);
                    }
                    else
                    {
                        ActivateState(FindStateByPath(rule.statePath), rule.operation == UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND);
                    }
                    break;
                }
                case UIFlowTransitionComponent::DEACTIVATE_STATE:
                case UIFlowTransitionComponent::DEACTIVATE_STATE_BACKGROUND:
                {
                    if (rule.statePath.empty() || rule.statePath == "@") // TODO: make support edit empty value in QE
                    {
                        DeactivateState(state, rule.operation == UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND);
                    }
                    else
                    {
                        UIControl* findStateControl = stateControl->FindByPath(rule.statePath);
                        if (findStateControl)
                        {
                            DeactivateState(findStateControl->GetComponent<UIFlowStateComponent>(), rule.operation == UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND);
                        }
                        else
                        {
                            DeactivateState(FindStateByPath(rule.statePath), rule.operation == UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND);
                        }
                    }
                    break;
                }
                case UIFlowTransitionComponent::PRELOAD_STATE:
                case UIFlowTransitionComponent::PRELOAD_STATE_BACKGROUND:
                {
                    UIControl* findStateControl = stateControl->FindByPath(rule.statePath);
                    if (findStateControl)
                    {
                        PreloadState(findStateControl->GetComponent<UIFlowStateComponent>(), rule.operation == UIFlowTransitionComponent::PRELOAD_STATE_BACKGROUND);
                    }
                    else
                    {
                        PreloadState(FindStateByPath(rule.statePath), rule.operation == UIFlowTransitionComponent::PRELOAD_STATE_BACKGROUND);
                    }
                    break;
                }
                case UIFlowTransitionComponent::SEND_EVENT:
                {
                    UIEventsSingleComponent* events = GetScene()->GetSingleComponent<UIEventsSingleComponent>();
                    if (events)
                    {
                        events->SendEvent(state->GetControl(), rule.sendEvent, Any());
                    }
                    break;
                }
                case UIFlowTransitionComponent::HISTORY_BACK:
                {
                    HistoryBack();
                    break;
                }
                default:
                    DVASSERT(false);
                    break;
                };
                processed = true;
            }
            if (processed)
            {
                break;
            }
        }
    }

    return processed;
}

void UIFlowStateSystem::StartActivation(UIFlowStateComponent* state)
{
    if (IsStateInitted(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::StartActivation", state);

    const Vector<UIFlowStateComponent::ServiceDescriptor>& services = state->GetServices();
    for (const UIFlowStateComponent::ServiceDescriptor& s : services)
    {
        context->InitServiceByType(FastName(s.name), s.nativeType);
    }

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->InitController(controllerComponent, context.get());
    }

    links[state].status = StateLink::Initted;
}

void UIFlowStateSystem::ProcessActivation(UIFlowStateComponent* state)
{
    if (IsStateLoaded(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::ProcessActivation", state);

    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    UIControl* view = nullptr;
    if (viewComponent && viewSys)
    {
        view = viewSys->InitView(viewComponent, context.get());
    }

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->LoadController(controllerComponent, context.get(), view);
    }

    links[state].status = StateLink::Loaded;
}

void UIFlowStateSystem::FinishActivation(UIFlowStateComponent* state)
{
    if (IsStateActive(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::FinishActivation", state);

    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    UIControl* view = nullptr;
    if (viewComponent && viewSys)
    {
        view = viewSys->ActivateView(viewComponent, context.get());
    }

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->ActivateController(controllerComponent, context.get(), view);
    }

    // Update system states
    switch (state->GetStateType())
    {
    case UIFlowStateComponent::STATE_SINGLE:
        currentState = state;
        break;
    case UIFlowStateComponent::STATE_MULTIPLE:
        currentMultipleStates.push_back(state);
        break;
    default:
        break;
    }

    UIEventsSingleComponent* eventSingle = GetScene()->GetSingleComponent<UIEventsSingleComponent>();
    for (const FastName& event : state->GetActivateEvents())
    {
        eventSingle->SendEvent(state->GetControl(), event, Any());
    }

    links[state].status = StateLink::Activated;
}

void UIFlowStateSystem::StartDeactivation(UIFlowStateComponent* state)
{
    if (!IsStateInitted(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::StartDeactivation", state);

    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    UIControl* view = nullptr;
    if (viewComponent && viewSys)
    {
        view = viewSys->GetLinkedView(viewComponent);
    }

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->DeactivateController(controllerComponent, context.get(), view);
    }

    if (viewComponent && viewSys)
    {
        viewSys->DeactivateView(viewComponent);
    }

    links[state].status = StateLink::Loaded;
}

void UIFlowStateSystem::ProcessDeactivation(UIFlowStateComponent* state)
{
    if (!IsStateLoaded(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::ProcessDeactivation", state);

    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    UIControl* view = nullptr;
    if (viewComponent && viewSys)
    {
        view = viewSys->GetLinkedView(viewComponent);
    }

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->UnloadController(controllerComponent, context.get(), view);
    }

    if (viewComponent && viewSys)
    {
        viewSys->ReleaseView(viewComponent);
    }

    links[state].status = StateLink::Initted;
}

void UIFlowStateSystem::FinishDeactivation(UIFlowStateComponent* state)
{
    if (!IsStateInitted(state))
    {
        return;
    }

    StateLog("UIFlowStateSystem::FinishDeactivation", state);

    UIFlowControllerSystem* controllerSys = GetScene()->GetSystem<UIFlowControllerSystem>();
    UIFlowControllerComponent* controllerComponent = state->GetControl()->GetComponent<UIFlowControllerComponent>();
    if (controllerComponent && controllerSys)
    {
        controllerSys->ReleaseController(controllerComponent, context.get());
    }

    const Vector<UIFlowStateComponent::ServiceDescriptor>& services = state->GetServices();
    for (const UIFlowStateComponent::ServiceDescriptor& s : services)
    {
        context->ReleaseService(FastName(s.name));
    }

    UIEventsSingleComponent* eventSingle = GetScene()->GetSingleComponent<UIEventsSingleComponent>();
    for (const FastName& event : state->GetDeactivateEvents())
    {
        eventSingle->SendEvent(state->GetControl(), event, Any());
    }

    // Update system's states
    switch (state->GetStateType())
    {
    case UIFlowStateComponent::STATE_SINGLE:
        DVASSERT(currentState == state);
        if (currentState == state)
        {
            currentState = nullptr;
        }
        break;
    case UIFlowStateComponent::STATE_MULTIPLE:
        currentMultipleStates.erase(std::remove(currentMultipleStates.begin(), currentMultipleStates.end(), state), currentMultipleStates.end());
        break;
    default:
        break;
    }

    links[state].status = StateLink::Off;
}

void UIFlowStateSystem::HideStateView(UIFlowStateComponent* state)
{
    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    if (viewComponent && viewSys)
    {
        UIControl* view = viewSys->GetLinkedView(viewComponent);
        if (view)
        {
            view->SetVisibilityFlag(false);
        }
    }
}

void UIFlowStateSystem::ShowStateView(UIFlowStateComponent* state)
{
    UIFlowViewSystem* viewSys = GetScene()->GetSystem<UIFlowViewSystem>();
    UIFlowViewComponent* viewComponent = state->GetControl()->GetComponent<UIFlowViewComponent>();
    if (viewComponent && viewSys)
    {
        UIControl* view = viewSys->GetLinkedView(viewComponent);
        if (view)
        {
            view->SetVisibilityFlag(true);
        }
    }
}

void UIFlowStateSystem::FinishTransaction(const UIFlowTransitionTransaction* transaction)
{
    if (!transitionTransactions.empty())
    {
        std::unique_ptr<UIFlowTransitionTransaction>& top = transitionTransactions.front();
        DVASSERT(top.get() == transaction);
        if (top.get() == transaction)
        {
            if (!top->IsEmpty())
            {
                PutHistoryTransaction(std::move(top));
            }
            transitionTransactions.pop_front();
        }
    }
}

void UIFlowStateSystem::PutTransitionTransaction(std::unique_ptr<UIFlowTransitionTransaction> transaction)
{
    if (transaction)
    {
        transitionTransactions.push_back(std::move(transaction));
    }
}

void UIFlowStateSystem::PutHistoryTransaction(std::unique_ptr<UIFlowTransitionTransaction> transaction)
{
    if (transaction && !transaction->IsSkipHistory())
    {
        UIFlowStateComponent* activateState = transaction->GetActivateState();
        UIFlowStateComponent* deactivateState = transaction->GetDeactivateState();

        // Skip if activate state shouldn't store to history
        if (activateState && activateState->GetHistoryType() == UIFlowStateComponent::HISTORY_NONE)
        {
            return;
        }

        // Skip if deactivate state shouldn't store to history
        if (deactivateState && deactivateState->GetHistoryType() == UIFlowStateComponent::HISTORY_NONE)
        {
            return;
        }

        if (activateState && activateState->GetHistoryType() == UIFlowStateComponent::HISTORY_SINGLE)
        {
            auto it = std::find_if(historyTransactions.begin(), historyTransactions.end(), [&](std::unique_ptr<UIFlowTransitionTransaction>& t) {
                return t->GetDeactivateState() == activateState;
            });
            if (it != historyTransactions.end())
            {
                historyTransactions.erase(it, historyTransactions.end());
                return;
            }
        }

        if (activateState && activateState->GetHistoryType() == UIFlowStateComponent::HISTORY_SINGLE_SIBLING)
        {
            auto it = std::find_if(historyTransactions.begin(), historyTransactions.end(), [&](std::unique_ptr<UIFlowTransitionTransaction>& t) {
                return UIFlowUtils::IsSibling(t->GetActivateState(), activateState);
            });
            if (it != historyTransactions.end())
            {
                (*it)->ChangeActivationBranch(*transaction);
                historyTransactions.erase(++it, historyTransactions.end());
                return;
            }
        }

        if (!historyTransactions.empty())
        {
            const std::unique_ptr<UIFlowTransitionTransaction>& last = historyTransactions.back();
            const UIFlowStateComponent* lastActivate = historyTransactions.back()->GetActivateState();
            if (lastActivate && lastActivate->GetHistoryType() == UIFlowStateComponent::HISTORY_ONLY_TOP)
            {
                if (transaction->GetDeactivateState() == lastActivate && transaction->GetActivateState() == nullptr) // Just deactivate last ONLY_TOP
                {
                    historyTransactions.pop_back();
                }
                else // Replace activation branch for last histroy element
                {
                    historyTransactions.back()->ChangeActivationBranch(*transaction);
                }
                return;
            }
        }

        historyTransactions.push_back(std::move(transaction));
    }
}

void UIFlowStateSystem::HistoryBack()
{
    if (!historyTransactions.empty())
    {
        std::unique_ptr<UIFlowTransitionTransaction> prev = std::move(historyTransactions.back());
        historyTransactions.pop_back();

        prev->Clean();
        prev->Reverse();
        prev->SetSkipHistory(true);
        PutTransitionTransaction(std::move(prev));
    }
}

void UIFlowStateSystem::AddStateLink(UIFlowStateComponent* component)
{
    links[component] = StateLink(component);
}

void UIFlowStateSystem::RemoveStateLink(UIFlowStateComponent* component)
{
    auto it = links.find(component);
    if (it != links.end())
    {
        switch (it->second.status)
        {
        case StateLink::Activated:
            StartDeactivation(it->second.state);
        case StateLink::Loaded:
            ProcessDeactivation(it->second.state);
        case StateLink::Initted:
            FinishDeactivation(it->second.state);
        case StateLink::Off:
            break;
        }
        links.erase(it);
    }
}
}
