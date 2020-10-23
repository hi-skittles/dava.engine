#pragma once

#include "UI/UISystem.h"

namespace DAVA
{
class UIControl;
class UIFlowStateSystem;
class UIRenderSystem;

/**
    Manage all UIFlowViewComponents and control screens.
*/
class UIFlowTransitionAnimationSystem final : public UISystem
{
public:
    UIFlowTransitionAnimationSystem(UIFlowStateSystem* stateSystem, UIRenderSystem* renderSystem);

    ~UIFlowTransitionAnimationSystem() override;

    bool Render();

protected:
    void Process(float32 elapsedTime) override;

private:
    UIFlowStateSystem* stateSystem = nullptr;
    UIRenderSystem* renderSystem = nullptr;
};
}
