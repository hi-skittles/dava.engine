#pragma once

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/GeoDecalComponent.h"
#include "Render/Highlevel/GeoDecalManager.h"

namespace DAVA
{
class GeoDecalSystem : public SceneSystem
{
public:
    GeoDecalSystem(Scene* scene);

    void BakeDecals();

    void Process(float32 timeElapsed) override;
    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void AddComponent(Entity* entity, Component* component) override;
    void RemoveComponent(Entity* entity, Component* component) override;
    void PrepareForRemove() override;

private:
    struct DecalBuildInfo
    {
        Entity* entity = nullptr;
        GeoDecalComponent* component = nullptr;
        RenderObject* object = nullptr;
        RenderBatch* batch = nullptr;
        AABBox3 box;
        int32 lodIndex = -1;
        int32 switchIndex = -1;
        Vector3 projectionAxis;
        Matrix4 projectionSpaceTransform;
        Matrix4 projectionSpaceInverseTransform;
    };
    struct DecalRenderBatch
    {
        RenderBatch* batch = nullptr;
        int32 lodIndex = -1;
        int32 switchIndex = -1;
    };
    struct RenderableEntity
    {
        Entity* entity = nullptr;
        RenderObject* renderObject = nullptr;
        RenderableEntity() = default;
        RenderableEntity(Entity* e, RenderObject* ro)
            : entity(e)
            , renderObject(ro)
        {
        }
    };
    void BuildDecal(Entity* entity, GeoDecalComponent* component);
    void RemoveCreatedDecals(Entity* entity, GeoDecalComponent* component);
    void GatherRenderableEntitiesInBox(Entity* top, const AABBox3& box, Vector<RenderableEntity>&);

private:
    struct GeoDecalCacheEntry
    {
        GeoDecalManager::DecalConfig lastValidConfig;
        Vector<std::pair<Entity*, GeoDecalManager::Decal>> decals;
    };
    Map<Component*, GeoDecalCacheEntry> decals;
};
}
