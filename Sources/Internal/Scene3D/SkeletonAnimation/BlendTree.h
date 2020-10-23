#pragma once

#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"

namespace DAVA
{
class SkeletonAnimation;
class SkeletonComponent;
class SkeletonPose;
class YamlNode;
class BlendTree
{
    BlendTree() = default;

public:
    enum eNodeType : uint8
    {
        TYPE_ANIMATION,
        TYPE_LERP_1D,
        TYPE_LERP_2D,
        TYPE_ADD,
        TYPE_DIFF,

        TYPE_COUNT
    };

    struct MarkerInfo
    {
        float32 phaseID = 0.f; //integer part - phase-index, fractional part - phase-value
        FastName markerID;

        operator float32() const
        {
            return phaseID;
        }
    };

    ~BlendTree();
    static BlendTree* LoadFromYaml(const YamlNode* yamlNode);

    void BindSkeleton(const SkeletonComponent* skeleton);
    void BindRootNode(const FastName& rootNodeID);

    void EvaluatePose(uint32 phaseIndex, float32 phase, const Vector<const float32*>& parameters, SkeletonPose* outPose) const;
    float32 EvaluatePhaseDuration(uint32 phaseIndex, const Vector<const float32*>& parameters) const;
    void EvaluateRootOffset(uint32 phaseIndex0, float32 phase0, uint32 phaseIndex1, float32 phase1, const Vector<const float32*>& parameters, Vector3* outOffset);

    const Vector<FastName>& GetParameterIDs() const;
    const Vector<MarkerInfo>& GetMarkersInfo() const;

    uint32 GetPhasesCount() const;

protected:
    struct Animation
    {
        SkeletonAnimation* skeletonAnimation = nullptr;
        Vector<float32> phaseEnds;
        bool treatAsPose = false;
    };

    struct BlendNode
    {
        struct BlendData
        {
            int32 beginChildIndex;
            int32 endChildIndex;
            int32 parameterIndex;
        };
        struct AnimationData
        {
            int32 index;
        };

        union
        {
            BlendData blendData;
            AnimationData animData;
        };

        Vector2 coord;
        eNodeType type = TYPE_COUNT;
    };

    void LoadIgnoreMask(const YamlNode* yamlNode, UnorderedSet<uint32>* ignoreMask);
    float32 GetClipTimestamp(const YamlNode* yamlNode) const;
    float32 GetPhaseIDByTimestamp(const BlendTree::Animation& animation, float32 timestamp) const;
    void LoadBlendNodeRecursive(const YamlNode* yamlNode, BlendTree* blendTree, uint32 nodeIndex, UnorderedSet<uint32> ignoreMask = UnorderedSet<uint32>() /*should be by value*/);
    void EvaluateRecursive(uint32 phaseIndex, float32 phase, uint32 phaseIndex1, float32 phase1, const BlendNode& node, const Vector<const float32*>& parameters, SkeletonPose* outPose, float32* outPhaseDuration, Vector3* outOffset) const;

    float32 GetAnimationPhaseTime(const Animation& animation, uint32 phaseIndex) const;
    float32 GetAnimationLocalTime(const Animation& animation, uint32 phaseIndex, float32 phase) const;

    Vector<BlendNode> nodes;
    Vector<Animation> animations;
    Vector<MarkerInfo> markers;
    Vector<FastName> parameterIDs;
    uint32 phasesCount = 0;
};

inline const Vector<FastName>& BlendTree::GetParameterIDs() const
{
    return parameterIDs;
}

inline const Vector<BlendTree::MarkerInfo>& BlendTree::GetMarkersInfo() const
{
    return markers;
}

inline uint32 BlendTree::GetPhasesCount() const
{
    return phasesCount;
}

} //ns