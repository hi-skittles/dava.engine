#include "Render/Highlevel/RenderPass.h"
#include "Render/Highlevel/RenderLayer.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Highlevel/ShadowVolumeRenderLayer.h"
#include "Render/ShaderCache.h"

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Concurrency/Thread.h"

#include "Render/Renderer.h"
#include "Render/Texture.h"
#include "Render/Image/ImageSystem.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/VisibilityQueryResults.h"

#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"

namespace DAVA
{
RenderPass::RenderPass(const FastName& _name)
    : passName(_name)
{
    renderLayers.reserve(RenderLayer::RENDER_LAYER_ID_COUNT);

    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.colorBuffer[0].clearColor[0] = 0.0f;
    passConfig.colorBuffer[0].clearColor[1] = 0.0f;
    passConfig.colorBuffer[0].clearColor[2] = 0.0f;
    passConfig.colorBuffer[0].clearColor[3] = 1.0f;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = PRIORITY_MAIN_3D;
    passConfig.viewport.x = 0;
    passConfig.viewport.y = 0;
    passConfig.viewport.width = Renderer::GetFramebufferWidth();
    passConfig.viewport.height = Renderer::GetFramebufferHeight();
}

RenderPass::~RenderPass()
{
    ClearLayersArrays();
    for (RenderLayer* layer : renderLayers)
    {
        SafeDelete(layer);
    }
    SafeRelease(multisampledTexture);
}

void RenderPass::AddRenderLayer(RenderLayer* layer, RenderLayer::eRenderLayerID afterLayer)
{
    if (RenderLayer::RENDER_LAYER_INVALID_ID != afterLayer)
    {
        uint32 size = static_cast<uint32>(renderLayers.size());
        for (uint32 i = 0; i < size; ++i)
        {
            RenderLayer::eRenderLayerID layerID = renderLayers[i]->GetRenderLayerID();
            if (afterLayer == layerID)
            {
                renderLayers.insert(renderLayers.begin() + i + 1, layer);
                layersBatchArrays[layerID].SetSortingFlags(layer->GetSortingFlags());
                return;
            }
        }
        DVASSERT(0 && "RenderPass::AddRenderLayer afterLayer not found");
    }
    else
    {
        renderLayers.push_back(layer);
        layersBatchArrays[layer->GetRenderLayerID()].SetSortingFlags(layer->GetSortingFlags());
    }
}

void RenderPass::RemoveRenderLayer(RenderLayer* layer)
{
    Vector<RenderLayer*>::iterator it = std::find(renderLayers.begin(), renderLayers.end(), layer);
    DVASSERT(it != renderLayers.end());

    renderLayers.erase(it);
}

void RenderPass::SetupCameraParams(Camera* mainCamera, Camera* drawCamera, Vector4* externalClipPlane)
{
    DVASSERT(drawCamera);
    DVASSERT(mainCamera);

    bool needInvertCamera = rhi::NeedInvertProjection(passConfig);
    passConfig.invertCulling = needInvertCamera ? 1 : 0;

    drawCamera->SetupDynamicParameters(needInvertCamera, externalClipPlane);
    if (mainCamera != drawCamera)
        mainCamera->PrepareDynamicParameters(needInvertCamera, externalClipPlane);
}

void RenderPass::Draw(RenderSystem* renderSystem)
{
    Camera* mainCamera = renderSystem->GetMainCamera();
    Camera* drawCamera = renderSystem->GetDrawCamera();
    SetupCameraParams(mainCamera, drawCamera);

    PrepareVisibilityArrays(mainCamera, renderSystem);

    if (BeginRenderPass())
    {
        DrawLayers(mainCamera);
        EndRenderPass();
    }
}

void RenderPass::PrepareVisibilityArrays(Camera* camera, RenderSystem* renderSystem)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::RENDER_PASS_PREPARE_ARRAYS)

    uint32 currVisibilityCriteria = RenderObject::CLIPPING_VISIBILITY_CRITERIA;
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::ENABLE_STATIC_OCCLUSION))
        currVisibilityCriteria &= ~RenderObject::VISIBLE_STATIC_OCCLUSION;

    visibilityArray.clear();
    renderSystem->GetRenderHierarchy()->Clip(camera, visibilityArray, currVisibilityCriteria);

    ClearLayersArrays();
    PrepareLayersArrays(visibilityArray, camera);
}

