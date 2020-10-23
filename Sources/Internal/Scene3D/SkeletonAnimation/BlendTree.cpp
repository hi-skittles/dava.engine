#include "BlendTree.h"
#include "SkeletonPose.h"
#include "SkeletonAnimation.h"

#include "Animation/AnimationClip.h"
#include "Base/BaseMath.h"
#include "Base/BaseTypes.h"
#include "Base/GlobalEnum.h"
#include "FileSystem/YamlNode.h"

ENUM_DECLARE(DAVA::BlendTree::eNodeType)
{
    ENUM_ADD_DESCR(DAVA::BlendTree::eNodeType::TYPE_ANIMATION, "Animation");
    ENUM_ADD_DESCR(DAVA::BlendTree::eNodeType::TYPE_LERP_1D, "LERP1D");
    ENUM_ADD_DESCR(DAVA::BlendTree::eNodeType::TYPE_LERP_2D, "LERP2D");
    ENUM_ADD_DESCR(DAVA::BlendTree::eNodeType::TYPE_ADD, "Add");
    ENUM_ADD_DESCR(DAVA::BlendTree::eNodeType::TYPE_DIFF, "Diff");
};

namespace DAVA
{
BlendTree::~BlendTree()
{
    for (Animation& a : animations)
        SafeDelete(a.skeletonAnimation);
}

void BlendTree::BindSkeleton(const SkeletonComponent* skeleton)
{
    for (const Animation& a : animations)
        a.skeletonAnimation->BindSkeleton(skeleton);
}

void BlendTree::BindRootNode(const FastName& rootNodeID)
{
    for (const Animation& a : animations)
        a.skeletonAnimation->BindRootNode(rootNodeID);
}

void BlendTree::EvaluatePose(uint32 phaseIndex, float32 phase, const Vector<const float32*>& parameters, SkeletonPose* outPose) const
{
    EvaluateRecursive(phaseIndex, phase, 0, 0.f, nodes.front(), parameters, outPose, nullptr, nullptr);
}

float32 BlendTree::EvaluatePhaseDuration(uint32 phaseIndex, const Vector<const float32*>& parameters) const
{
    float32 duration = 0.f;
    EvaluateRecursive(phaseIndex, 0.f, 0, 0.f, nodes.front(), parameters, nullptr, &duration, nullptr);
    return duration;
}

void BlendTree::EvaluateRootOffset(uint32 phaseIndex0, float32 phase0, uint32 phaseIndex1, float32 phase1, const Vector<const float32*>& parameters, Vector3* outOffset)
{
    EvaluateRecursive(phaseIndex0, phase0, phaseIndex1, phase1, nodes.front(), parameters, nullptr, nullptr, outOffset);
}

float32 BlendTree::GetAnimationPhaseTime(const Animation& animation, uint32 phaseIndex) const
{
    if (animation.treatAsPose)
        return 0.f;

    float32 phaseStart = (phaseIndex > 0) ? animation.phaseEnds[phaseIndex - 1] : 0.f;
    float32 phaseEnd = animation.phaseEnds[phaseIndex];

    return (phaseEnd - phaseStart) * animation.skeletonAnimation->GetDuration();
}

float32 BlendTree::GetAnimationLocalTime(const Animation& animation, uint32 phaseIndex, float32 phase) const
{
    if (animation.treatAsPose)
        return 0.f;

    float32 phaseStart = (phaseIndex > 0) ? animation.phaseEnds[phaseIndex - 1] : 0.f;
    float32 phaseEnd = animation.phaseEnds[phaseIndex];

    return (phaseStart + phase * (phaseEnd - phaseStart)) * animation.skeletonAnimation->GetDuration();
}

void BlendTree::EvaluateRecursive(uint32 phaseIndex, float32 phase, uint32 phaseIndex1, float32 phase1, const BlendNode& node, const Vector<const float32*>& parameters, SkeletonPose* outPose, float32* outPhaseDuration, Vector3* outOffset) const
{
    switch (node.type)
    {
    case TYPE_ANIMATION:
    {
        int32 animationIndex = node.animData.index;
        if (animationIndex != -1)
        {
            DVASSERT(phaseIndex < phasesCount);

            const BlendTree::Animation& animation = animations[node.animData.index];

            if (outPose != nullptr)
            {
                animation.skeletonAnimation->EvaluatePose(GetAnimationLocalTime(animation, phaseIndex, phase), outPose);
            }

            if (outPhaseDuration != nullptr)
            {
                *outPhaseDuration = GetAnimationPhaseTime(animation, phaseIndex);
            }

            if (outOffset != nullptr)
            {
                float32 animationLocalTime0 = GetAnimationLocalTime(animation, phaseIndex, phase);
                float32 animationLocalTime1 = GetAnimationLocalTime(animation, phaseIndex1, phase1);
                animation.skeletonAnimation->EvaluateRootOffset(animationLocalTime0, animationLocalTime1, outOffset);
            }
        }
    }
    break;
    case TYPE_LERP_1D:
    {
        const BlendNode::BlendData& blendData = node.blendData;
        int32 childrenCount = blendData.endChildIndex - blendData.beginChildIndex;
        if (childrenCount == 1)
        {
            EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex], parameters, outPose, outPhaseDuration, outOffset);
        }
        else
        {
            float32 parameter = (parameters[blendData.parameterIndex] != nullptr) ? *parameters[blendData.parameterIndex] : 0.f;

            int32 c = blendData.beginChildIndex;
            for (; c < blendData.endChildIndex; ++c)
            {
                if (nodes[c].coord.x >= parameter)
                    break;
            }

            if (c == blendData.beginChildIndex)
            {
                EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex], parameters, outPose, outPhaseDuration, outOffset);
            }
            else if (c == blendData.endChildIndex)
            {
                EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.endChildIndex - 1], parameters, outPose, outPhaseDuration, outOffset);
            }
            else
            {
                const BlendNode& child0 = nodes[c - 1];
                const BlendNode& child1 = nodes[c];
                float32 coord0 = child0.coord.x;
                float32 coord1 = child1.coord.x;

                float32 factor = (parameter - coord0) / (coord1 - coord0);
                if (outPose != nullptr)
                {
                    SkeletonPose pose1;
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child0, parameters, outPose, nullptr, nullptr);
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child1, parameters, &pose1, nullptr, nullptr);
                    outPose->Lerp(pose1, factor);
                }

                if (outPhaseDuration != nullptr)
                {
                    float32 dur0, dur1;
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child0, parameters, nullptr, &dur0, nullptr);
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child1, parameters, nullptr, &dur1, nullptr);
                    *outPhaseDuration = Lerp(dur0, dur1, factor);
                }

                if (outOffset != nullptr)
                {
                    Vector3 offset0, offset1;
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child0, parameters, nullptr, nullptr, &offset0);
                    EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, child1, parameters, nullptr, nullptr, &offset1);
                    *outOffset = Lerp(offset0, offset1, factor);
                }
            }
        }
    }
    break;
    case TYPE_LERP_2D:
    {
        //TODO: *Skinning*
        DVASSERT(false);
    }
    break;

    case TYPE_ADD:
    case TYPE_DIFF:
    {
        const BlendNode::BlendData& blendData = node.blendData;
        DVASSERT((blendData.endChildIndex - blendData.beginChildIndex) == 2);
        if (outPose != nullptr)
        {
            SkeletonPose pose1;
            EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex], parameters, outPose, nullptr, nullptr);
            EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex + 1], parameters, &pose1, nullptr, nullptr);

            if (node.type == TYPE_ADD)
                outPose->Add(pose1);
            else if (node.type == TYPE_DIFF)
                outPose->Diff(pose1);
        }

        if (outPhaseDuration != nullptr)
        {
            float32 dur0, dur1;
            EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex], parameters, nullptr, &dur0, nullptr);
            EvaluateRecursive(phaseIndex, phase, phaseIndex1, phase1, nodes[blendData.beginChildIndex + 1], parameters, nullptr, &dur1, nullptr);

            *outPhaseDuration = Max(dur0, dur1);
        }
    }
    break;
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////

