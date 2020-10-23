#include "Motion.h"
#include "MotionTransition.h"

#include "FileSystem/YamlNode.h"
#include "Scene3D/SkeletonAnimation/BlendTree.h"

namespace DAVA
{
Motion::~Motion()
{
    SafeDelete(blendTree);
}

void Motion::Reset()
{
    rootOffset = Vector3();
    animationPrevPhaseIndex = 0;
    animationCurrPhaseIndex = 0;
    animationPhase = 0.f;
}

void Motion::Update(float32 dTime)
{
    if (blendTree == nullptr)
        return;

    animationEndReached = false;

    animationPrevPhaseIndex = animationCurrPhaseIndex;

    float32 animationPhase0 = animationPhase;
    uint32 animationCurrPhaseIndex0 = animationCurrPhaseIndex;

    float32 duration = blendTree->EvaluatePhaseDuration(animationCurrPhaseIndex, boundParams);
    animationPhase += (duration != 0.f) ? (dTime / duration) : 0.f;
    if (animationPhase >= 1.f)
    {
        animationPhase -= 1.f;

        ++animationCurrPhaseIndex;
        if (animationCurrPhaseIndex == blendTree->GetPhasesCount())
        {
            animationCurrPhaseIndex = 0;
            animationEndReached = true;
        }

        float32 nextPhaseDuration = blendTree->EvaluatePhaseDuration(animationCurrPhaseIndex, boundParams);
        animationPhase = (nextPhaseDuration != 0.f) ? (animationPhase * duration / nextPhaseDuration) : animationPhase;
        animationPhase = Clamp(animationPhase, 0.f, 1.f);
    }

    blendTree->EvaluateRootOffset(animationCurrPhaseIndex0, animationPhase0, animationCurrPhaseIndex, animationPhase, boundParams, &rootOffset);

    reachedMarkers.clear();
    reachedMarkersSet.clear();

    const Vector<BlendTree::MarkerInfo>& markersInfo = blendTree->GetMarkersInfo();
    if (!markersInfo.empty())
    {
        float32 phaseID0 = float32(animationCurrPhaseIndex0) + animationPhase0;
        float32 phaseID1 = float32(animationCurrPhaseIndex) + animationPhase;

        Vector<BlendTree::MarkerInfo>::const_iterator markerLeft = markersInfo.end(), markerRight = markersInfo.begin();
        for (Vector<BlendTree::MarkerInfo>::const_iterator it = markersInfo.begin(); it != markersInfo.end(); ++it)
        {
            if (markerLeft == markersInfo.end() && it->phaseID >= phaseID0)
                markerLeft = it;

            if (it->phaseID < phaseID1)
                markerRight = it;
            else
                break;
        }

        if (markerRight >= markerLeft && markerLeft != markersInfo.end() && markerRight != markersInfo.end())
        {
            auto itEnd = ++markerRight;
            for (auto it = markerLeft; it != itEnd; ++it)
            {
                const FastName& markerID = it->markerID;
                if (!markerID.empty())
                {
                    reachedMarkers.push_back(markerID);
                    reachedMarkersSet.insert(markerID);
                }
            }
        }
    }
}

void Motion::EvaluatePose(SkeletonPose* outPose) const
{
    if (blendTree != nullptr)
        blendTree->EvaluatePose(animationCurrPhaseIndex, animationPhase, boundParams, outPose);
}

void Motion::GetRootOffsetDelta(Vector3* offset) const
{
    *offset = rootOffset;
}

void Motion::SyncPhase(const Motion* other, const MotionTransitionInfo* transitionInfo)
{
    DVASSERT(transitionInfo != nullptr);

    const Vector<uint32>& phaseMap = transitionInfo->phaseMap;
    if (!phaseMap.empty())
    {
        animationPrevPhaseIndex = (other->animationPrevPhaseIndex < phaseMap.size()) ? phaseMap[other->animationPrevPhaseIndex] : animationPrevPhaseIndex;
        animationCurrPhaseIndex = (other->animationCurrPhaseIndex < phaseMap.size()) ? phaseMap[other->animationCurrPhaseIndex] : animationCurrPhaseIndex;

        DVASSERT(animationPrevPhaseIndex < blendTree->GetPhasesCount());
        DVASSERT(animationCurrPhaseIndex < blendTree->GetPhasesCount());
    }

    if (transitionInfo->syncPhase)
    {
        animationPhase = other->animationPhase;
    }

    if (transitionInfo->inversePhase)
    {
        animationPhase = 1.f - animationPhase;
    }
}

const Vector<FastName>& Motion::GetBlendTreeParameters() const
{
    static Vector<FastName> empty;
    return (blendTree != nullptr) ? blendTree->GetParameterIDs() : empty;
}

void Motion::BindSkeleton(const SkeletonComponent* skeleton)
{
    if (blendTree != nullptr)
        blendTree->BindSkeleton(skeleton);
}

void Motion::BindRootNode(const FastName& rootNodeID)
{
    if (blendTree != nullptr)
        blendTree->BindRootNode(rootNodeID);
}

bool Motion::BindParameter(const FastName& parameterID, const float32* param)
{
    bool success = false;
    if (blendTree != nullptr)
    {
        const Vector<FastName>& params = blendTree->GetParameterIDs();
        auto found = std::find(params.begin(), params.end(), parameterID);
        if (found != params.end())
        {
            size_t paramIndex = std::distance(params.begin(), found);
            boundParams[paramIndex] = param;
            success = true;
        }
    }

    return success;
}

void Motion::UnbindParameters()
{
    std::fill(boundParams.begin(), boundParams.end(), nullptr);
}

void Motion::AddTransition(const FastName& trigger, MotionTransitionInfo* transitionInfo, Motion* dstMotion, uint32 srcPhase)
{
    DVASSERT(transitionInfo != nullptr);
    DVASSERT(dstMotion != nullptr);

    TransitionKey key(trigger, srcPhase);
    DVASSERT(transitions.count(key) == 0);

    transitions[key] = { transitionInfo, dstMotion };
}

const Motion::TransitionInfo& Motion::GetTransitionInfo(const FastName& trigger) const
{
    auto found = transitions.find(TransitionKey(trigger, animationCurrPhaseIndex));
    if (found != transitions.end())
        return found->second;

    found = transitions.find(TransitionKey(trigger));
    if (found != transitions.end())
        return found->second;

    static TransitionInfo emptyInfo = { nullptr, nullptr };
    return emptyInfo;
}

void Motion::LoadFromYaml(const YamlNode* motionNode)
{
    DVASSERT(motionNode);

    const YamlNode* motionIDNode = motionNode->Get("motion-id");
    if (motionIDNode != nullptr && motionIDNode->GetType() == YamlNode::TYPE_STRING)
    {
        id = motionIDNode->AsFastName();
    }

    const YamlNode* blendTreeNode = motionNode->Get("blend-tree");
    if (blendTreeNode != nullptr)
    {
        blendTree = BlendTree::LoadFromYaml(blendTreeNode);
        DVASSERT(blendTree != nullptr);

        boundParams.resize(blendTree->GetParameterIDs().size());
    }
}

} //ns