void RenderPass::PrepareLayersArrays(const Vector<RenderObject*> objectsArray, Camera* camera)
{
    size_t size = objectsArray.size();
    for (size_t ro = 0; ro < size; ++ro)
    {
        RenderObject* renderObject = objectsArray[ro];
        if (renderObject->GetFlags() & RenderObject::CUSTOM_PREPARE_TO_RENDER)
        {
            renderObject->PrepareToRender(camera);
        }

        uint32 batchCount = renderObject->GetActiveRenderBatchCount();
        for (uint32 batchIndex = 0; batchIndex < batchCount; ++batchIndex)
        {
            RenderBatch* batch = renderObject->GetActiveRenderBatch(batchIndex);

            NMaterial* material = batch->GetMaterial();
            DVASSERT(material);
            if (material->PreBuildMaterial(passName))
            {
                layersBatchArrays[material->GetRenderLayerID()].AddRenderBatch(batch);
            }
        }
    }
}

void RenderPass::DrawLayers(Camera* camera)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::RENDER_PASS_DRAW_LAYERS)

    ShaderDescriptorCache::ClearDynamicBindigs();

    //per pass viewport bindings
    viewportSize = Vector2(viewport.dx, viewport.dy);
    rcpViewportSize = Vector2(1.0f / viewport.dx, 1.0f / viewport.dy);
    viewportOffset = Vector2(viewport.x, viewport.y);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEWPORT_SIZE, &viewportSize, reinterpret_cast<pointer_size>(&viewportSize));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_RCP_VIEWPORT_SIZE, &rcpViewportSize, reinterpret_cast<pointer_size>(&rcpViewportSize));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEWPORT_OFFSET, &viewportOffset, reinterpret_cast<pointer_size>(&viewportOffset));

    size_t size = renderLayers.size();
    for (size_t k = 0; k < size; ++k)
    {
        RenderLayer* layer = renderLayers[k];
        RenderBatchArray& batchArray = layersBatchArrays[layer->GetRenderLayerID()];
        batchArray.Sort(camera);

        layer->Draw(camera, batchArray, packetList);
    }
}

void RenderPass::DrawDebug(Camera* camera, RenderSystem* renderSystem)
{
    if (!renderSystem->GetDebugDrawer()->IsEmpty())
    {
        renderSystem->GetDebugDrawer()->Present(packetList, &camera->GetMatrix(), &camera->GetProjectionMatrix());
        renderSystem->GetDebugDrawer()->Clear();
    }
}

void RenderPass::SetRenderTargetProperties(uint32 width, uint32 height, PixelFormat format)
{
    renderTargetProperties.width = width;
    renderTargetProperties.height = height;
    renderTargetProperties.format = format;
}

void RenderPass::ValidateMultisampledTextures(const rhi::RenderPassConfig& config)
{
    uint32 requestedSamples = rhi::TextureSampleCountForAAType(config.antialiasingType);

    bool invalidDescription =
    (multisampledDescription.sampleCount != requestedSamples) ||
    (multisampledDescription.format != renderTargetProperties.format) ||
    (multisampledDescription.width != renderTargetProperties.width) ||
    (multisampledDescription.height != renderTargetProperties.height);

    if (invalidDescription || (multisampledTexture == nullptr))
    {
        SafeRelease(multisampledTexture);

        multisampledDescription.width = renderTargetProperties.width;
        multisampledDescription.height = renderTargetProperties.height;
        multisampledDescription.format = renderTargetProperties.format;
        multisampledDescription.needDepth = true;
        multisampledDescription.needPixelReadback = false;
        multisampledDescription.ensurePowerOf2 = false;
        multisampledDescription.sampleCount = requestedSamples;

        multisampledTexture = Texture::CreateFBO(multisampledDescription);
    }
}

bool RenderPass::BeginRenderPass()
{
    bool success = false;

#ifdef __DAVAENGINE_RENDERSTATS__
    passConfig.queryBuffer = VisibilityQueryResults::GetQueryBuffer();
#endif

    DVASSERT(renderTargetProperties.width > 0);
    DVASSERT(renderTargetProperties.height > 0);
    DVASSERT(renderTargetProperties.format != PixelFormat::FORMAT_INVALID);

    if (passConfig.antialiasingType != rhi::AntialiasingType::NONE)
    {
        ValidateMultisampledTextures(passConfig);
        passConfig.colorBuffer[0].multisampleTexture = multisampledTexture->handle;
        passConfig.depthStencilBuffer.multisampleTexture = multisampledTexture->handleDepthStencil;
    }

    renderPass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    if (renderPass != rhi::InvalidHandle)
    {
        rhi::BeginRenderPass(renderPass);
        rhi::BeginPacketList(packetList);
        success = true;
    }

    return success;
}