BlendTree* BlendTree::LoadFromYaml(const YamlNode* yamlNode)
{
    DVASSERT(yamlNode != nullptr);

    BlendTree* blendTree = new BlendTree();
    blendTree->phasesCount = 1;

    const YamlNode* phasesNode = yamlNode->Get("phases-count");
    if (phasesNode != nullptr && phasesNode->GetType() == YamlNode::TYPE_STRING)
        blendTree->phasesCount = phasesNode->AsUInt32();

    blendTree->nodes.emplace_back();
    blendTree->LoadBlendNodeRecursive(yamlNode, blendTree, 0);

    std::sort(blendTree->markers.begin(), blendTree->markers.end(), std::less<float32>());

    return blendTree;
}

void BlendTree::LoadIgnoreMask(const YamlNode* yamlNode, UnorderedSet<uint32>* ignoreMask)
{
    const YamlNode* ignoreMaskNode = yamlNode->Get("ignore-mask");
    if (ignoreMaskNode != nullptr && ignoreMaskNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        uint32 ignoreJointsCount = ignoreMaskNode->GetCount();
        for (uint32 ij = 0; ij < ignoreJointsCount; ++ij)
        {
            const YamlNode* ignoreJointNode = ignoreMaskNode->Get(ij);
            if (ignoreJointNode->GetType() == YamlNode::TYPE_STRING)
            {
                ignoreMask->insert(ignoreJointNode->AsUInt32());
            }
        }
    }
}

