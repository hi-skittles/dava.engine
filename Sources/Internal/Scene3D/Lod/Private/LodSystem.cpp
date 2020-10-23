#include "Scene3D/Lod/LodSystem.h"
#include "Debug/DVAssert.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Render/Highlevel/Camera.h"
#include "Time/SystemTimer.h"
#include "Core/PerformanceSettings.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Scene3D/Systems/EventSystem.h"

namespace DAVA
{
LodSystem::LodSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::START_PARTICLE_EFFECT);
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::STOP_PARTICLE_EFFECT);
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::LOD_DISTANCE_CHANGED);
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::LOD_RECURSIVE_UPDATE_ENABLED);
}

void LodSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_LOD_SYSTEM);

    TransformSingleComponent* tsc = GetScene()->transformSingleComponent;

    for (auto& pair : tsc->worldTransformChanged.map)
    {
        if (pair.first->GetComponentsCount(Type::Instance<LodComponent>()) > 0)
        {
            for (Entity* entity : pair.second)
            {
                auto iter = fastMap.find(entity);
                if (iter != fastMap.end())
                {
                    int32 index = iter->second;
                    FastStruct* fast = &fastVector[index];
                    fast->position = entity->GetComponent<TransformComponent>()->GetWorldTransform().GetTranslation();
                }
            }
        }
    }

    Camera* camera = GetScene()->GetCurrentCamera();
    if (!camera)
    {
        return;
    }

    if (timeElapsed == 0.f)
    {
        timeElapsed = 0.000001f;
    }

    //lod degrade
    float32 currFps = 1.0f / timeElapsed;
    float32 currPSValue = (currFps - PerformanceSettings::Instance()->GetPsPerformanceMinFPS()) / (PerformanceSettings::Instance()->GetPsPerformanceMaxFPS() - PerformanceSettings::Instance()->GetPsPerformanceMinFPS());
    currPSValue = Clamp(currPSValue, 0.0f, 1.0f);
    float32 lodOffset = PerformanceSettings::Instance()->GetPsPerformanceLodOffset() * (1 - currPSValue);
    float32 lodMult = 1.0f + (PerformanceSettings::Instance()->GetPsPerformanceLodMult() - 1.0f) * (1 - currPSValue);
    /*as we use square values - multiply it too*/
    lodOffset *= lodOffset;
    lodMult *= lodMult;

    Vector3 cameraPos = camera->GetPosition();
    float32 cameraZoomFactorSq = camera->GetZoomFactor() * camera->GetZoomFactor();

    int32 size = static_cast<int32>(fastVector.size());
    for (int32 index = 0; index < size; ++index)
    {
        FastStruct& fast = fastVector[index];
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
        __builtin_prefetch(&fastVector[index + 1]);
        __builtin_prefetch(&fastVector[index + 2]);
        __builtin_prefetch(&fastVector[index + 3]);
        __builtin_prefetch(&fastVector[index + 4]);
#endif

        if (fast.effectStopped)
        {
            //do not update inactive effects
        }
        else
        {
            int32 newLod = 0;
            if (forceLodUsed && (slowVector[index].forceLodLayer != LodComponent::INVALID_LOD_LAYER))
            {
                newLod = slowVector[index].forceLodLayer;
            }
            else
            {
                float32 dst;
                if (forceLodUsed && slowVector[index].forceLodDistance != LodComponent::INVALID_DISTANCE)
                {
                    SlowStruct& slow = slowVector[index];
                    dst = slow.forceLodDistance * slow.forceLodDistance;
                }
                else
                {
                    dst = (cameraPos - fast.position).SquareLength();
                    dst *= cameraZoomFactorSq;
                }

                if (fast.isEffect)
                {
                    if (dst > fast.farSquare0) //preserve lod 0 from degrade
                        dst = dst * lodMult + lodOffset;
                }

                if ((fast.currentLod != LodComponent::INVALID_LOD_LAYER) &&
                    (dst >= fast.nearSquare) &&
                    (dst <= fast.farSquare))
                {
                    newLod = fast.currentLod;
                }
                else
                {
                    newLod = LodComponent::INVALID_LOD_LAYER;
                    SlowStruct* slow = &slowVector[index];
                    for (int32 i = LodComponent::MAX_LOD_LAYERS - 1; i >= 0; --i)
                    {
                        if (dst < slow->farSquares[i])
                        {
                            newLod = i;
                        }
                    }
                }
            }

            //switch lod
            if (fast.currentLod != newLod)
            {
                fast.currentLod = newLod;
                SlowStruct& slow = slowVector[index];
                slow.lod->currentLod = fast.currentLod;

                if (newLod == LodComponent::INVALID_LOD_LAYER)
                {
                    fast.nearSquare = fast.farSquare;
                    fast.farSquare = std::numeric_limits<float32>::max();
                }
                else
                {
                    fast.nearSquare = slow.nearSquares[fast.currentLod];
                    fast.farSquare = slow.farSquares[fast.currentLod];
                }

                ParticleEffectComponent* effect = slow.effect;
                if (effect)
                {
                    effect->SetDesiredLodLevel(fast.currentLod);
                }
                else
                {
                    if (slow.recursiveUpdate)
                    {
                        SetEntityLodRecursive(slow.entity, fast.currentLod);
                    }
                    else
                    {
                        SetEntityLod(slow.entity, fast.currentLod);
                    }
                }
            }
        }
    }
}

