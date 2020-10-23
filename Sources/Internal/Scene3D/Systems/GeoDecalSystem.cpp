#include "Scene3D/Systems/GeoDecalSystem.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/SkeletonSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Scene3D/Scene.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Render/Highlevel/SkinnedMesh.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"

#define DAVA_GEODECAL_SYSTEM_DEBUG_RENDER 0

namespace DAVA
{
GeoDecalSystem::GeoDecalSystem(Scene* scene)
    : SceneSystem(scene)
{
}

void GeoDecalSystem::BakeDecals()
{
    for (auto& decal : decals)
    {
        GeoDecalComponent* geoDecalComponent = static_cast<GeoDecalComponent*>(decal.first);
        const GeoDecalManager::DecalConfig& currentConfig = geoDecalComponent->GetConfig();
        if (currentConfig != decal.second.lastValidConfig)
        {
            Entity* entity = decal.first->GetEntity();
            RemoveCreatedDecals(entity, geoDecalComponent);
            BuildDecal(entity, geoDecalComponent);
            decal.second.lastValidConfig = currentConfig;

            GlobalEventSystem::Instance()->Event(geoDecalComponent, EventSystem::GEO_DECAL_CHANGED);
        }
    }
}

void GeoDecalSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_GEODECAL_SYSTEM);

    TransformSingleComponent* tsc = GetScene()->transformSingleComponent;

    for (auto& pair : tsc->worldTransformChanged.map)
    {
        if (pair.first->GetComponentsCount(Type::Instance<GeoDecalComponent>()) > 0)
        {
            for (Entity* entity : pair.second)
            {
                for (uint32 i = 0, e = entity->GetComponentCount<GeoDecalComponent>(); i < e; ++i)
                {
                    GeoDecalComponent* component = entity->GetComponent<GeoDecalComponent>(i);
                    if (component->GetRebakeOnTransform())
                    {
                        decals[component].lastValidConfig.invalidate();
                    }
                }
            }
        }
    }

    for (auto& i : decals)
    {
        if (static_cast<GeoDecalComponent*>(i.first)->GetRebakeOnTransform())
        {
            for (const std::pair<Entity*, GeoDecalManager::Decal>& d : i.second.decals)
            {
                auto changedEntity = std::find(tsc->localTransformChanged.begin(), tsc->localTransformChanged.end(), d.first);
                if (changedEntity != tsc->localTransformChanged.end())
                {
                    i.second.lastValidConfig.invalidate();
                }
            }
        }
    }

    BakeDecals();

#if (DAVA_GEODECAL_SYSTEM_DEBUG_RENDER)
    DAVA::RenderHelper* drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    for (auto& decal : decals)
    {
        GeoDecalComponent* geoDecalComponent = static_cast<GeoDecalComponent*>(decal.first);
        DAVA::Matrix4 transform = decal.first->GetEntity()->GetComponent<TransformComponent>()->GetWorldMatrix();

        DAVA::RenderHelper::eDrawType dt = DAVA::RenderHelper::eDrawType::DRAW_WIRE_DEPTH;
        DAVA::Color baseColor(1.0f, 0.5f, 0.25f, 1.0f);
        DAVA::Color accentColor(1.0f, 1.0f, 0.5f, 1.0f);

        DAVA::AABBox3 box = geoDecalComponent->GetBoundingBox();
        DAVA::Vector3 boxCenter = box.GetCenter();
        DAVA::Vector3 boxHalfSize = 0.5f * box.GetSize();

        DAVA::Vector3 farPoint = DAVA::Vector3(boxCenter.x, boxCenter.y, box.min.z) * transform;
        DAVA::Vector3 nearPoint = DAVA::Vector3(boxCenter.x, boxCenter.y, box.max.z) * transform;

        DAVA::Vector3 direction = farPoint - nearPoint;
        direction.Normalize();

        drawer->DrawAABoxTransformed(box, transform, baseColor, dt);

        if (geoDecalComponent->GetConfig().mapping == DAVA::GeoDecalManager::Mapping::CYLINDRICAL)
        {
            DAVA::Vector3 side = DAVA::Vector3(boxCenter.x - boxHalfSize.x, 0.0f, box.max.z) * transform;

            float radius = (side - nearPoint).Length();
            drawer->DrawCircle(nearPoint, direction, radius, 32, accentColor, dt);
            drawer->DrawCircle(farPoint, -direction, radius, 32, accentColor, dt);
            drawer->DrawLine(nearPoint, side, accentColor);
        }
        else if (geoDecalComponent->GetConfig().mapping == DAVA::GeoDecalManager::Mapping::SPHERICAL)
        {
            // no extra debug visualization
        }
        else /* planar assumed */
        {
            drawer->DrawArrow(nearPoint - direction, nearPoint, 0.25f * direction.Length(), accentColor, dt);
        }
    }