void RenderPass::EndRenderPass()
{
    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(renderPass);
}

void RenderPass::ClearLayersArrays()
{
    for (uint32 id = 0; id < static_cast<uint32>(RenderLayer::RENDER_LAYER_ID_COUNT); ++id)
    {
        layersBatchArrays[id].Clear();
    }
}

MainForwardRenderPass::MainForwardRenderPass(const FastName& name)
    : RenderPass(name)
    , reflectionPass(nullptr)
    , refractionPass(nullptr)
{
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_OPAQUE_ID, RenderLayer::LAYER_SORTING_FLAGS_OPAQUE));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_OPAQUE_ID, RenderLayer::LAYER_SORTING_FLAGS_AFTER_OPAQUE));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_VEGETATION_ID, RenderLayer::LAYER_SORTING_FLAGS_VEGETATION));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_ALPHA_TEST_LAYER_ID, RenderLayer::LAYER_SORTING_FLAGS_ALPHA_TEST_LAYER));
    AddRenderLayer(new ShadowVolumeRenderLayer(RenderLayer::RENDER_LAYER_SHADOW_VOLUME_ID, RenderLayer::LAYER_SORTING_FLAGS_SHADOW_VOLUME));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_WATER_ID, RenderLayer::LAYER_SORTING_FLAGS_WATER));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_TRANSLUCENT_ID, RenderLayer::LAYER_SORTING_FLAGS_TRANSLUCENT));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_TRANSLUCENT_ID, RenderLayer::LAYER_SORTING_FLAGS_AFTER_TRANSLUCENT));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_DEBUG_DRAW_ID, RenderLayer::LAYER_SORTING_FLAGS_DEBUG_DRAW));

    passConfig.priority = PRIORITY_MAIN_3D;
}

void MainForwardRenderPass::InitReflectionRefraction()
{
    DVASSERT(!reflectionPass);

    reflectionPass = new WaterReflectionRenderPass(PASS_REFLECTION_REFRACTION);
    reflectionPass->GetPassConfig().colorBuffer[0].texture = Renderer::GetRuntimeTextures().GetDynamicTexture(RuntimeTextures::TEXTURE_DYNAMIC_REFLECTION);
    reflectionPass->GetPassConfig().colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    reflectionPass->GetPassConfig().colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    reflectionPass->GetPassConfig().depthStencilBuffer.texture = Renderer::GetRuntimeTextures().GetDynamicTexture(RuntimeTextures::TEXTURE_DYNAMIC_RR_DEPTHBUFFER);
    reflectionPass->GetPassConfig().depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    reflectionPass->GetPassConfig().depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    reflectionPass->SetViewport(Rect(0, 0, static_cast<float32>(RuntimeTextures::REFLECTION_TEX_SIZE), static_cast<float32>(RuntimeTextures::REFLECTION_TEX_SIZE)));
    reflectionPass->SetRenderTargetProperties(RuntimeTextures::REFLECTION_TEX_SIZE, RuntimeTextures::REFLECTION_TEX_SIZE, Renderer::GetRuntimeTextures().GetDynamicTextureFormat(RuntimeTextures::TEXTURE_DYNAMIC_REFLECTION));

    refractionPass = new WaterRefractionRenderPass(PASS_REFLECTION_REFRACTION);
    refractionPass->GetPassConfig().colorBuffer[0].texture = Renderer::GetRuntimeTextures().GetDynamicTexture(RuntimeTextures::TEXTURE_DYNAMIC_REFRACTION);
    refractionPass->GetPassConfig().colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    refractionPass->GetPassConfig().colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    refractionPass->GetPassConfig().depthStencilBuffer.texture = Renderer::GetRuntimeTextures().GetDynamicTexture(RuntimeTextures::TEXTURE_DYNAMIC_RR_DEPTHBUFFER);
    refractionPass->GetPassConfig().depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    refractionPass->GetPassConfig().depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    refractionPass->SetViewport(Rect(0, 0, static_cast<float32>(RuntimeTextures::REFRACTION_TEX_SIZE), static_cast<float32>(RuntimeTextures::REFRACTION_TEX_SIZE)));
    refractionPass->SetRenderTargetProperties(RuntimeTextures::REFRACTION_TEX_SIZE, RuntimeTextures::REFRACTION_TEX_SIZE, Renderer::GetRuntimeTextures().GetDynamicTextureFormat(RuntimeTextures::TEXTURE_DYNAMIC_REFRACTION));
}

