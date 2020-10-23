#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderPassNames.h"

namespace DAVA
{
class Camera;
class RenderPass : public InspBase
{
public:
    RenderPass(const FastName& name);
    virtual ~RenderPass();

    inline const FastName& GetName() const;

    void AddRenderLayer(RenderLayer* layer, RenderLayer::eRenderLayerID afterLayer = RenderLayer::RENDER_LAYER_INVALID_ID);
    void RemoveRenderLayer(RenderLayer* layer);

    virtual void Draw(RenderSystem* renderSystem);

    inline uint32 GetRenderLayerCount() const;
    inline RenderLayer* GetRenderLayer(uint32 index) const;

    inline rhi::RenderPassConfig& GetPassConfig();
    inline void SetViewport(const Rect& viewPort);

    void SetRenderTargetProperties(uint32 width, uint32 height, PixelFormat format);

protected:
    FastName passName;
    rhi::RenderPassConfig passConfig;
    Rect viewport;

    Vector2 viewportSize, rcpViewportSize, viewportOffset; //storage fro dynamic bindings

    /*convinience*/
    void PrepareVisibilityArrays(Camera* camera, RenderSystem* renderSystem);
    void PrepareLayersArrays(const Vector<RenderObject*> objectsArray, Camera* camera);
    void ClearLayersArrays();

    void SetupCameraParams(Camera* mainCamera, Camera* drawCamera, Vector4* externalClipPlane = NULL);
    void DrawLayers(Camera* camera);
    void DrawDebug(Camera* camera, RenderSystem* renderSystem);

    bool BeginRenderPass();
    void EndRenderPass();

    void ValidateMultisampledTextures(const rhi::RenderPassConfig& forConfig);

    Vector<RenderLayer*> renderLayers;
    std::array<RenderBatchArray, RenderLayer::RENDER_LAYER_ID_COUNT> layersBatchArrays;
    Vector<RenderObject*> visibilityArray;

    rhi::HPacketList packetList;
    rhi::HRenderPass renderPass;

    Texture::FBODescriptor multisampledDescription;
    Texture* multisampledTexture = nullptr;

    struct RenderTargetProperites
    {
        uint32 width = 0;
        uint32 height = 0;
        PixelFormat format = PixelFormat::FORMAT_INVALID;
    } renderTargetProperties;

    friend class RenderSystem;
};

inline rhi::RenderPassConfig& RenderPass::GetPassConfig()
{
    return passConfig;
}

inline void RenderPass::SetViewport(const Rect& _viewport)
{
    viewport = _viewport;
    passConfig.viewport.x = int32(viewport.x);
    passConfig.viewport.y = int32(viewport.y);
    passConfig.viewport.width = int32(viewport.dx);
    passConfig.viewport.height = int32(viewport.dy);
}

inline const FastName& RenderPass::GetName() const
{
    return passName;
}

inline uint32 RenderPass::GetRenderLayerCount() const
{
    return uint32(renderLayers.size());
}

inline RenderLayer* RenderPass::GetRenderLayer(uint32 index) const
{
    return renderLayers[index];
}

class WaterPrePass : public RenderPass
{
public:
    inline void SetWaterLevel(float32 level)
    {
        waterLevel = level;
    }
    WaterPrePass(const FastName& name);
    ~WaterPrePass();

protected:
    Camera *passMainCamera, *passDrawCamera;
    float32 waterLevel = 0;
};

class WaterReflectionRenderPass : public WaterPrePass
{
public:
    WaterReflectionRenderPass(const FastName& name);
    virtual void Draw(RenderSystem* renderSystem);

private:
    void UpdateCamera(Camera* camera);
};

class WaterRefractionRenderPass : public WaterPrePass
{
public:
    WaterRefractionRenderPass(const FastName& name);
    virtual void Draw(RenderSystem* renderSystem);
};

class MainForwardRenderPass : public RenderPass
{
public:
    MainForwardRenderPass(const FastName& name);
    ~MainForwardRenderPass();
    virtual void Draw(RenderSystem* renderSystem);

private:
    WaterReflectionRenderPass* reflectionPass;
    WaterRefractionRenderPass* refractionPass;

    AABBox3 waterBox;

    void InitReflectionRefraction();
    void PrepareReflectionRefractionTextures(RenderSystem* renderSystem);
};
}
