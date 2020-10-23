#include "UI/Flow/UIFlowTransitionEffectConfig.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::UIFlowTransitionEffectConfig::Effect)
{
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::None, "None");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::Static, "Static");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::FadeAlpha, "FadeAlpha");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::Fade, "Fade");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::Scale, "Scale");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::Flip, "Flip");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::MoveLeft, "MoveLeft");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionEffectConfig::Effect::MoveRight, "MoveRight");
}