void MainForwardRenderPass::PrepareReflectionRefractionTextures(RenderSystem* renderSystem)
{
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::WATER_REFLECTION_REFRACTION_DRAW))
        return;

    if (!reflectionPass)
        InitReflectionRefraction();

    const RenderBatchArray& waterLayerBatches = layersBatchArrays[RenderLayer::RENDER_LAYER_WATER_ID];
    uint32 waterBatchesCount = waterLayerBatches.GetRenderBatchCount();
    if (waterBatchesCount)
    {
        waterBox.Empty();
        for (uint32 i = 0; i < waterBatchesCount; ++i)
        {
            RenderBatch* batch = waterLayerBatches.Get(i);
            waterBox.AddAABBox(batch->GetRenderObject()->GetWorldBoundingBox());
        }
    }

    const float32* clearColor = static_cast<const float32*>(Renderer::GetDynamicBindings().GetDynamicParam(DynamicBindings::PARAM_WATER_CLEAR_COLOR));

    for (int32 i = 0; i < 4; ++i)
    {
        reflectionPass->GetPassConfig().colorBuffer[0].clearColor[i] = clearColor[i];
        refractionPass->GetPassConfig().colorBuffer[0].clearColor[i] = clearColor[i];
    }

    reflectionPass->SetWaterLevel(waterBox.max.z);
    reflectionPass->GetPassConfig().priority = passConfig.priority + PRIORITY_SERVICE_3D;
    reflectionPass->Draw(renderSystem);

    refractionPass->SetWaterLevel(waterBox.min.z);
    refractionPass->GetPassConfig().priority = passConfig.priority + PRIORITY_SERVICE_3D;
    refractionPass->Draw(renderSystem);
}

void MainForwardRenderPass::Draw(RenderSystem* renderSystem)
{
    Camera* mainCamera = renderSystem->GetMainCamera();
    Camera* drawCamera = renderSystem->GetDrawCamera();

    /*    drawCamera->SetPosition(Vector3(5, 5, 5));
    drawCamera->SetTarget(Vector3(0, 0, 0));
    Vector4 clip(0, 0, 1, -1);*/
    SetupCameraParams(mainCamera, drawCamera);

    PrepareVisibilityArrays(mainCamera, renderSystem);

    DAVA_PROFILER_GPU_RENDER_PASS(passConfig, ProfilerGPUMarkerName::RENDER_PASS_MAIN_3D);
    if (BeginRenderPass())
    {
        DrawLayers(mainCamera);

        if (layersBatchArrays[RenderLayer::RENDER_LAYER_WATER_ID].GetRenderBatchCount() != 0)
            PrepareReflectionRefractionTextures(renderSystem);

        DrawDebug(drawCamera, renderSystem);

        EndRenderPass();
    }
}

MainForwardRenderPass::~MainForwardRenderPass()
{
    SafeDelete(reflectionPass);
    SafeDelete(refractionPass);
}

WaterPrePass::WaterPrePass(const FastName& name)
    : RenderPass(name)
    , passMainCamera(NULL)
    , passDrawCamera(NULL)
{
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_OPAQUE_ID, RenderLayer::LAYER_SORTING_FLAGS_OPAQUE));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_OPAQUE_ID, RenderLayer::LAYER_SORTING_FLAGS_AFTER_OPAQUE));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_ALPHA_TEST_LAYER_ID, RenderLayer::LAYER_SORTING_FLAGS_ALPHA_TEST_LAYER));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_TRANSLUCENT_ID, RenderLayer::LAYER_SORTING_FLAGS_TRANSLUCENT));
    AddRenderLayer(new RenderLayer(RenderLayer::RENDER_LAYER_AFTER_TRANSLUCENT_ID, RenderLayer::LAYER_SORTING_FLAGS_AFTER_TRANSLUCENT));

    passConfig.priority = PRIORITY_SERVICE_3D;
}
WaterPrePass::~WaterPrePass()
{
    SafeRelease(passMainCamera);
    SafeRelease(passDrawCamera);
}