float32 BlendTree::GetClipTimestamp(const YamlNode* yamlNode) const
{
    DVASSERT(yamlNode != nullptr && yamlNode->GetType() == YamlNode::TYPE_STRING);
    const String& string = yamlNode->AsString();

    float32 result = 0.f, divisor = 0.f;
    if (sscanf(string.c_str(), "%f/%f", &result, &divisor) == 2)
        result /= divisor;

    return result;
}

float32 BlendTree::GetPhaseIDByTimestamp(const BlendTree::Animation& animation, float32 timestamp) const
{
    DVASSERT(!animation.phaseEnds.empty());

    auto found = std::find_if(animation.phaseEnds.begin(), animation.phaseEnds.end(), [&timestamp](float32 phaseEnd) {
        return phaseEnd > timestamp;
    });

    DVASSERT(found != animation.phaseEnds.end());

    uint32 phaseIndex = 0;
    float32 phaseValue = 0.f;

    if (found == animation.phaseEnds.begin())
    {
        phaseValue = timestamp / animation.phaseEnds[0];
    }
    else
    {
        float32 prevPhaseTimestamp = *(--found);
        DVASSERT(timestamp >= prevPhaseTimestamp);

        phaseIndex = uint32(std::distance(animation.phaseEnds.begin(), found));
        phaseValue = (timestamp - prevPhaseTimestamp) / (*found - prevPhaseTimestamp);
    }

    DVASSERT(phaseIndex < uint32(animation.phaseEnds.size()));
    DVASSERT(phaseValue >= 0.f && phaseValue < 1.f);

    return float32(phaseIndex) + phaseValue;
}

