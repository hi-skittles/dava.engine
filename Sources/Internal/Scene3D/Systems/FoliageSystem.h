#ifndef __DAVAENGINE_SCENE3D_FOLIAGE_SYSTEM_H__
#define __DAVAENGINE_SCENE3D_FOLIAGE_SYSTEM_H__

#include "Base/BaseTypes.h"
#include "Math/MathConstants.h"
#include "Math/Matrix4.h"
#include "Base/Singleton.h"
#include "Entity/SceneSystem.h"
#include "Render/Highlevel/RenderBatch.h"

namespace DAVA
{
class Entity;
class Scene;
class Landscape;
class VegetationRenderObject;

class FoliageSystem : public SceneSystem
{
public:
    FoliageSystem(Scene* scene);
    ~FoliageSystem() override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Process(float32 timeElapsed) override;

    void SyncFoliageWithLandscape();

    void SetPerturbation(const Vector3& point, const Vector3& force, float32 distance);

    void CollectFoliageMaterials(Set<NMaterial*>& materials);

    void SetFoliageVisible(bool show);
    bool IsFoliageVisible() const;

    void DebugDrawVegetation();

private:
    void ProcessVegetationRenderObject(VegetationRenderObject*, float32 timeElapsed);

private:
    Entity* landscapeEntity = nullptr;
    DAVA::Vector<Entity*> foliageEntities;
};
};

#endif