WaterReflectionRenderPass::WaterReflectionRenderPass(const FastName& name)
    : WaterPrePass(name)
{
}

void WaterReflectionRenderPass::UpdateCamera(Camera* camera)
{
    Vector3 v;
    v = camera->GetPosition();
    v.z = waterLevel - (v.z - waterLevel);
    camera->SetPosition(v);
    v = camera->GetTarget();
    v.z = waterLevel - (v.z - waterLevel);
    camera->SetTarget(v);
}

void WaterReflectionRenderPass::Draw(RenderSystem* renderSystem)
{
    Camera* mainCamera = renderSystem->GetMainCamera();
    Camera* drawCamera = renderSystem->GetDrawCamera();

    if (!passDrawCamera)
    {
        passMainCamera = new Camera();
        passDrawCamera = new Camera();
    }

    passMainCamera->CopyMathOnly(*mainCamera);
    UpdateCamera(passMainCamera);

    Vector4 clipPlane(0, 0, 1, -(waterLevel - 0.1f));
    Camera* currMainCamera = passMainCamera;
    Camera* currDrawCamera;

    if (drawCamera == mainCamera)
    {
        currDrawCamera = currMainCamera;
    }
    else
    {
        passDrawCamera->CopyMathOnly(*drawCamera);
        UpdateCamera(passDrawCamera);
        currDrawCamera = passDrawCamera;
    }

    SetupCameraParams(currMainCamera, currDrawCamera, &clipPlane);

    visibilityArray.clear();
    renderSystem->GetRenderHierarchy()->Clip(currMainCamera, visibilityArray, RenderObject::CLIPPING_VISIBILITY_CRITERIA | RenderObject::VISIBLE_REFLECTION);
    ClearLayersArrays();
    PrepareLayersArrays(visibilityArray, currMainCamera);

    DAVA_PROFILER_GPU_RENDER_PASS(passConfig, ProfilerGPUMarkerName::RENDER_PASS_WATER_REFLECTION);
    if (BeginRenderPass())
    {
        DrawLayers(currMainCamera);
        EndRenderPass();
    }
}

WaterRefractionRenderPass::WaterRefractionRenderPass(const FastName& name)
    : WaterPrePass(name)
{
    /*const RenderLayerManager * renderLayerManager = RenderLayerManager::Instance();
    AddRenderLayer(renderLayerManager->GetRenderLayer(LAYER_SHADOW_VOLUME), LAST_LAYER);*/
}

void WaterRefractionRenderPass::Draw(RenderSystem* renderSystem)
{
    Camera* mainCamera = renderSystem->GetMainCamera();
    Camera* drawCamera = renderSystem->GetDrawCamera();

    if (!passDrawCamera)
    {
        passMainCamera = new Camera();
        passDrawCamera = new Camera();
    }

    passMainCamera->CopyMathOnly(*mainCamera);

    //-0.1f ?
    //Vector4 clipPlane(0,0, -1, waterLevel*3);
    Vector4 clipPlane(0, 0, -1, waterLevel + 0.1f);

    Camera* currMainCamera = passMainCamera;
    Camera* currDrawCamera;

    if (drawCamera == mainCamera)
    {
        currDrawCamera = currMainCamera;
    }
    else
    {
        passDrawCamera->CopyMathOnly(*drawCamera);
        currDrawCamera = passDrawCamera;
    }

    SetupCameraParams(currMainCamera, currDrawCamera, &clipPlane);

    visibilityArray.clear();
    renderSystem->GetRenderHierarchy()->Clip(currMainCamera, visibilityArray, RenderObject::CLIPPING_VISIBILITY_CRITERIA | RenderObject::VISIBLE_REFRACTION);
    ClearLayersArrays();
    PrepareLayersArrays(visibilityArray, currMainCamera);

    DAVA_PROFILER_GPU_RENDER_PASS(passConfig, ProfilerGPUMarkerName::RENDER_PASS_WATER_REFRACTION);
    if (BeginRenderPass())
    {
        DrawLayers(currMainCamera);
        EndRenderPass();
    }
}
};
