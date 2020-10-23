#include "Animation/AnimatedObject.h"
#include "Animation/AnimationManager.h"
#include "Engine/Engine.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(AnimatedObject)
{
    ReflectionRegistrator<AnimatedObject>::Begin()
    .End();
}

AnimatedObject::AnimatedObject() = default;

AnimatedObject::~AnimatedObject()
{
    StopAnimations();
}

void AnimatedObject::StopAnimations(int32 track)
{
    GetEngineContext()->animationManager->DeleteAnimations(this, track);
}

bool AnimatedObject::IsAnimating(int32 track) const
{
    return GetEngineContext()->animationManager->IsAnimating(this, track);
}

Animation* AnimatedObject::FindPlayingAnimation(int32 track /*= -1*/) const
{
    return GetEngineContext()->animationManager->FindPlayingAnimation(this, track);
}
}
