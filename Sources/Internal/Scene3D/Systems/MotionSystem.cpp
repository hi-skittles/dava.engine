#include "MotionSystem.h"

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/MotionComponent.h"
#include "Scene3D/Components/SingleComponents/MotionSingleComponent.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/SkeletonAnimation/MotionLayer.h"
#include "Scene3D/SkeletonAnimation/SimpleMotion.h"

namespace DAVA
{
MotionSystem::MotionSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

MotionSystem::~MotionSystem()
{
    GetScene()->GetEventSystem()->UnregisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

void MotionSystem::SetScene(Scene* scene)
{
    motionSingleComponent = (scene != nullptr) ? scene->motionSingleComponent : nullptr;
}

void MotionSystem::AddEntity(Entity* entity)
{
    DVASSERT(motionSingleComponent != nullptr);

    MotionComponent* motionComponent = GetMotionComponent(entity);
    motionSingleComponent->reloadDescriptor.push_back(motionComponent);
}

void MotionSystem::RemoveEntity(Entity* entity)
{
    FindAndRemoveExchangingWithLast(activeComponents, GetMotionComponent(entity));
}

void MotionSystem::PrepareForRemove()
{
    activeComponents.clear();
}

void MotionSystem::ImmediateEvent(Component* component, uint32 event)
{
    DVASSERT(motionSingleComponent != nullptr);

    if (event == EventSystem::SKELETON_CONFIG_CHANGED)
    {
        MotionComponent* motionComponent = GetMotionComponent(component->GetEntity());
        motionSingleComponent->rebindSkeleton.push_back(motionComponent);
    }
}

void MotionSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_MOTION_SYSTEM);
    DVASSERT(motionSingleComponent != nullptr);

    for (MotionComponent* motionComponent : motionSingleComponent->reloadDescriptor)
    {
        motionComponent->ReloadFromFile();
        FindAndRemoveExchangingWithLast(activeComponents, motionComponent);
        motionSingleComponent->rebindSkeleton.emplace_back(motionComponent);
    }

    for (MotionComponent* motionComponent : motionSingleComponent->rebindSkeleton)
    {
        SkeletonComponent* skeleton = GetSkeletonComponent(motionComponent->GetEntity());
        if (skeleton != nullptr)
        {
            uint32 motionLayersCount = motionComponent->GetMotionLayersCount();
            for (uint32 l = 0; l < motionLayersCount; ++l)
            {
                MotionLayer* motionLayer = motionComponent->GetMotionLayer(l);
                motionLayer->BindSkeleton(skeleton);
            }

            FindAndRemoveExchangingWithLast(activeComponents, motionComponent);
            activeComponents.emplace_back(motionComponent);

            SkeletonPose defaultPose = skeleton->GetDefaultPose();
            SimpleMotion* simpleMotion = motionComponent->simpleMotion;
            if (simpleMotion != nullptr)
            {
                simpleMotion->BindSkeleton(skeleton);
                simpleMotion->EvaluatePose(&defaultPose);
            }
            skeleton->ApplyPose(defaultPose);
        }
    }

    for (MotionComponent* motionComponent : motionSingleComponent->stopSimpleMotion)
    {
        if (motionComponent->simpleMotion != nullptr)
            motionComponent->simpleMotion->Stop();
    }

    for (MotionComponent* motionComponent : motionSingleComponent->startSimpleMotion)
    {
        SimpleMotion* simpleMotion = motionComponent->simpleMotion;
        if (simpleMotion != nullptr)
        {
            if (simpleMotion->IsPlaying())
                continue;

            simpleMotion->SetRepeatsCount(motionComponent->simpleMotionRepeatsCount);
            simpleMotion->Start();
        }
    }

    motionSingleComponent->Clear();

    for (MotionComponent* component : activeComponents)
    {
        UpdateMotionLayers(component, timeElapsed);
    }
}

void MotionSystem::UpdateMotionLayers(MotionComponent* motionComponent, float32 dTime)
{
    DVASSERT(motionComponent);

    SkeletonComponent* skeleton = GetSkeletonComponent(motionComponent->GetEntity());
    if (skeleton != nullptr && (motionComponent->GetMotionLayersCount() != 0 || (motionComponent->simpleMotion != nullptr && motionComponent->simpleMotion->IsPlaying())))
    {
        dTime *= motionComponent->playbackRate;
        SkeletonPose resultPose = skeleton->GetDefaultPose();

        uint32 motionLayersCount = motionComponent->GetMotionLayersCount();
        for (uint32 l = 0; l < motionLayersCount; ++l)
        {
            MotionLayer* motionLayer = motionComponent->GetMotionLayer(l);

            motionLayer->Update(dTime);

            for (const auto& motionEnd : motionLayer->GetEndedMotions())
                motionSingleComponent->animationEnd.insert(MotionSingleComponent::AnimationInfo(motionComponent, motionLayer->GetName(), motionEnd));

            for (const auto& motionMarker : motionLayer->GetReachedMarkers())
                motionSingleComponent->animationMarkerReached.insert(MotionSingleComponent::AnimationInfo(motionComponent, motionLayer->GetName(), motionMarker.first, motionMarker.second));

            const SkeletonPose& pose = motionLayer->GetCurrentSkeletonPose();
            MotionLayer::eMotionBlend blendMode = motionLayer->GetBlendMode();
            switch (blendMode)
            {
            case MotionLayer::BLEND_OVERRIDE:
                resultPose.Override(pose);
                motionComponent->rootOffsetDelta = motionLayer->GetCurrentRootOffsetDelta();
                break;
            case MotionLayer::BLEND_ADD:
                resultPose.Add(pose);
                break;
            case MotionLayer::BLEND_DIFF:
                resultPose.Diff(pose);
                break;
            default:
                break;
            }
        }

        SimpleMotion* simpleMotion = motionComponent->simpleMotion;
        if (simpleMotion != nullptr && simpleMotion->IsPlaying())
        {
            simpleMotion->Update(dTime);
            if (!simpleMotion->IsPlaying())
                motionSingleComponent->simpleMotionFinished.emplace_back(motionComponent);

            simpleMotion->EvaluatePose(&resultPose);
        }

        skeleton->ApplyPose(resultPose);
    }
}
}