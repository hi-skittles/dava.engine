#pragma once

#include "VisibilityCheckRenderer.h"
#include "Entity/SceneSystem.h"

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

namespace DAVA
{
class Landscape;
}

class VisibilityCheckSystem : public DAVA::SceneSystem, VisibilityCheckRendererDelegate, public EditorSceneSystem
{
public:
    static void ReleaseCubemapRenderTargets();

public:
    VisibilityCheckSystem(DAVA::Scene* scene);
    ~VisibilityCheckSystem();

    void RegisterEntity(DAVA::Entity* entity) override;
    void UnregisterEntity(DAVA::Entity* entity) override;
    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void Recalculate();
    void Process(DAVA::float32 timeElapsed) override;

    void InvalidateMaterials();

    void FixCurrentFrame();
    void ReleaseFixedFrame();

protected:
    void Draw() override;

private:
    using EntityMap = DAVA::Map<DAVA::Entity*, DAVA::Vector<DAVA::Vector3>>;

    void BuildPointSetForEntity(EntityMap::value_type& item);
    void BuildIndexSet();
    DAVA::Color GetNormalizedColorForEntity(const EntityMap::value_type& item) const;

    DAVA::Camera* GetRenderCamera() const;
    DAVA::Camera* GetFinalGatherCamera() const;

    void UpdatePointSet();
    void Prerender();

    bool CacheIsValid();
    void BuildCache();

    bool ShouldDrawRenderObject(DAVA::RenderObject*) override;

    struct StateCache
    {
        DAVA::Size2i viewportSize;
        DAVA::Matrix4 viewprojMatrix;
        DAVA::Camera* camera = nullptr;
    };

private:
    EntityMap entitiesWithVisibilityComponent;
    DAVA::Landscape* landscape = nullptr;
    DAVA::Vector<VisibilityCheckRenderer::VisbilityPoint> controlPoints;
    DAVA::Vector<DAVA::uint32> controlPointIndices;
    DAVA::Map<DAVA::RenderObject*, DAVA::Entity*> renderObjectToEntity;
    VisibilityCheckRenderer renderer;
    DAVA::ScopedPtr<DAVA::NMaterial> debugMaterial;
    StateCache stateCache;
    size_t currentPointIndex = 0;
    bool shouldPrerender = true;
    bool forceRebuildPoints = true;
    bool shouldFixFrame = false;
};
