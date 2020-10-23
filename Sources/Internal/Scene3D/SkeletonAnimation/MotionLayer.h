#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/UnordererMap.h"
#include "Reflection/Reflection.h"
#include "Scene3D/SkeletonAnimation/SkeletonPose.h"
#include "Scene3D/Components/SkeletonComponent.h"

#include "Private/Motion.h"
#include "Private/MotionTransition.h"

namespace DAVA
{
class BlendTree;
class YamlNode;
class MotionLayer;
class Motion;
struct MotionTransitionInfo;

class MotionLayer
{
    MotionLayer() = default;

public:
    enum eMotionBlend
    {
        BLEND_OVERRIDE,
        BLEND_ADD,
        BLEND_DIFF,

        BLEND_COUNT
    };

    static MotionLayer* LoadFromYaml(const YamlNode* motionNode);

    const FastName& GetName() const;
    eMotionBlend GetBlendMode() const;
    const SkeletonPose& GetCurrentSkeletonPose() const;
    const Vector3& GetCurrentRootOffsetDelta() const;

    void TriggerEvent(const FastName& trigger); //TODO: *Skinning* make adequate naming

    void Update(float32 dTime);

    void BindSkeleton(const SkeletonComponent* skeleton);

    const Vector<FastName>& GetParameterIDs() const;
    bool BindParameter(const FastName& parameterID, const float32* param);
    bool UnbindParameter(const FastName& parameterID);
    void UnbindParameters();

    const Vector<FastName>& GetMotionIDs() const;
    const Vector<std::pair<FastName, FastName>>& GetReachedMarkers() const;
    const Vector<FastName>& GetEndedMotions() const;

protected:
    FastName layerID;
    eMotionBlend blendMode = BLEND_COUNT;

    Vector<Motion> motions;
    Vector<MotionTransitionInfo> transitions;

    Vector<FastName> motionsIDs;
    Vector<FastName> parameterIDs;

    MotionTransition motionTransition; //transition from 'current' to 'next' motion
    Motion* currentMotion = nullptr;
    Motion* nextMotion = nullptr;
    Motion* pendingMotion = nullptr;
    MotionTransitionInfo* pendingTransition = nullptr;

    Vector3 currentRootOffsetDelta;
    Vector3 rootExtractionMask = Vector3(0.f, 0.f, 0.f);
    Vector3 rootResetMask = Vector3(1.f, 1.f, 1.f);

    FastName rootNodeID;
    uint32 rootNodeJointIndex = SkeletonComponent::INVALID_JOINT_INDEX;

    SkeletonPose currentPose;
    Vector<std::pair<FastName, FastName>> reachedMarkers; /*[motion-id, phase-id]*/
    Vector<FastName> endedMotions;

    DAVA_REFLECTION(MotionLayer);
};

inline const FastName& MotionLayer::GetName() const
{
    return layerID;
}

inline MotionLayer::eMotionBlend MotionLayer::GetBlendMode() const
{
    return blendMode;
}

inline const SkeletonPose& MotionLayer::GetCurrentSkeletonPose() const
{
    return currentPose;
}

inline const Vector3& MotionLayer::GetCurrentRootOffsetDelta() const
{
    return currentRootOffsetDelta;
}

inline const Vector<FastName>& MotionLayer::GetParameterIDs() const
{
    return parameterIDs;
}

inline const Vector<std::pair<FastName, FastName>>& MotionLayer::GetReachedMarkers() const
{
    return reachedMarkers;
}

inline const Vector<FastName>& MotionLayer::GetEndedMotions() const
{
    return endedMotions;
}

} //ns