void LodSystem::UpdateDistances(LodComponent* from, LodSystem::SlowStruct* to)
{
    //lods will overlap +- 5%
    to->nearSquares[0] = 0.f;
    to->farSquares[0] = from->GetLodLayerDistance(0) * 1.05f;
    to->farSquares[0] *= to->farSquares[0];

    for (int32 i = 1; i < LodComponent::MAX_LOD_LAYERS; ++i)
    {
        to->nearSquares[i] = from->GetLodLayerDistance(i - 1) * 0.95f;
        to->nearSquares[i] *= to->nearSquares[i];

        to->farSquares[i] = from->GetLodLayerDistance(i) * 1.05f;
        to->farSquares[i] *= to->farSquares[i];
    }
}

void LodSystem::AddEntity(Entity* entity)
{
    TransformComponent* transform = entity->GetComponent<TransformComponent>();
    LodComponent* lod = entity->GetComponent<LodComponent>();
    ParticleEffectComponent* effect = entity->GetComponent<ParticleEffectComponent>();
    Vector3 position = transform->GetWorldTransform().GetTranslation();

    lod->currentLod = LodComponent::INVALID_LOD_LAYER;

    SlowStruct slow;
    slow.entity = entity;
    slow.lod = lod;
    slow.effect = effect;
    slow.recursiveUpdate = lod->recursiveUpdate;
    UpdateDistances(lod, &slow);
    slowVector.push_back(slow);

    FastStruct fast;
    fast.farSquare0 = slow.farSquares[0];
    fast.position = position;
    fast.currentLod = LodComponent::INVALID_LOD_LAYER;
    fast.nearSquare = -1.f;
    fast.farSquare = -1.f;
    fast.effectStopped = effect ? effect->IsStopped() : false;
    fast.isEffect = effect != nullptr;

    fastVector.push_back(fast);
    fastMap.insert(std::make_pair(entity, static_cast<int32>(fastVector.size() - 1)));
}

void LodSystem::RemoveEntity(Entity* entity)
{
    //find in fastMap
    auto iter = fastMap.find(entity);
    DVASSERT(iter != fastMap.end());
    int32 index = iter->second;

    //delete from slow
    SlowStruct& slowLast = slowVector.back();
    slowVector[index].lod->currentLod = LodComponent::INVALID_LOD_LAYER;
    slowVector[index] = slowLast;
    slowVector.pop_back();

    //delete from fast
    FastStruct& fastLast = fastVector.back();
    fastVector[index] = fastLast;
    fastVector.pop_back();

    //delete in fastMap
    fastMap.erase(entity);
    Entity* movedEntity = slowLast.entity;
    if (entity != movedEntity)
    {
        fastMap[movedEntity] = index;
    }
}

void LodSystem::RegisterComponent(Entity* entity, Component* component)
{
    if (component->GetType()->Is<ParticleEffectComponent>())
    {
        auto iter = fastMap.find(entity);
        if (iter != fastMap.end())
        {
            int32 index = iter->second;
            SlowStruct* slow = &slowVector[index];
            DVASSERT(slow->effect == nullptr);
            slow->effect = static_cast<ParticleEffectComponent*>(component);
            FastStruct* fast = &fastVector[index];
            fast->isEffect = slow->effect != nullptr;
        }
    }

    SceneSystem::RegisterComponent(entity, component);
}

