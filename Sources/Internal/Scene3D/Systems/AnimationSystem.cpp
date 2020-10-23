#include "Scene3D/Systems/AnimationSystem.h"
#include "Scene3D/Components/AnimationComponent.h"
#include "Scene3D/Entity.h"
#include "Debug/DVAssert.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Scene3D/AnimationData.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"

namespace DAVA
{
AnimationSystem::AnimationSystem(Scene* scene)
    : SceneSystem(scene)
{
    if (scene)
    {
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::START_ANIMATION);
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::STOP_ANIMATION);
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::MOVE_ANIMATION_TO_THE_FIRST_FRAME);
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::MOVE_ANIMATION_TO_THE_LAST_FRAME);
    }
}

AnimationSystem::~AnimationSystem()
{
}

void AnimationSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_ANIMATION_SYSTEM);

    int componentsCount = static_cast<int32>(activeComponents.size());
    for (int i = 0; i < componentsCount; i++)
    {
        AnimationComponent* comp = activeComponents[i];
        comp->time += timeElapsed * comp->animationTimeScale;
        if (comp->time > comp->animation->duration)
        {
            comp->currRepeatsCont++;
            if ((comp->repeatsCount == 0) || (comp->currRepeatsCont < comp->repeatsCount))
            {
                comp->time -= comp->animation->duration;
            }
            else
            {
                RemoveFromActive(comp);
                componentsCount--;
                i--;
                comp->animationTransform.Identity();
                comp->time = 0;

                if (comp->playbackComplete)
                    comp->playbackComplete(comp);

                continue;
            }
        }

        Matrix4 animTransform;
        comp->animation->Interpolate(comp->time, comp->frameIndex).GetMatrix(animTransform);
        comp->animationTransform = comp->animation->invPose * animTransform;
        TransformSingleComponent* tsc = GetScene()->transformSingleComponent;
        tsc->animationTransformChanged.push_back(comp->GetEntity());
    }
}

void AnimationSystem::ImmediateEvent(Component* component, uint32 event)
{
    DVASSERT(component->GetType()->Is<AnimationComponent>());
    AnimationComponent* comp = static_cast<AnimationComponent*>(component);
    if (event == EventSystem::START_ANIMATION)
    {
        if (comp->state == AnimationComponent::STATE_STOPPED)
            AddToActive(comp);
        comp->state = AnimationComponent::STATE_PLAYING;
        comp->currRepeatsCont = 0;
    }
    else if (event == EventSystem::MOVE_ANIMATION_TO_THE_LAST_FRAME)
    {
        MoveAnimationToFrame(comp, comp->animation->GetKeyCount() - 1);
        comp->Stop();
    }
    else if (event == EventSystem::STOP_ANIMATION)
        RemoveFromActive(comp);
    else if (event == EventSystem::MOVE_ANIMATION_TO_THE_FIRST_FRAME)
        MoveAnimationToFrame(comp, 0);
}

void AnimationSystem::MoveAnimationToFrame(AnimationComponent* comp, int frameIndex)
{
    comp->time = 0; // NOTE: will be correct only for last and end frames
    Matrix4 animationMatrix;
    comp->animation->GetKeyForFrame(frameIndex).GetMatrix(animationMatrix);
    comp->animationTransform = comp->animation->invPose * animationMatrix;
    TransformSingleComponent* tsc = GetScene()->transformSingleComponent;
    tsc->animationTransformChanged.push_back(comp->GetEntity());
}

void AnimationSystem::AddToActive(AnimationComponent* comp)
{
    if (comp->state == AnimationComponent::STATE_STOPPED)
    {
        activeComponents.push_back(comp);
    }
}

void AnimationSystem::RemoveFromActive(AnimationComponent* comp)
{
    Vector<AnimationComponent*>::iterator it = std::find(activeComponents.begin(), activeComponents.end(), comp);
    DVASSERT(it != activeComponents.end());
    activeComponents.erase(it);
    comp->state = AnimationComponent::STATE_STOPPED;
}

void AnimationSystem::RemoveEntity(Entity* entity)
{
    AnimationComponent* comp = GetAnimationComponent(entity);
    if (comp->state != AnimationComponent::STATE_STOPPED)
    {
        RemoveFromActive(comp);
    }
}

void AnimationSystem::PrepareForRemove()
{
    activeComponents.clear();
}
};
