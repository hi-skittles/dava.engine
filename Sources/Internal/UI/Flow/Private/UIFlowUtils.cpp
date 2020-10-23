#include "UI/Flow/Private/UIFlowUtils.h"
#include "UI/Flow/UIFlowStateComponent.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/UIControl.h"

namespace DAVA
{
void UIFlowUtils::RemoveDuplicates(List<UIFlowStateComponent*>& queue)
{
    if (queue.size() <= 1)
    {
        return;
    }

    Set<UIFlowStateComponent*> uniqueSet;
    queue.remove_if([&](UIFlowStateComponent* state) {
        return !uniqueSet.insert(state).second;
    });
}

void UIFlowUtils::RemoveSharingParentsFromQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue)
{
    UIFlowStateComponent* target = queue.back();
    auto rit = ++queue.rbegin(); // Skip last element because it our target element
    for (; rit != queue.rend(); ++rit)
    {
        UIFlowStateComponent* state = *rit;

        // Check target's state mode
        if (state->GetStateType() != UIFlowStateComponent::STATE_GROUP)
        {
            queue.erase(queue.begin(), rit.base());
            return;
        }

        // Check target's sibling elements
        UIControl* ctrl = state->GetControl();
        for (const auto& c : ctrl->GetChildren())
        {
            if (c == target->GetControl())
            {
                continue; // Skip target
            }
            UIFlowStateComponent* siblingState = c->GetComponent<UIFlowStateComponent>();
            if (siblingState)
            {
                if (system->IsStateActive(siblingState))
                {
                    queue.erase(queue.begin(), rit.base());
                    return;
                }
            }
        }
        target = state;
    }
}

void UIFlowUtils::RemoveActiveParentsFromQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue)
{
    auto rit = ++queue.rbegin(); // Skip last element because it our target element
    for (; rit != queue.rend(); ++rit)
    {
        UIFlowStateComponent* state = *rit;
        // Check target's sibling elements
        if (system->IsStateActive(state))
        {
            queue.erase(queue.begin(), rit.base());
            return;
        }
    }
}

void UIFlowUtils::BuildParentsQueue(List<UIFlowStateComponent*>& queue)
{
    if (queue.empty())
    {
        return;
    }

    auto* front = queue.front();

    if (front == nullptr)
    {
        return;
    }

    auto* ctrl = front->GetControl()->GetParent();
    while (ctrl)
    {
        UIFlowStateComponent* state = ctrl->GetComponent<UIFlowStateComponent>();
        if (state)
        {
            queue.push_front(state);
        }
        ctrl = ctrl->GetParent();
    }
}

void UIFlowUtils::BuildActivatedQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue)
{
    if (queue.empty())
    {
        return;
    }

    auto* back = queue.back();

    if (back == nullptr)
    {
        return;
    }

    if (!system->IsStateInitted(back))
    {
        queue.pop_back();
        return;
    }

    auto* ctrl = back->GetControl();
    const auto& children = ctrl->GetChildren();
    for (const auto& child : children)
    {
        UIFlowStateComponent* state = child->GetComponent<UIFlowStateComponent>();
        DVASSERT(state);
        if (system->IsStateInitted(state))
        {
            queue.push_back(state);
            BuildActivatedQueue(system, queue);
        }
    }
}

bool UIFlowUtils::IsSibling(UIFlowStateComponent* first, UIFlowStateComponent* second)
{
    if (!first || !second)
    {
        return false;
    }
    return first->GetControl()->GetParent() == second->GetControl()->GetParent();
}

UIFlowStateComponent* UIFlowUtils::FindNearChildSingleState(UIFlowStateComponent* parent)
{
    if (parent)
    {
        UIControl* ctrl = parent->GetControl();
        for (const auto& c : ctrl->GetChildren())
        {
            UIFlowStateComponent* state = c->GetComponent<UIFlowStateComponent>();
            if (state)
            {
                if (state->GetStateType() == UIFlowStateComponent::STATE_SINGLE)
                {
                    return state;
                }
            }
            state = FindNearChildSingleState(state);
            if (state)
            {
                return state;
            }
        }
    }
    return nullptr;
}
}
