#pragma once

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Render/Highlevel/IRenderUpdatable.h"
#include "Render/Highlevel/VisibilityQuadTree.h"
#include "Render/Highlevel/GeoDecalManager.h"
#include "Render/RenderHelper.h"

namespace DAVA
{
class RenderPass;
class RenderLayer;
class RenderObject;
class RenderBatch;
class Entity;
class Camera;
class Light;
class ParticleEmitterSystem;
class RenderHierarchy;
class NMaterial;

class RenderSystem
{
public:
    RenderSystem();
    virtual ~RenderSystem();

    /**
        \brief Get Render Hierarchy. It allow you to work with current render hierarchy and perform all main tasks with geometry on the level.
     */
    inline RenderHierarchy* GetRenderHierarchy() const;

    /**
        \brief Register render objects for permanent rendering
     */
    void RenderPermanent(RenderObject* renderObject);

    /**
        \brief Unregister render objects for permanent rendering
     */
    void RemoveFromRender(RenderObject* renderObject);

    /**
        \brief Register batch
     */
    void RegisterBatch(RenderBatch* batch);
    /**
        \brief Unregister batch
     */
    void UnregisterBatch(RenderBatch* batch);

    void RegisterMaterial(NMaterial* material);
    void UnregisterMaterial(NMaterial* material);

    void PrepareForShutdown();

    /**
        \brief Set main camera
     */
    inline void SetMainCamera(Camera* camera);
    inline Camera* GetMainCamera() const;
    inline void SetDrawCamera(Camera* camera);
    inline Camera* GetDrawCamera() const;

    void SetGlobalMaterial(NMaterial* material);
    NMaterial* GetGlobalMaterial() const;

    void Update(float32 timeElapsed);
    void Render();

    void MarkForUpdate(RenderObject* renderObject);
    void MarkForUpdate(Light* lightNode);

    /**
        \brief This is required for objects that needs permanent update every frame like 
        Landscape and Particles.
     */
    void RegisterForUpdate(IRenderUpdatable* renderObject);
    void UnregisterFromUpdate(IRenderUpdatable* renderObject);

    void AddLight(Light* light);
    void RemoveLight(Light* light);
    Vector<Light*>& GetLights();
    void SetForceUpdateLights();
    void UpdateNearestLights(RenderObject* renderObject);

    void SetMainRenderTarget(rhi::HTexture color, rhi::HTexture depthStencil, rhi::LoadAction colorLoadAction, const Color& clearColor);
    void SetMainPassProperties(uint32 priority, const Rect& viewport, uint32 width, uint32 height, PixelFormat format);
    void SetAntialiasingAllowed(bool allowed);

    void DebugDrawHierarchy(const Matrix4& cameraMatrix);

    RenderHierarchy* GetRenderHierarchy()
    {
        return renderHierarchy;
    }

    inline bool IsRenderHierarchyInitialized() const
    {
        return hierarchyInitialized;
    }

    inline RenderHelper* GetDebugDrawer() const
    {
        return debugDrawer;
    }

    inline GeoDecalManager* GetGeoDecalManager() const
    {
        return geoDecalManager;
    }

public:
    DAVA_DEPRECATED(rhi::RenderPassConfig& GetMainPassConfig());

private:
    void FindNearestLights();
    void AddRenderObject(RenderObject* renderObject);
    void RemoveRenderObject(RenderObject* renderObject);
    void PrebuildMaterial(NMaterial* material);

private:
    friend class RenderPass;

    Vector<IRenderUpdatable*> objectsForUpdate;
    Vector<RenderObject*> objectsForPermanentUpdate;
    Vector<RenderObject*> markedObjects;
    Vector<Light*> movedLights;
    Vector<RenderObject*> renderObjectArray;
    Vector<Light*> lights;

    RenderPass* mainRenderPass = nullptr;
    RenderHierarchy* renderHierarchy = nullptr;
    Camera* mainCamera = nullptr;
    Camera* drawCamera = nullptr;
    NMaterial* globalMaterial = nullptr;
    RenderHelper* debugDrawer = nullptr;
    GeoDecalManager* geoDecalManager = nullptr;

    bool hierarchyInitialized = false;
    bool forceUpdateLights = false;
    bool allowAntialiasing = true;
};

inline RenderHierarchy* RenderSystem::GetRenderHierarchy() const
{
    return renderHierarchy;
}

inline void RenderSystem::SetMainCamera(Camera* _camera)
{
    SafeRelease(mainCamera);
    mainCamera = SafeRetain(_camera);
}

inline void RenderSystem::SetDrawCamera(Camera* _camera)
{
    SafeRelease(drawCamera);
    drawCamera = SafeRetain(_camera);
}

inline Camera* RenderSystem::GetMainCamera() const
{
    return mainCamera;
}

inline Camera* RenderSystem::GetDrawCamera() const
{
    return drawCamera;
}
}