void BlendTree::LoadBlendNodeRecursive(const YamlNode* yamlNode, BlendTree* blendTree, uint32 nodeIndex, UnorderedSet<uint32> ignoreMask)
{
    Vector<BlendNode>& nodes = blendTree->nodes;
    BlendNode& node = nodes[nodeIndex];

    const YamlNode* paramNode = yamlNode->Get("param-value");
    if (paramNode != nullptr)
    {
        if (paramNode->GetType() == YamlNode::TYPE_ARRAY)
            node.coord = paramNode->AsVector2();
        else if (paramNode->GetType() == YamlNode::TYPE_STRING)
            node.coord.x = paramNode->AsFloat();
    }

    LoadIgnoreMask(yamlNode, &ignoreMask);

    Vector<const YamlNode*> animationNodes;
    {
        const YamlNode* clipNode = yamlNode->Get("clip");
        const YamlNode* animationsNode = yamlNode->Get("animation");

        if (clipNode != nullptr && clipNode->GetType() == YamlNode::TYPE_STRING)
        {
            DVASSERT(animationsNode == nullptr);

            animationNodes.push_back(yamlNode);
        }
        if (animationsNode != nullptr && animationsNode->GetType() == YamlNode::TYPE_ARRAY)
        {
            DVASSERT(clipNode == nullptr);

            uint32 animationCount = animationsNode->GetCount();
            for (uint32 ac = 0; ac < animationCount; ++ac)
            {
                animationNodes.push_back(animationsNode->Get(ac));
                DVASSERT(animationNodes.back()->Get("clip") != nullptr);
            }
        }
    }

    if (!animationNodes.empty())
    {
        node.type = eNodeType::TYPE_ANIMATION;
        node.animData.index = int32(animations.size());

        animations.emplace_back();
        BlendTree::Animation& animation = animations.back();
        animation.skeletonAnimation = new SkeletonAnimation();

        float32 syncPointsBase = 0.f;

        for (const YamlNode* animationNode : animationNodes)
        {
            const YamlNode* clipNode = animationNode->Get("clip");
            DVASSERT(clipNode != nullptr && clipNode->GetType() == YamlNode::TYPE_STRING);

            FilePath animationClipPath(clipNode->AsString());
            ScopedPtr<AnimationClip> animationClip(AnimationClip::Load(animationClipPath));
            DVASSERT(animationClip);

            UnorderedSet<uint32> clipIgnoreMask = ignoreMask;
            LoadIgnoreMask(animationNode, &clipIgnoreMask);

            float32 clipStart = 0.f;
            float32 clipEnd = animationClip->GetDuration();

            const YamlNode* rangeNode = animationNode->Get("range");
            if (rangeNode != nullptr && rangeNode->GetType() == YamlNode::TYPE_ARRAY)
            {
                DVASSERT(rangeNode->GetCount() == 2);

                clipStart = GetClipTimestamp(rangeNode->Get(0));
                clipEnd = GetClipTimestamp(rangeNode->Get(1));

                DVASSERT(clipEnd > clipStart);
            }

            animation.skeletonAnimation->AddAnimationClip(animationClip, clipIgnoreMask, clipStart, clipEnd);

            const YamlNode* treatAsPoseNode = animationNode->Get("treat-as-pose");
            if (treatAsPoseNode != nullptr && treatAsPoseNode->GetType() == YamlNode::TYPE_STRING)
            {
                animation.treatAsPose = treatAsPoseNode->AsBool();
            }

            const YamlNode* syncPointsNode = animationNode->Get("sync-points");
            if (syncPointsNode != nullptr && syncPointsNode->GetType() == YamlNode::TYPE_ARRAY)
            {
                uint32 pointsCount = syncPointsNode->GetCount();
                for (uint32 sp = 0; sp < pointsCount; ++sp)
                {
                    const YamlNode* syncPointNode = syncPointsNode->Get(sp);
                    if (syncPointNode->GetType() == YamlNode::TYPE_STRING)
                    {
                        float32 syncPointTimestamp = GetClipTimestamp(syncPointNode);
                        if (syncPointTimestamp > EPSILON)
                        {
                            syncPointTimestamp += syncPointsBase;
                            DVASSERT(animation.phaseEnds.empty() || animation.phaseEnds.back() < syncPointTimestamp);
                            animation.phaseEnds.push_back(syncPointTimestamp);
                        }
                    }
                }
            }

            syncPointsBase = animation.skeletonAnimation->GetDuration();

            if (animation.phaseEnds.empty() || (!animation.phaseEnds.empty() && !FLOAT_EQUAL(animation.phaseEnds.back(), syncPointsBase)))
                animation.phaseEnds.push_back(syncPointsBase);

            const YamlNode* markersNode = animationNode->Get("markers");
            if (markersNode != nullptr && markersNode->GetType() == YamlNode::TYPE_MAP)
            {
                uint32 markersCount = markersNode->GetCount();
                for (uint32 marker = 0; marker < markersCount; ++marker)
                {
                    MarkerInfo markerInfo;
                    markerInfo.markerID = FastName(markersNode->GetItemKeyName(marker).c_str());

                    Vector<float32> markersTimestamps;

                    const YamlNode* markerNode = markersNode->Get(marker);
                    if (markerNode->GetType() == YamlNode::TYPE_STRING)
                    {
                        markersTimestamps.emplace_back(GetClipTimestamp(markerNode));
                    }
                    else if (markerNode->GetType() == YamlNode::TYPE_ARRAY)
                    {
                        uint32 valuesCount = markerNode->GetCount();
                        for (uint32 value = 0; value < valuesCount; ++value)
                        {
                            markersTimestamps.emplace_back(GetClipTimestamp(markerNode->Get(value)));
                        }
                    }

                    for (float32 timestamp : markersTimestamps)
                    {
                        markerInfo.phaseID = GetPhaseIDByTimestamp(animation, timestamp);
                        markers.push_back(markerInfo);
                    }
                }
            }
        }

        for (float32& phaseEnd : animation.phaseEnds)
            phaseEnd /= syncPointsBase;

        DVASSERT(animation.phaseEnds.front() > EPSILON);
        DVASSERT(FLOAT_EQUAL(animation.phaseEnds.back(), 1.f));

        //All animation should have the same phases count
        //TODO: *Skinning* return error, not assert
        DVASSERT(phasesCount == uint32(animation.phaseEnds.size()) || animation.treatAsPose);
    }
    else
    {
        BlendNode::BlendData& blendData = node.blendData;
        blendData.parameterIndex = -1;
        blendData.beginChildIndex = -1;
        blendData.endChildIndex = -1;

        const YamlNode* operationNode = yamlNode->Get("operation");
        if (operationNode != nullptr && operationNode->GetType() == YamlNode::TYPE_MAP)
        {
            const YamlNode* typeNode = operationNode->Get("type");
            if (typeNode != nullptr && typeNode->GetType() == YamlNode::TYPE_STRING)
            {
                int32 nodeType;
                if (GlobalEnumMap<eNodeType>::Instance()->ToValue(typeNode->AsString().c_str(), nodeType))
                {
                    node.type = eNodeType(nodeType);

                    const YamlNode* parameterNode = operationNode->Get("parameter");
                    if (parameterNode != nullptr && parameterNode->GetType() == YamlNode::TYPE_STRING)
                    {
                        FastName parameterID = parameterNode->AsFastName();
                        auto found = std::find(parameterIDs.begin(), parameterIDs.end(), parameterID);

                        blendData.parameterIndex = int32(std::distance(parameterIDs.begin(), found));
                        if (found == parameterIDs.end())
                            parameterIDs.emplace_back(parameterID);
                    }

                    const YamlNode* childrenNode = yamlNode->Get("nodes");
                    if (childrenNode != nullptr && childrenNode->GetType() == YamlNode::TYPE_ARRAY)
                    {
                        LoadIgnoreMask(yamlNode, &ignoreMask);

                        uint32 childrenCount = childrenNode->GetCount();
                        uint32 childBegin = uint32(nodes.size());
                        uint32 childEnd = childBegin + childrenCount;

                        blendData.beginChildIndex = int32(childBegin);
                        blendData.endChildIndex = int32(childEnd);

                        //Hint: 'blendData' and 'node' should be filled at this point. After 'nodes.resize()' references is invalid.
                        nodes.resize(childEnd);
                        for (uint32 c = 0; c < childrenCount; ++c)
                        {
                            const YamlNode* childNode = childrenNode->Get(c);
                            LoadBlendNodeRecursive(childNode, blendTree, childBegin + c, ignoreMask);
                        }

                        if (nodes[nodeIndex].type == TYPE_LERP_1D)
                        {
                            std::sort(nodes.data() + childBegin, nodes.data() + childEnd, [](const BlendNode& l, const BlendNode& r) {
                                return l.coord.x < r.coord.x;
                            });
                        }
                    }
                }
            }
        }
    }
}

} //ns