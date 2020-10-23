#pragma once

#include <Base/BaseTypes.h>
#include <Entity/SceneSystem.h>
#include <Base/Message.h>

namespace DAVA
{
class Camera;
class RenderObject;
class StaticOcclusionComponent;
class StaticOcclusionData;
class StaticOcclusionDataComponent;
class StaticOcclusionDebugDrawComponent;
class NMaterial;
class PolygonGroup;

// System that allow to use occlusion information during rendering
class StaticOcclusionSystem : public SceneSystem
{
public:
    StaticOcclusionSystem(Scene* scene);

    void SetCamera(Camera* camera);

    void RegisterEntity(Entity* entity) override;
    void UnregisterEntity(Entity* entity) override;
    void RegisterComponent(Entity* entity, Component* component) override;
    void UnregisterComponent(Entity* entity, Component* component) override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

    void AddRenderObjectToOcclusion(RenderObject* renderObject);
    void RemoveRenderObjectFromOcclusion(RenderObject* renderObject);

    void ClearOcclusionObjects();
    void CollectOcclusionObjectsRecursively(Entity* entity);

    void InvalidateOcclusion();
    void InvalidateOcclusionIndicesRecursively(Entity* entity);

private:
    // Final system part
    void ProcessStaticOcclusionForOneDataSet(uint32 blockIndex, StaticOcclusionData* data);
    void UndoOcclusionVisibility();

private:
    Camera* camera = nullptr;
    StaticOcclusionData* activePVSSet = nullptr;
    uint32 activeBlockIndex = 0;
    Vector<StaticOcclusionDataComponent*> staticOcclusionComponents;
    Vector<RenderObject*> indexedRenderObjects;
    bool isInPvs = false;

    uint32 occludedObjectsCount = 0;
    uint32 visibleObjestsCount = 0;
};

class StaticOcclusionDebugDrawSystem : public SceneSystem
{
public:
    StaticOcclusionDebugDrawSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void ImmediateEvent(Component* component, uint32 event) override;
    void PrepareForRemove() override;

    ~StaticOcclusionDebugDrawSystem();

protected:
    void SetScene(Scene* scene) override;

private:
    void UpdateGeometry(StaticOcclusionDebugDrawComponent* component);

    void CreateStaticOcclusionDebugDrawVertices(StaticOcclusionDebugDrawComponent* target, StaticOcclusionComponent* source);
    void CreateStaticOcclusionDebugDrawGridIndice(StaticOcclusionDebugDrawComponent* target, StaticOcclusionComponent* source);
    void CreateStaticOcclusionDebugDrawCoverIndice(StaticOcclusionDebugDrawComponent* target, StaticOcclusionComponent* source);

    void RemoveComponentFromEntity(Entity* entity);

    NMaterial *gridMaterial, *coverMaterial;
    uint32 vertexLayoutId;
    Vector<Entity*> entities;
};

inline void StaticOcclusionSystem::SetCamera(Camera* _camera)
{
    camera = _camera;
}

} // ns
