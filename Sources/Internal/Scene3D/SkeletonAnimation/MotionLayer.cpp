#include "MotionLayer.h"

#include "BlendTree.h"
#include "Base/GlobalEnum.h"
#include "FileSystem/YamlNode.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

ENUM_DECLARE(DAVA::MotionLayer::eMotionBlend)
{
    ENUM_ADD_DESCR(DAVA::MotionLayer::eMotionBlend::BLEND_OVERRIDE, "Override");
    ENUM_ADD_DESCR(DAVA::MotionLayer::eMotionBlend::BLEND_ADD, "Add");
    ENUM_ADD_DESCR(DAVA::MotionLayer::eMotionBlend::BLEND_DIFF, "Diff");
};

namespace DAVA
{
DAVA_REFLECTION_IMPL(MotionLayer)
{
    ReflectionRegistrator<MotionLayer>::Begin()
    .Field("layerID", &MotionLayer::layerID)[M::ReadOnly()]
    .End();
}

void MotionLayer::TriggerEvent(const FastName& trigger)
{
    const Motion::TransitionInfo& transitionInfo = (nextMotion != nullptr && motionTransition.IsStarted()) ? nextMotion->GetTransitionInfo(trigger) : currentMotion->GetTransitionInfo(trigger);
    if (transitionInfo.info != nullptr && transitionInfo.motion != nullptr)
    {
        pendingTransition = transitionInfo.info;
        pendingMotion = transitionInfo.motion;
    }
}

void MotionLayer::Update(float32 dTime)
{
    if (pendingMotion != nullptr)
    {
        if (currentMotion != nullptr)
        {
            if (nextMotion != nullptr && motionTransition.IsStarted())
            {
                if (pendingTransition != nullptr && motionTransition.CanBeInterrupted(pendingTransition, nextMotion, pendingMotion))
                {
                    motionTransition.Interrupt(pendingTransition, nextMotion, pendingMotion);
                    currentMotion = nextMotion;
                    nextMotion = pendingMotion;

                    pendingMotion = nullptr;
                    pendingTransition = nullptr;
                }
            }
            else
            {
                if (pendingTransition != nullptr)
                {
                    motionTransition.Reset(pendingTransition, currentMotion, pendingMotion);
                    nextMotion = pendingMotion;
                    nextMotion->Reset();

                    pendingMotion = nullptr;
                    pendingTransition = nullptr;
                }
            }
        }
        else
        {
            currentMotion = pendingMotion;
            currentMotion->Reset();
        }
    }

    //////////////////////////////////////////////////////////////////////////

    if (nextMotion != nullptr) //transition is active
    {
        motionTransition.Update(dTime);
    }
    else
    {
        currentMotion->Update(dTime);
    }

    //////////////////////////////////////////////////////////////////////////

    reachedMarkers.clear();
    for (const FastName& m : currentMotion->GetReachedMarkers())
        reachedMarkers.emplace_back(currentMotion->GetID(), m);

    if (nextMotion != nullptr && motionTransition.IsStarted())
    {
        for (const FastName& m : nextMotion->GetReachedMarkers())
            reachedMarkers.emplace_back(nextMotion->GetID(), m);
    }

    endedMotions.clear();
    if (currentMotion->IsAnimationEndReached())
        endedMotions.emplace_back(currentMotion->GetID());

    //////////////////////////////////////////////////////////////////////////

    currentPose.Reset();
    if (nextMotion != nullptr) //transition is active
    {
        motionTransition.Evaluate(&currentPose, &currentRootOffsetDelta);
    }
    else
    {
        currentMotion->EvaluatePose(&currentPose);
        currentMotion->GetRootOffsetDelta(&currentRootOffsetDelta);
    }

    //////////////////////////////////////////////////////////////////////////

    if (nextMotion != nullptr) //transition is active
    {
        if (motionTransition.IsComplete())
        {
            currentMotion = nextMotion;
            nextMotion = nullptr;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    currentRootOffsetDelta *= rootExtractionMask;

    if (rootNodeJointIndex != SkeletonComponent::INVALID_JOINT_INDEX)
    {
        Vector3 rootPosition = currentPose.GetJointTransform(rootNodeJointIndex).GetPosition();
        rootPosition *= rootResetMask;
        currentPose.SetPosition(rootNodeJointIndex, rootPosition);
    }
}

void MotionLayer::BindSkeleton(const SkeletonComponent* skeleton)
{
    for (Motion& s : motions)
        s.BindSkeleton(skeleton);

    if (currentMotion != nullptr)
    {
        currentPose.Reset();
        currentMotion->Reset();
        currentMotion->EvaluatePose(&currentPose);
        currentMotion->GetRootOffsetDelta(&currentRootOffsetDelta);
    }

    rootNodeJointIndex = skeleton->GetJointIndex(rootNodeID);
}

bool MotionLayer::BindParameter(const FastName& parameterID, const float32* param)
{
    bool success = false;

    for (Motion& s : motions)
        success |= s.BindParameter(parameterID, param);

    return success;
}

bool MotionLayer::UnbindParameter(const FastName& parameterID)
{
    return BindParameter(parameterID, nullptr);
}

void MotionLayer::UnbindParameters()
{
    for (Motion& m : motions)
        m.UnbindParameters();
}

const Vector<FastName>& MotionLayer::GetMotionIDs() const
{
    return motionsIDs;
}

MotionLayer* MotionLayer::LoadFromYaml(const YamlNode* motionNode)
{
    MotionLayer* motionLayer = new MotionLayer();

    int32 enumValue;
    Set<FastName> motionsParameters;

    const YamlNode* layerIDNode = motionNode->Get("layer-id");
    if (layerIDNode != nullptr && layerIDNode->GetType() == YamlNode::TYPE_STRING)
    {
        motionLayer->layerID = layerIDNode->AsFastName();
    }

    const YamlNode* blendModeNode = motionNode->Get("blend-mode");
    if (blendModeNode != nullptr && blendModeNode->GetType() == YamlNode::TYPE_STRING)
    {
        if (GlobalEnumMap<MotionLayer::eMotionBlend>::Instance()->ToValue(blendModeNode->AsString().c_str(), enumValue))
            motionLayer->blendMode = eMotionBlend(enumValue);
    }

    FastName defaultMotionID;
    const YamlNode* defaultMotionNode = motionNode->Get("default-motion");
    if (defaultMotionNode != nullptr && defaultMotionNode->GetType() == YamlNode::TYPE_STRING)
    {
        defaultMotionID = defaultMotionNode->AsFastName();
    }

    const YamlNode* motionsNode = motionNode->Get("motions");
    if (motionsNode != nullptr && motionsNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        uint32 motionsCount = motionsNode->GetCount();
        motionLayer->motions.resize(motionsCount);
        motionLayer->motionsIDs.resize(motionsCount);
        for (uint32 m = 0; m < motionsCount; ++m)
        {
            Motion& motion = motionLayer->motions[m];
            motion.LoadFromYaml(motionsNode->Get(m));

            motionLayer->motionsIDs[m] = motion.GetID();

            const Vector<FastName>& blendTreeParams = motion.GetBlendTreeParameters();
            motionsParameters.insert(blendTreeParams.begin(), blendTreeParams.end());

            if (defaultMotionID == motion.GetID())
                motionLayer->currentMotion = motionLayer->motions.data() + m;
        }

        if (motionLayer->currentMotion == nullptr && motionsCount > 0)
            motionLayer->currentMotion = motionLayer->motions.data();
    }

    const YamlNode* transitionsNode = motionNode->Get("transitions");
    if (transitionsNode != nullptr && transitionsNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        uint32 transitionsCount = transitionsNode->GetCount();
        motionLayer->transitions.resize(transitionsCount);
        for (uint32 t = 0; t < transitionsCount; ++t)
        {
            const YamlNode* transitionNode = transitionsNode->Get(t);

            const YamlNode* srcNode = transitionNode->Get("src-motion");
            const YamlNode* dstNode = transitionNode->Get("dst-motion");
            const YamlNode* triggerNode = transitionNode->Get("trigger");
            if (srcNode != nullptr && srcNode->GetType() == YamlNode::TYPE_STRING &&
                dstNode != nullptr && dstNode->GetType() == YamlNode::TYPE_STRING &&
                triggerNode != nullptr && triggerNode->GetType() == YamlNode::TYPE_STRING)
            {
                uint32 srcPhase = std::numeric_limits<uint32>::max();
                const YamlNode* srcPhaseNode = transitionNode->Get("src-phase");
                if (srcPhaseNode != nullptr && srcPhaseNode->GetType() == YamlNode::TYPE_STRING)
                    srcPhase = srcPhaseNode->AsUInt32();

                FastName srcName = srcNode->AsFastName();
                FastName dstName = dstNode->AsFastName();
                FastName trigger = triggerNode->AsFastName();

                auto foundSrc = std::find_if(motionLayer->motions.begin(), motionLayer->motions.end(), [&srcName](const Motion& motion) {
                    return motion.GetID() == srcName;
                });

                auto foundDst = std::find_if(motionLayer->motions.begin(), motionLayer->motions.end(), [&dstName](const Motion& motion) {
                    return motion.GetID() == dstName;
                });

                if (foundSrc != motionLayer->motions.end() && foundDst != motionLayer->motions.end())
                {
                    motionLayer->transitions[t] = MotionTransitionInfo::LoadFromYaml(transitionNode);
                    foundSrc->AddTransition(trigger, &motionLayer->transitions[t], &(*foundDst), srcPhase);
                }
            }
        }
    }

    const YamlNode* rootTransformNode = motionNode->Get("root-transform");
    if (rootTransformNode != nullptr && rootTransformNode->GetType() == YamlNode::TYPE_MAP)
    {
        const YamlNode* rootIDNode = rootTransformNode->Get("root-node");
        if (rootIDNode != nullptr && rootIDNode->GetType() == YamlNode::TYPE_STRING)
        {
            FastName rootID = rootIDNode->AsFastName();
            for (Motion& motion : motionLayer->motions)
                motion.BindRootNode(rootID);

            motionLayer->rootNodeID = rootID;

            const YamlNode* extractPositionNode = nullptr;
            {
                extractPositionNode = rootTransformNode->Get("extract-position-x");
                if (extractPositionNode != nullptr && extractPositionNode->GetType() == YamlNode::TYPE_STRING && extractPositionNode->AsBool() == true)
                    motionLayer->rootExtractionMask.x = 1.f;

                extractPositionNode = rootTransformNode->Get("extract-position-y");
                if (extractPositionNode != nullptr && extractPositionNode->GetType() == YamlNode::TYPE_STRING && extractPositionNode->AsBool() == true)
                    motionLayer->rootExtractionMask.y = 1.f;

                extractPositionNode = rootTransformNode->Get("extract-position-z");
                if (extractPositionNode != nullptr && extractPositionNode->GetType() == YamlNode::TYPE_STRING && extractPositionNode->AsBool() == true)
                    motionLayer->rootExtractionMask.z = 1.f;
            }

            const YamlNode* resetPositionNode = nullptr;
            {
                resetPositionNode = rootTransformNode->Get("reset-position-x");
                if (resetPositionNode != nullptr && resetPositionNode->GetType() == YamlNode::TYPE_STRING && resetPositionNode->AsBool() == true)
                    motionLayer->rootResetMask.x = 0.f;

                resetPositionNode = rootTransformNode->Get("reset-position-y");
                if (resetPositionNode != nullptr && resetPositionNode->GetType() == YamlNode::TYPE_STRING && resetPositionNode->AsBool() == true)
                    motionLayer->rootResetMask.y = 0.f;

                resetPositionNode = rootTransformNode->Get("reset-position-z");
                if (resetPositionNode != nullptr && resetPositionNode->GetType() == YamlNode::TYPE_STRING && resetPositionNode->AsBool() == true)
                    motionLayer->rootResetMask.z = 0.f;
            }
        }
    }

    motionLayer->parameterIDs.insert(motionLayer->parameterIDs.begin(), motionsParameters.begin(), motionsParameters.end());

    return motionLayer;
}

} //ns