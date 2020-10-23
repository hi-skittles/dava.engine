#pragma once

#include "Base/BaseTypes.h"
#include "Scene3D/SkeletonAnimation/JointTransform.h"

namespace DAVA
{
class SkeletonPose
{
public:
    SkeletonPose(uint32 jointCount = 0);

    void SetJointCount(uint32 jointCount);
    uint32 GetJointsCount() const;

    void Reset();

    void SetTransform(uint32 jointIndex, const JointTransform& transform);

    void SetPosition(uint32 jointIndex, const Vector3& position);
    void SetOrientation(uint32 jointIndex, const Quaternion& orientation);
    void SetScale(uint32 jointIndex, float32 scale);

    const JointTransform& GetJointTransform(uint32 jointIndex) const;

    void Add(const SkeletonPose& other);
    void Diff(const SkeletonPose& other);
    void Override(const SkeletonPose& other);
    void Lerp(const SkeletonPose& other, float32 factor);

private:
    Vector<JointTransform> jointTransforms;
};

inline void SkeletonPose::SetJointCount(uint32 jointCount)
{
    jointTransforms.resize(jointCount, JointTransform());
}

inline uint32 SkeletonPose::GetJointsCount() const
{
    return uint32(jointTransforms.size());
}

inline void SkeletonPose::Reset()
{
    for (JointTransform& t : jointTransforms)
        t.Reset();
}

inline void SkeletonPose::SetTransform(uint32 jointIndex, const JointTransform& transform)
{
    if (GetJointsCount() <= jointIndex)
        SetJointCount(jointIndex + 1);

    jointTransforms[jointIndex] = transform;
}

inline void SkeletonPose::SetPosition(uint32 jointIndex, const Vector3& position)
{
    if (GetJointsCount() <= jointIndex)
        SetJointCount(jointIndex + 1);

    jointTransforms[jointIndex].SetPosition(position);
}

inline void SkeletonPose::SetOrientation(uint32 jointIndex, const Quaternion& orientation)
{
    if (GetJointsCount() <= jointIndex)
        SetJointCount(jointIndex + 1);

    jointTransforms[jointIndex].SetOrientation(orientation);
}

inline void SkeletonPose::SetScale(uint32 jointIndex, float32 scale)
{
    if (GetJointsCount() <= jointIndex)
        SetJointCount(jointIndex + 1);

    jointTransforms[jointIndex].SetScale(scale);
}

inline const JointTransform& SkeletonPose::GetJointTransform(uint32 jointIndex) const
{
    static JointTransform emptyTransform;

    if (jointIndex < GetJointsCount())
        return jointTransforms[jointIndex];
    else
        return emptyTransform;
}

} //ns