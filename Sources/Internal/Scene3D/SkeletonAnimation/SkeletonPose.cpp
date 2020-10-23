#include "SkeletonPose.h"
#include "Math/MathHelpers.h"

namespace DAVA
{
SkeletonPose::SkeletonPose(uint32 jointCount)
{
    SetJointCount(jointCount);
}

void SkeletonPose::Add(const SkeletonPose& other)
{
    uint32 jointCount = other.GetJointsCount();
    SetJointCount(Max(GetJointsCount(), jointCount));

    for (uint32 j = 0; j < jointCount; ++j)
    {
        const JointTransform& transform0 = GetJointTransform(j);
        const JointTransform& transform1 = other.GetJointTransform(j);
        SetTransform(j, transform0.AppendTransform(transform1));
    }
}

void SkeletonPose::Diff(const SkeletonPose& other)
{
    uint32 jointCount = other.GetJointsCount();
    SetJointCount(Max(GetJointsCount(), jointCount));

    for (uint32 j = 0; j < jointCount; ++j)
    {
        const JointTransform& transform0 = GetJointTransform(j);
        const JointTransform& transform1 = other.GetJointTransform(j);
        SetTransform(j, transform0.GetInverse().AppendTransform(transform1));
    }
}

void SkeletonPose::Override(const SkeletonPose& other)
{
    uint32 jointCount = other.GetJointsCount();
    SetJointCount(Max(GetJointsCount(), jointCount));

    for (uint32 j = 0; j < jointCount; ++j)
    {
        const JointTransform& transform0 = GetJointTransform(j);
        const JointTransform& transform1 = other.GetJointTransform(j);
        SetTransform(j, JointTransform::Override(transform0, transform1));
    }
}

void SkeletonPose::Lerp(const SkeletonPose& other, float32 factor)
{
    uint32 jointCount = other.GetJointsCount();
    SetJointCount(Max(GetJointsCount(), jointCount));

    for (uint32 j = 0; j < jointCount; ++j)
    {
        const JointTransform& transform0 = GetJointTransform(j);
        const JointTransform& transform1 = other.GetJointTransform(j);
        SetTransform(j, JointTransform::Lerp(transform0, transform1, factor));
    }
}

} //ns