#include "Motion.h"
#include "MotionTransition.h"

#include "Base/GlobalEnum.h"
#include "Base/UnordererMap.h"
#include "FileSystem/YamlNode.h"
#include "Scene3D/SkeletonAnimation/MotionLayer.h"
#include "Scene3D/SkeletonAnimation/SkeletonPose.h"

ENUM_DECLARE(DAVA::MotionTransitionInfo::eType)
{
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eType::TYPE_REPLACE, "replace");
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eType::TYPE_CROSS_FADE, "cross-fade");
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eType::TYPE_FROZEN_FADE, "frozen-fade");
};

ENUM_DECLARE(DAVA::MotionTransitionInfo::eSync)
{
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eSync::SYNC_IMMIDIATE, "immidiate");
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eSync::SYNC_WAIT_END, "wait-end");
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eSync::SYNC_WAIT_PHASE_END, "wait-phase-end");
    ENUM_ADD_DESCR(DAVA::MotionTransitionInfo::eSync::SYNC_WAIT_MARKER, "wait-marker");
};

namespace DAVA
{
void MotionTransition::Reset(const MotionTransitionInfo* _transitionInfo, Motion* _srcMotion, Motion* _dstMotion)
{
    DVASSERT(_transitionInfo != nullptr);
    DVASSERT(_srcMotion != nullptr);
    DVASSERT(_dstMotion != nullptr);

    transitionInfo = _transitionInfo;

    srcMotion = _srcMotion;
    dstMotion = _dstMotion;

    frozenPose.Reset();
    transitionPhase = 0.f;
    srcFrozen = false;
    inversed = false;
    started = false;
}

void MotionTransition::Update(float32 dTime)
{
    if (IsComplete())
        return;

    DVASSERT(srcMotion != nullptr && dstMotion != nullptr);

    if (!started)
    {
        srcMotion->Update(dTime);

        switch (transitionInfo->sync)
        {
        case MotionTransitionInfo::SYNC_WAIT_END:
            started = srcMotion->IsAnimationEndReached();
            break;
        case MotionTransitionInfo::SYNC_WAIT_PHASE_END:
            started = srcMotion->IsPhaseEndReached(transitionInfo->phaseToWait);
            break;
        case MotionTransitionInfo::SYNC_WAIT_MARKER:
            started = srcMotion->IsMarkerReached(transitionInfo->markerToWait);
            break;
        default:
            started = true;
            break;
        }

        if (started)
        {
            dstMotion->SyncPhase(srcMotion, transitionInfo);
            dstMotion->Update(dTime);

            if (transitionInfo->type == MotionTransitionInfo::TYPE_FROZEN_FADE)
            {
                frozenPose.Reset();
                srcMotion->EvaluatePose(&frozenPose);
                srcMotion->GetRootOffsetDelta(&frozenOffset);
                srcFrozen = true;
            }
        }
    }
    else
    {
        if (!srcFrozen)
        {
            srcMotion->Update(dTime);
        }

        dstMotion->Update(dTime);
    }

    if (IsStarted() && !IsComplete())
    {
        transitionPhase += dTime / transitionInfo->duration;
    }
}

void MotionTransition::Evaluate(SkeletonPose* outPose, Vector3* outOffset)
{
    if (IsStarted())
    {
        if (transitionInfo->type == MotionTransitionInfo::TYPE_REPLACE)
        {
            dstMotion->EvaluatePose(outPose);
            dstMotion->GetRootOffsetDelta(outOffset);
        }
        else
        {
            if (srcFrozen)
            {
                *outPose = frozenPose;
                *outOffset = frozenOffset;
            }
            else
            {
                srcMotion->EvaluatePose(outPose);
                srcMotion->GetRootOffsetDelta(outOffset);
            }

            workPose.Reset();
            dstMotion->EvaluatePose(&workPose);

            Vector3 dstOffset;
            dstMotion->GetRootOffsetDelta(&dstOffset);

            Interpolation::Func func = transitionInfo->func;
            float32 lerpFactor = inversed ? (1.f - func(1.f - transitionPhase)) : func(transitionPhase);

            outPose->Lerp(workPose, lerpFactor);
            outOffset->Lerp(*outOffset, dstOffset, lerpFactor);
        }
    }
    else
    {
        srcMotion->EvaluatePose(outPose);
        srcMotion->GetRootOffsetDelta(outOffset);
    }
}

bool MotionTransition::CanBeInterrupted(const MotionTransitionInfo* other, const Motion* newSrcMotion, const Motion* newDstMotion) const
{
    DVASSERT(other != nullptr);
    DVASSERT(IsStarted());

    return (other->type == MotionTransitionInfo::TYPE_FROZEN_FADE) || (srcMotion == newDstMotion && dstMotion == newSrcMotion);
}

void MotionTransition::Interrupt(const MotionTransitionInfo* other, Motion* newSrcMotion, Motion* newDstMotion)
{
    DVASSERT(CanBeInterrupted(other, newSrcMotion, newDstMotion));

    if (other->type == MotionTransitionInfo::TYPE_FROZEN_FADE)
    {
        Evaluate(&frozenPose, &frozenOffset);
        srcFrozen = true;
    }
    else
    {
        DVASSERT(srcMotion == newDstMotion);
        DVASSERT(dstMotion == newSrcMotion);

        transitionPhase = 1.f - transitionPhase;
        inversed = !inversed;
    }

    transitionInfo = other;
    srcMotion = newSrcMotion;
    dstMotion = newDstMotion;
}

//////////////////////////////////////////////////////////////////////////

MotionTransitionInfo MotionTransitionInfo::LoadFromYaml(const YamlNode* transitionNode)
{
    DVASSERT(transitionNode);

    MotionTransitionInfo transition;

    const YamlNode* durationNode = transitionNode->Get("duration");
    if (durationNode != nullptr && durationNode->GetType() == YamlNode::TYPE_STRING)
        transition.duration = durationNode->AsFloat();

    int32 workEnumValue = -1;

    const YamlNode* typeNode = transitionNode->Get("type");
    if (typeNode != nullptr && typeNode->GetType() == YamlNode::TYPE_STRING)
    {
        if (GlobalEnumMap<MotionTransitionInfo::eType>::Instance()->ToValue(typeNode->AsString().c_str(), workEnumValue))
            transition.type = eType(workEnumValue);
    }

    const YamlNode* funcNode = transitionNode->Get("func");
    if (funcNode != nullptr && funcNode->GetType() == YamlNode::TYPE_STRING)
    {
        if (GlobalEnumMap<Interpolation::FuncType>::Instance()->ToValue(funcNode->AsString().c_str(), workEnumValue))
            transition.func = Interpolation::GetFunction(Interpolation::FuncType(workEnumValue));
        else
            transition.func = Interpolation::GetFunction(Interpolation::LINEAR);
    }

    const YamlNode* syncNode = transitionNode->Get("sync");
    if (syncNode != nullptr && syncNode->GetType() == YamlNode::TYPE_STRING)
    {
        if (GlobalEnumMap<MotionTransitionInfo::eSync>::Instance()->ToValue(syncNode->AsString().c_str(), workEnumValue))
            transition.sync = eSync(workEnumValue);
    }

    const YamlNode* waitMarkerNode = transitionNode->Get("wait-marker");
    if (waitMarkerNode != nullptr && waitMarkerNode->GetType() == YamlNode::TYPE_STRING)
    {
        transition.markerToWait = waitMarkerNode->AsFastName();
    }

    const YamlNode* waitPhaseNode = transitionNode->Get("wait-phase");
    if (waitPhaseNode != nullptr && waitPhaseNode->GetType() == YamlNode::TYPE_STRING)
    {
        transition.phaseToWait = waitPhaseNode->AsUInt32();
    }

    const YamlNode* syncPhaseNode = transitionNode->Get("sync-phase");
    if (syncPhaseNode != nullptr && syncPhaseNode->GetType() == YamlNode::TYPE_STRING)
    {
        transition.syncPhase = syncPhaseNode->AsBool();
    }

    const YamlNode* inversePhaseNode = transitionNode->Get("inverse-phase");
    if (inversePhaseNode != nullptr && inversePhaseNode->GetType() == YamlNode::TYPE_STRING)
    {
        transition.inversePhase = inversePhaseNode->AsBool();
    }

    const YamlNode* phaseMapNode = transitionNode->Get("phase-map");
    if (phaseMapNode != nullptr && phaseMapNode->GetType() == YamlNode::TYPE_ARRAY)
    {
        Map<uint32, uint32> phaseMap;

        uint32 from, to;
        uint32 childrenCount = phaseMapNode->GetCount();
        for (uint32 c = 0; c < childrenCount; ++c)
        {
            const String& str = phaseMapNode->Get(c)->AsString();

            if (sscanf(str.c_str(), "%u -> %u", &from, &to) == 2)
                phaseMap[from] = to;
        }

        if (!phaseMap.empty())
        {
            uint32 phaseCount = phaseMap.rbegin()->first + 1;
            transition.phaseMap.resize(phaseCount);

            for (auto& it : phaseMap)
                transition.phaseMap[it.first] = it.second;
        }
    }

    return transition;
}

} //ns