#endif
}

void GeoDecalSystem::AddComponent(Entity* entity, Component* component)
{
    DVASSERT(component != nullptr);
    DVASSERT(component->GetType()->Is<GeoDecalComponent>());
    DVASSERT(decals.count(component) == 0);

    decals[component].lastValidConfig.invalidate();
}

void GeoDecalSystem::RemoveComponent(Entity* entity, Component* component)
{
    DVASSERT(component != nullptr);
    DVASSERT(component->GetType()->Is<GeoDecalComponent>());
    DVASSERT(decals.count(component) > 0);

    RemoveCreatedDecals(entity, static_cast<GeoDecalComponent*>(component));
    decals.erase(component);
}

void GeoDecalSystem::AddEntity(Entity* entity)
{
    for (uint32 i = 0, e = entity->GetComponentCount<GeoDecalComponent>(); i < e; ++i)
    {
        Component* component = entity->GetComponent<GeoDecalComponent>(i);
        AddComponent(entity, component);
    }
}

void GeoDecalSystem::RemoveEntity(Entity* entity)
{
    for (uint32 i = 0, e = entity->GetComponentCount<GeoDecalComponent>(); i < e; ++i)
    {
        Component* component = entity->GetComponent<GeoDecalComponent>(i);
        RemoveComponent(entity, component);
    }
}

void GeoDecalSystem::PrepareForRemove()
{
    GeoDecalManager* manager = GetScene()->GetRenderSystem()->GetGeoDecalManager();
    for (const auto& node : decals)
    {
        for (const auto& geoDecal : node.second.decals)
        {
            manager->DeleteDecal(geoDecal.second);
        }
    }

    decals.clear();
}

void GeoDecalSystem::GatherRenderableEntitiesInBox(Entity* top, const AABBox3& box, Vector<RenderableEntity>& entities)
{
    RenderObject* object = GetRenderObject(top);
    if ((object != nullptr) && object->GetWorldBoundingBox().IntersectsWithBox(box))
    {
        if ((object->GetType() == RenderObject::eType::TYPE_MESH) || (object->GetType() == RenderObject::eType::TYPE_SKINNED_MESH))
            entities.emplace_back(top, object);
    }

    for (int32 i = 0; i < top->GetChildrenCount(); ++i)
        GatherRenderableEntitiesInBox(top->GetChild(i), box, entities);
}

void GeoDecalSystem::RemoveCreatedDecals(Entity* entity, GeoDecalComponent* component)
{
    GeoDecalManager* manager = GetScene()->GetRenderSystem()->GetGeoDecalManager();
    for (const auto& i : decals[component].decals)
    {
        manager->DeleteDecal(i.second);
    }
    decals[component].decals.clear();
}

void GeoDecalSystem::BuildDecal(Entity* entityWithDecal, GeoDecalComponent* component)
{
    AABBox3 worldSpaceBox;
    TransformComponent* transformComponent = entityWithDecal->GetComponent<TransformComponent>();
    component->GetBoundingBox().GetTransformedBox(transformComponent->GetWorldMatrix(), worldSpaceBox);

    Vector<RenderableEntity> entities;
    GatherRenderableEntitiesInBox(entityWithDecal->GetScene(), worldSpaceBox, entities);

    RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    GeoDecalManager* manager = renderSystem->GetGeoDecalManager();
    for (const RenderableEntity& e : entities)
    {
        SkeletonComponent* skeletonComponent = GetSkeletonComponent(e.entity);
        bool isValidSkinnedMesh = (e.renderObject->GetType() == RenderObject::TYPE_SKINNED_MESH) && (skeletonComponent != nullptr);

        if (isValidSkinnedMesh)
        {
            // update mesh before creating decal in order to have valid skinning
            SkinnedMesh* mesh = static_cast<SkinnedMesh*>(e.renderObject);
            Scene* scene = GetScene();
            scene->skeletonSystem->UpdateSkinnedMesh(skeletonComponent, mesh);
        }

        GeoDecalManager::Decal decal = manager->BuildDecal(component->GetConfig(), transformComponent->GetWorldMatrix(), e.renderObject);
        decals[component].decals.emplace_back(e.entity, decal);
    }
}
}
