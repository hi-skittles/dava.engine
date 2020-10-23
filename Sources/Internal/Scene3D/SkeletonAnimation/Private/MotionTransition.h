#pragma once

#include "Base/AllocatorFactory.h"
#include "Animation/Interpolation.h"
#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"

#include "Scene3D/SkeletonAnimation/SkeletonPose.h"

namespace DAVA
{
class Motion;
class SkeletonPose;
class YamlNode;

struct MotionTransitionInfo
{
public:
    enum eType : uint8
    {
        TYPE_REPLACE,
        TYPE_CROSS_FADE,
        TYPE_FROZEN_FADE,

        TYPE_COUNT
    };

    enum eSync : uint8
    {
        SYNC_IMMIDIATE,
        SYNC_WAIT_END,
        SYNC_WAIT_PHASE_END,
        SYNC_WAIT_MARKER,

        SYNC_COUNT
    };

    static MotionTransitionInfo LoadFromYaml(const YamlNode* transitionNode);

    eType type = TYPE_REPLACE;
    eSync sync = SYNC_IMMIDIATE;
    Interpolation::Func func;

    Vector<uint32> phaseMap;
    FastName markerToWait;
    uint32 phaseToWait = std::numeric_limits<uint32>::max();
    float32 duration = 0.f;
    bool syncPhase = false;
    bool inversePhase = false;
};

class MotionTransition
{
public:
    MotionTransition() = default;

    void Reset(const MotionTransitionInfo* transitionInfo, Motion* srcMotion, Motion* dstMotion);

    void Update(float32 dTime);
    void Evaluate(SkeletonPose* outPose, Vector3* outOffset);

    bool IsComplete() const;
    bool IsStarted() const;

    bool CanBeInterrupted(const MotionTransitionInfo* other, const Motion* srcMotion, const Motion* dstMotion) const;
    void Interrupt(const MotionTransitionInfo* other, Motion* srcMotion, Motion* dstMotion);

protected:
    const MotionTransitionInfo* transitionInfo = nullptr;

    Motion* srcMotion = nullptr;
    Motion* dstMotion = nullptr;

    SkeletonPose workPose;
    SkeletonPose frozenPose;
    Vector3 frozenOffset;
    float32 transitionPhase = 0.f;

    bool srcFrozen = false;
    bool inversed = false;
    bool started = false;
};

inline bool MotionTransition::IsStarted() const
{
    return started;
}

inline bool MotionTransition::IsComplete() const
{
    DVASSERT(transitionInfo != nullptr);
    return IsStarted() && ((transitionPhase >= 1.f) || (transitionInfo->duration < EPSILON) || (transitionInfo->type == MotionTransitionInfo::TYPE_REPLACE));
}

} //ns