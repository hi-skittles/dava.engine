#pragma once

#include "Base/AllocatorFactory.h"
#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/Hash.h"
#include "Base/UnordererMap.h"

#include "MotionTransition.h"

namespace DAVA
{
class BlendTree;
class SkeletonComponent;
class SkeletonPose;
class YamlNode;
struct MotionTransitionInfo;

class Motion
{
public:
    Motion() = default;
    ~Motion();

    struct TransitionInfo
    {
        MotionTransitionInfo* info;
        Motion* motion;
    };

    void LoadFromYaml(const YamlNode* motionNode);

    void Reset();
    void Update(float32 dTime);
    void EvaluatePose(SkeletonPose* outPose) const;
    void GetRootOffsetDelta(Vector3* offset) const;
    void SyncPhase(const Motion* withOther, const MotionTransitionInfo* transitionInfo);

    bool IsAnimationEndReached() const;
    bool IsPhaseEndReached(uint32 phaseIndex) const;
    bool IsMarkerReached(const FastName& marker) const;

    const Vector<FastName>& GetReachedMarkers() const;

    const FastName& GetID() const;
    const Vector<FastName>& GetBlendTreeParameters() const;

    void BindSkeleton(const SkeletonComponent* skeleton);
    void BindRootNode(const FastName& rootNodeID);

    bool BindParameter(const FastName& parameterID, const float32* param);
    void UnbindParameters();

    void AddTransition(const FastName& trigger, MotionTransitionInfo* transitionInfo, Motion* dstMotion, uint32 srcPhase = std::numeric_limits<uint32>::max());
    const TransitionInfo& GetTransitionInfo(const FastName& trigger) const;

protected:
    struct TransitionKey
    {
        TransitionKey(const FastName& _trigger, uint32 _phase = std::numeric_limits<uint32>::max())
            : trigger(_trigger)
            , phase(_phase)
        {
        }

        inline bool operator==(const TransitionKey& other) const
        {
            return trigger == other.trigger && phase == other.phase;
        }

        FastName trigger;
        uint32 phase = std::numeric_limits<uint32>::max();
    };

    struct TransitionKeyHash
    {
        std::size_t operator()(const TransitionKey& key) const
        {
            std::size_t seed = 0;
            HashCombine(seed, key.trigger);
            HashCombine(seed, key.phase);
            return seed;
        }
    };

    FastName id;
    BlendTree* blendTree = nullptr;

    Vector<const float32*> boundParams;

    Vector<FastName> reachedMarkers;
    UnorderedSet<FastName> reachedMarkersSet;
    UnorderedMap<TransitionKey, TransitionInfo, TransitionKeyHash> transitions;

    Vector3 rootOffset;
    uint32 animationCurrPhaseIndex = 0u;
    uint32 animationPrevPhaseIndex = 0u;
    float32 animationPhase = 0.f;
    bool animationEndReached = false;
};

inline const FastName& Motion::GetID() const
{
    return id;
}

inline const Vector<FastName>& Motion::GetReachedMarkers() const
{
    return reachedMarkers;
}

inline bool Motion::IsAnimationEndReached() const
{
    return animationEndReached;
}

inline bool Motion::IsPhaseEndReached(uint32 phaseIndex) const
{
    return (animationPrevPhaseIndex == phaseIndex) && (animationCurrPhaseIndex != phaseIndex);
}

inline bool Motion::IsMarkerReached(const FastName& marker) const
{
    return reachedMarkersSet.count(marker) > 0;
}

} //ns
