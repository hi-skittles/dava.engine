#include "SkeletonAnimation.h"

#include "Animation/AnimationClip.h"
#include "Animation/AnimationTrack.h"

namespace DAVA
{
SkeletonAnimation::~SkeletonAnimation()
{
    for (SkeletonAnimationClip& clip : animationClips)
        SafeRelease(clip.animationClip);
}

void SkeletonAnimation::AddAnimationClip(AnimationClip* animationClip, const UnorderedSet<uint32>& ignoreMask, float32 startTime, float32 endTime)
{
    if (animationClip != nullptr)
    {
        SkeletonAnimationClip clip;
        clip.animationClip = SafeRetain(animationClip);
        clip.jointsIgnoreMask = ignoreMask;
        clip.clipStartTimestamp = std::max(0.f, startTime);
        clip.duration = std::min(clip.animationClip->GetDuration(), endTime - clip.clipStartTimestamp);
        clip.animationStartTimestamp = animationClips.empty() ? 0.f : (animationClips.back().animationStartTimestamp + animationClips.back().duration);

        animationClips.emplace_back(clip);
    }
}

void SkeletonAnimation::BindSkeleton(const SkeletonComponent* skeleton)
{
    DVASSERT(skeleton);

    maxJointIndex = 0;
    for (SkeletonAnimationClip& clip : animationClips)
    {
        clip.boundTracks.clear();

        uint32 trackCount = clip.animationClip->GetTrackCount();
        uint32 jointCount = skeleton->GetJointsCount();
        for (uint32 j = 0; j < jointCount; ++j)
        {
            if (clip.jointsIgnoreMask.count(j) != 0)
                continue;

            const SkeletonComponent::Joint& joint = skeleton->GetJoint(j);

            const AnimationTrack* track = clip.animationClip->FindTrack(joint.uid.c_str());
            if (track != nullptr)
            {
                clip.boundTracks.emplace_back(std::make_pair(j, track));
                maxJointIndex = Max(maxJointIndex, j);
            }
        }
    }
}

void SkeletonAnimation::BindRootNode(const FastName& rootNodeID)
{
    for (SkeletonAnimationClip& clip : animationClips)
    {
        clip.rootNodeTrack = clip.animationClip->FindTrack(rootNodeID.c_str());
        if (clip.rootNodeTrack != nullptr)
        {
            for (uint32 c = 0; c < clip.rootNodeTrack->GetChannelsCount(); ++c)
            {
                if (clip.rootNodeTrack->GetChannelTarget(c) == AnimationTrack::CHANNEL_TARGET_POSITION)
                {
                    clip.rootNodePositionChannel = c;
                    break;
                }
            }
        }
    }
}

void SkeletonAnimation::EvaluatePose(float32 animationLocalTime, SkeletonPose* outPose)
{
    if (animationClips.empty())
        return;

    DVASSERT(outPose);
    outPose->SetJointCount(maxJointIndex + 1);

    SkeletonAnimationClip* clip = FindClip(animationLocalTime);

    uint32 boundTrackCount = uint32(clip->boundTracks.size());
    for (uint32 t = 0; t < boundTrackCount; ++t)
    {
        uint32 jointIndex = clip->boundTracks[t].first;
        const AnimationTrack* track = clip->boundTracks[t].second;

        outPose->SetTransform(jointIndex, EvaluateJointTransform(animationLocalTime, track));
    }
}

void SkeletonAnimation::EvaluateRootOffset(float32 animationLocalTime0, float32 animationLocalTime1, Vector3* offset)
{
    if (animationClips.empty())
        return;

    SkeletonAnimationClip* clip0 = FindClip(animationLocalTime0);
    SkeletonAnimationClip* clip1 = FindClip(animationLocalTime1);

    if (animationLocalTime0 > animationLocalTime1 || clip0 != clip1)
    {
        Vector3 clip0End, clip1Start;
        EvaluateRootPosition(clip1, clip1->animationStartTimestamp, &clip1Start);
        EvaluateRootPosition(clip0, clip0->animationStartTimestamp + clip0->duration, &clip0End);

        Vector3 offset0, offset1;
        EvaluateRootPosition(clip0, animationLocalTime0, &offset0);
        EvaluateRootPosition(clip1, animationLocalTime1, &offset1);

        *offset = (clip0End - offset0) + (offset1 - clip1Start);
    }
    else
    {
        Vector3 position;
        EvaluateRootPosition(clip0, animationLocalTime0, &position);
        EvaluateRootPosition(clip1, animationLocalTime1, offset);
        *offset -= position;
    }
}

void SkeletonAnimation::EvaluateRootPosition(float32 animationLocalTime, Vector3* offset)
{
    if (animationClips.empty())
        return;

    EvaluateRootPosition(FindClip(animationLocalTime), animationLocalTime, offset);
}

float32 SkeletonAnimation::GetDuration() const
{
    return animationClips.empty() ? 0.f : (animationClips.back().animationStartTimestamp + animationClips.back().duration);
}

//////////////////////////////////////////////////////////////////////////

JointTransform SkeletonAnimation::EvaluateJointTransform(float32 time, const AnimationTrack* track)
{
    static const uint32 MAX_CHANNEL_VALUE_SIZE = 4;
    DVASSERT(MAX_CHANNEL_VALUE_SIZE >= track->GetMaxChannelValueSize());

    JointTransform transform;
    Array<float32, MAX_CHANNEL_VALUE_SIZE> workData;
    for (uint32 c = 0; c < track->GetChannelsCount(); ++c)
    {
        track->Evaluate(time, c, workData.data(), uint32(workData.size()));

        AnimationTrack::eChannelTarget target = track->GetChannelTarget(c);
        switch (target)
        {
        case AnimationTrack::CHANNEL_TARGET_POSITION:
            DVASSERT(track->GetChannelValueSize(c) == 3);
            transform.SetPosition(Vector3(workData.data()));
            break;

        case AnimationTrack::CHANNEL_TARGET_ORIENTATION:
            DVASSERT(track->GetChannelValueSize(c) == 4);
            transform.SetOrientation(Quaternion(workData.data()));
            break;

        case AnimationTrack::CHANNEL_TARGET_SCALE:
            DVASSERT(track->GetChannelValueSize(c) == 1);
            transform.SetScale(*workData.data());
            break;

        default:
            break;
        }
    }

    return transform;
}

void SkeletonAnimation::EvaluateRootPosition(SkeletonAnimationClip* clip, float32 animationLocalTime, Vector3* outPosition)
{
    DVASSERT(clip != nullptr);

    if (clip->rootNodePositionChannel != std::numeric_limits<uint32>::max() && clip->rootNodeTrack != nullptr)
    {
        clip->rootNodeTrack->Evaluate(GetClipLocalTime(clip, animationLocalTime), clip->rootNodePositionChannel, outPosition->data, uint32(Vector3::AXIS_COUNT));
    }
}

SkeletonAnimation::SkeletonAnimationClip* SkeletonAnimation::FindClip(float32 animationTime)
{
    DVASSERT(!animationClips.empty());

    auto found = std::find_if(animationClips.begin(), animationClips.end(), [&animationTime](const SkeletonAnimationClip& clip) {
        return clip.animationStartTimestamp > animationTime;
    });

    if (found != animationClips.begin())
        --found;

    return &(*found);
}

float32 SkeletonAnimation::GetClipLocalTime(SkeletonAnimationClip* clip, float32 animationLocalTime)
{
    return clip->clipStartTimestamp + animationLocalTime - clip->animationStartTimestamp;
}

} //ns