void LodSystem::UnregisterComponent(Entity* entity, Component* component)
{
    if (component->GetType()->Is<ParticleEffectComponent>())
    {
        auto iter = fastMap.find(entity);
        if (iter != fastMap.end())
        {
            int32 index = iter->second;
            SlowStruct* slow = &slowVector[index];
            DVASSERT(slow->effect != nullptr);
            slow->effect = nullptr;
            FastStruct* fast = &fastVector[index];
            fast->isEffect = false;
        }
    }

    SceneSystem::UnregisterComponent(entity, component);
}

void LodSystem::PrepareForRemove()
{
    slowVector.clear();
    fastVector.clear();
    fastMap.clear();
}

void LodSystem::ImmediateEvent(Component* component, uint32 event)
{
    switch (event)
    {
    case EventSystem::START_PARTICLE_EFFECT:
    case EventSystem::STOP_PARTICLE_EFFECT:
    {
        DVASSERT(component->GetType()->Is<ParticleEffectComponent>());
        auto iter = fastMap.find(component->GetEntity());
        if (iter != fastMap.end())
        {
            int32 index = iter->second;
            FastStruct* fast = &fastVector[index];
            fast->effectStopped = event == EventSystem::STOP_PARTICLE_EFFECT;
        }
    }
    break;

    case EventSystem::LOD_DISTANCE_CHANGED:
    {
        DVASSERT(component->GetType()->Is<LodComponent>());
        LodComponent* lod = static_cast<LodComponent*>(component);
        auto iter = fastMap.find(component->GetEntity());
        if (iter != fastMap.end())
        {
            int32 index = iter->second;
            SlowStruct* slow = &slowVector[index];
            UpdateDistances(lod, slow);

            FastStruct* fast = &fastVector[index];
            //force recalc nearSquare/farSquare on next Process
            fast->nearSquare = -1.f;
            fast->farSquare = -1.f;
        }
    }
    break;

    case EventSystem::LOD_RECURSIVE_UPDATE_ENABLED:
    {
        DVASSERT(component->GetType()->Is<LodComponent>());
        auto iter = fastMap.find(component->GetEntity());
        DVASSERT(iter != fastMap.end());
        int32 index = iter->second;
        SlowStruct* slow = &slowVector[index];
        slow->recursiveUpdate = true;
    }
    break;

    default:
        break;
    }
}

void LodSystem::SetForceLodLayer(LodComponent* forComponent, int32 layer)
{
    auto iter = fastMap.find(forComponent->GetEntity());
    DVASSERT(iter != fastMap.end());
    int32 index = iter->second;
    SlowStruct* slow = &slowVector[index];
    slow->forceLodLayer = layer;

    if (layer != LodComponent::INVALID_LOD_LAYER)
    {
        forceLodUsed = true;
    }
}

int32 LodSystem::GetForceLodLayer(LodComponent* forComponent)
{
    auto iter = fastMap.find(forComponent->GetEntity());
    DVASSERT(iter != fastMap.end());
    int32 index = iter->second;
    SlowStruct* slow = &slowVector[index];
    return slow->forceLodLayer;
}

void LodSystem::SetForceLodDistance(LodComponent* forComponent, float32 distance)
{
    auto iter = fastMap.find(forComponent->GetEntity());
    DVASSERT(iter != fastMap.end());
    int32 index = iter->second;
    SlowStruct* slow = &slowVector[index];
    slow->forceLodDistance = distance;

    if (distance != LodComponent::INVALID_DISTANCE)
    {
        forceLodUsed = true;
    }
}

DAVA::float32 LodSystem::GetForceLodDistance(LodComponent* forComponent)
{
    auto iter = fastMap.find(forComponent->GetEntity());
    DVASSERT(iter != fastMap.end());
    int32 index = iter->second;
    SlowStruct* slow = &slowVector[index];
    return slow->forceLodDistance;
}

void LodSystem::SetEntityLod(Entity* entity, int32 currentLod)
{
    RenderObject* ro = GetRenderObject(entity);
    if (ro)
    {
        ro->SetLodIndex(currentLod);
    }
}

void LodSystem::SetEntityLodRecursive(Entity* entity, int32 currentLod)
{
    SetEntityLod(entity, currentLod);

    int32 count = entity->GetChildrenCount();
    for (int32 i = 0; i < count; ++i)
    {
        SetEntityLodRecursive(entity->GetChild(i), currentLod);
    }
}
}
