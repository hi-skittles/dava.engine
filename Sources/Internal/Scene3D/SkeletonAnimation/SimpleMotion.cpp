#include "SimpleMotion.h"

#include "Animation/AnimationClip.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Components/MotionComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SingleComponents/MotionSingleComponent.h"
#include "Scene3D/SkeletonAnimation/SkeletonAnimation.h"
#include "Scene3D/SkeletonAnimation/SkeletonPose.h"

namespace DAVA
{
SimpleMotion::~SimpleMotion()
{
    SafeRelease(animationClip);
    SafeDelete(skeletonAnimation);
}

void SimpleMotion::BindSkeleton(SkeletonComponent* skeleton)
{
    SafeDelete(skeletonAnimation);
    skeletonAnimation = new SkeletonAnimation();
    skeletonAnimation->AddAnimationClip(animationClip);
    skeletonAnimation->BindSkeleton(skeleton);

    currentAnimationTime = 0.f;
}

void SimpleMotion::Start()
{
    isPlaying = true;
    repeatsLeft = repeatsCount;
}

void SimpleMotion::Stop()
{
    isPlaying = false;
    currentAnimationTime = 0.f;
}

void SimpleMotion::Update(float32 timeElapsed)
{
    if (animationClip == nullptr || skeletonAnimation == nullptr)
        return;

    if (isPlaying)
    {
        currentAnimationTime += timeElapsed;

        if (currentAnimationTime >= animationClip->GetDuration())
        {
            if (repeatsCount != 0)
            {
                --repeatsLeft;
                isPlaying = (repeatsLeft > 0);
            }

            if (isPlaying)
            {
                currentAnimationTime -= animationClip->GetDuration();
            }
        }
    }
}

void SimpleMotion::SetAnimation(AnimationClip* clip)
{
    SafeRelease(animationClip);
    animationClip = SafeRetain(clip);
}

void SimpleMotion::EvaluatePose(SkeletonPose* outPose)
{
    skeletonAnimation->EvaluatePose(currentAnimationTime, outPose);
}
}