#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/** Describe configuration of transition effect between two states. */
struct UIFlowTransitionEffectConfig final
{
    enum Effect : int32
    {
        None = 0,
        Static,
        FadeAlpha,
        Fade,
        Scale,
        Flip,
        MoveLeft,
        MoveRight
    };

    /** Effect then the state out of screen. */
    Effect effectOut = Effect::None;
    /** Effect then the state enter on screen. */
    Effect effectIn = Effect::None;
    /** Effect duration. */
    float32 duration = 0.f;
};
}
