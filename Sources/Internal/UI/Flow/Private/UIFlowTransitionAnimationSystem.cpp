#include "UI/Flow/Private/UIFlowTransitionAnimationSystem.h"
#include "Engine/EngineContext.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "UI/Flow/Private/UIFlowTransitionTransaction.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/Flow/Private/UIFlowTransitionEffect.h"

namespace DAVA
{
UIFlowTransitionAnimationSystem::UIFlowTransitionAnimationSystem(UIFlowStateSystem* stateSystem, UIRenderSystem* renderSystem)
    : stateSystem(stateSystem)
    , renderSystem(renderSystem)
{
}

UIFlowTransitionAnimationSystem::~UIFlowTransitionAnimationSystem() = default;

bool UIFlowTransitionAnimationSystem::Render()
{
    UIFlowTransitionTransaction* transaction = stateSystem->GetTopTransitionTransaction();
    if (transaction)
    {
        UIFlowTransitionEffect* effect = transaction->GetEffect();
        if (effect)
        {
            effect->Render(renderSystem);
        }
    }
    return false;
}

void UIFlowTransitionAnimationSystem::Process(float32 elapsedTime)
{
    UIFlowTransitionTransaction* transaction = stateSystem->GetTopTransitionTransaction();
    if (transaction && transaction->IsAnimating())
    {
        UIFlowTransitionEffect* effect = transaction->GetEffect();
        if (effect)
        {
            effect->Process(elapsedTime);
            if (effect->IsFinish())
            {
                transaction->FinishAnimation(stateSystem);
            }
        }
    }
}
}
