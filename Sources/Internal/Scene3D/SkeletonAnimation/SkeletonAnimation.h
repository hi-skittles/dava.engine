#pragma once

#include "Base/BaseTypes.h"
#include "Scene3D/Components/SkeletonComponent.h"

namespace DAVA
{
class AnimationClip;

class SkeletonAnimation
{
public:
    SkeletonAnimation() = default;
    ~SkeletonAnimation();

    void AddAnimationClip(AnimationClip* animationClip, const UnorderedSet<uint32>& ignoreMask = UnorderedSet<uint32>(), float32 startTime = -std::numeric_limits<float32>::max(), float32 endTime = std::numeric_limits<float32>::max());

    void BindSkeleton(const SkeletonComponent* skeleton);
    void BindRootNode(const FastName& rootNodeID);

    void EvaluatePose(float32 animationLocalTime, SkeletonPose* outPose);
    void EvaluateRootPosition(float32 animationLocalTime, Vector3* offset);
    void EvaluateRootOffset(float32 animationLocalTime0, float32 animationLocalTime1, Vector3* offset);

    float32 GetDuration() const;

protected:
    struct SkeletonAnimationClip
    {
        AnimationClip* animationClip = nullptr;
        UnorderedSet<uint32> jointsIgnoreMask;

        Vector<std::pair<uint32, const AnimationTrack*>> boundTracks; //[jointIndex, track]
        const AnimationTrack* rootNodeTrack = nullptr; //for root-node transform extraction
        uint32 rootNodePositionChannel = std::numeric_limits<uint32>::max();

        float32 duration = 0.f;
        float32 clipStartTimestamp = 0.f;
        float32 animationStartTimestamp = 0.f;
    };

    static JointTransform EvaluateJointTransform(float32 time, const AnimationTrack* track);
    void EvaluateRootPosition(SkeletonAnimationClip* clip, float32 animationLocalTime, Vector3* outPosition);
    SkeletonAnimationClip* FindClip(float32 animationTime);
    float32 GetClipLocalTime(SkeletonAnimationClip* clip, float32 animationLocalTime);

    Vector<SkeletonAnimationClip> animationClips;
    uint32 maxJointIndex = 0;
};

} //ns