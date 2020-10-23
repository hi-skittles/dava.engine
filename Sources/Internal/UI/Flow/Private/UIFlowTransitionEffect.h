#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/Flow/UIFlowTransitionEffectConfig.h"

namespace DAVA
{
class Sprite;
class UIControl;
class UIRenderSystem;

/** Transition effects controller. */
class UIFlowTransitionEffect final
{
public:
    /** Default constructor. */
    UIFlowTransitionEffect();
    /** Constructor with specified effects config. */
    UIFlowTransitionEffect(const UIFlowTransitionEffectConfig& config);
    /** Desctructor. */
    ~UIFlowTransitionEffect();

    /** Return true if animations is over. */
    bool IsFinish() const;
    /** Start animation. */
    void Start();
    /** Finish animation. */
    void Stop();
    /** Make screenshot of previous state. */
    void MakePrevShot(UIRenderSystem* renderSystem, UIControl* ctrl);
    /** Make screenshot of next state. */
    void MakeNextShot(UIRenderSystem* renderSystem, UIControl* ctrl);
    /** Process animations. */
    void Process(float32 delta);
    /** Render animations. */
    void Render(UIRenderSystem* renderSystem);
    /** Swap out-in animation effects. */
    void Reverse();

private:
    UIFlowTransitionEffectConfig config;
    RefPtr<Sprite> prevShot;
    RefPtr<Sprite> nextShot;
    float32 position = 0.f;
};
}