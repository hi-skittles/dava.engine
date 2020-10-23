#pragma once

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Render/Highlevel/RenderObject.h"

namespace DAVA
{
class Camera;
class LodComponent;
class ParticleEffectComponent;

class LodSystem : public SceneSystem
{
public:
    LodSystem(Scene* scene);

    void Process(float32 timeElapsed) override;
    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void RegisterComponent(Entity* entity, Component* component) override;
    void UnregisterComponent(Entity* entity, Component* component) override;
    void PrepareForRemove() override;
    void ImmediateEvent(Component* component, uint32 event) override;

    void SetForceLodLayer(LodComponent* forComponent, int32 layer);
    int32 GetForceLodLayer(LodComponent* forComponent);

    void SetForceLodDistance(LodComponent* forComponent, float32 distance);
    float32 GetForceLodDistance(LodComponent* forComponent);

private:
    struct SlowStruct
    {
        Array<float32, LodComponent::MAX_LOD_LAYERS> farSquares;
        Array<float32, LodComponent::MAX_LOD_LAYERS> nearSquares;
        Entity* entity = nullptr;
        int32 forceLodLayer = LodComponent::INVALID_LOD_LAYER;
        float32 forceLodDistance = LodComponent::INVALID_DISTANCE;
        LodComponent* lod = nullptr;
        ParticleEffectComponent* effect = nullptr;
        bool recursiveUpdate = false;
    };
    Vector<SlowStruct> slowVector;

    struct FastStruct
    {
        float32 farSquare0;
        Vector3 position;
        int32 currentLod;
        float32 nearSquare;
        float32 farSquare;
        bool effectStopped : 1;
        bool isEffect : 1;
    };
    Vector<FastStruct> fastVector;
    UnorderedMap<Entity*, int32> fastMap = UnorderedMap<Entity*, int32>(1024);

    void UpdateDistances(LodComponent* from, LodSystem::SlowStruct* to);

    void SetEntityLod(Entity* entity, int32 currentLod);
    void SetEntityLodRecursive(Entity* entity, int32 currentLod);

    bool forceLodUsed = false;
};
}
