#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"

namespace DAVA
{
class AnimationClip;
class SkeletonAnimation;
class SkeletonComponent;
class SkeletonPose;
class SimpleMotion
{
public:
    SimpleMotion() = default;
    ~SimpleMotion();

    void BindSkeleton(SkeletonComponent* skeleton);
    void Start();
    void Stop();
    void Update(float32 dTime);

    void EvaluatePose(SkeletonPose* outPose);

    bool IsPlaying() const;

    void SetAnimation(AnimationClip* animationClip);
    AnimationClip* GetAnimation() const;

    uint32 GetRepeatsCount() const;
    void SetRepeatsCount(uint32 count);

protected:
    AnimationClip* animationClip = nullptr;
    SkeletonAnimation* skeletonAnimation = nullptr;

    uint32 repeatsCount = 0;
    uint32 repeatsLeft = 0;
    float32 currentAnimationTime = 0.f;

    bool isPlaying = false;
};

inline bool SimpleMotion::IsPlaying() const
{
    return isPlaying;
}

inline AnimationClip* SimpleMotion::GetAnimation() const
{
    return animationClip;
}

inline uint32 SimpleMotion::GetRepeatsCount() const
{
    return repeatsCount;
}

inline void SimpleMotion::SetRepeatsCount(uint32 count)
{
    repeatsCount = count;
}

} //ns