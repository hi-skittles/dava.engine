#include "RenderSystem2D.h"

#include "Engine/Engine.h"
#include "UI/UIControl.h"
#include "UI/UIControlBackground.h"
#include "UI/UIControlSystem.h"

#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/DynamicBufferAllocator.h"
#include "Render/Material/NMaterial.h"
#include "Render/Renderer.h"
#include "Render/ShaderCache.h"
#include "Render/VisibilityQueryResults.h"

#include "Time/SystemTimer.h"

#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"

#include "Logger/Logger.h"

namespace DAVA
{
namespace
{
const bool virtualToPhysicalTransformEnabledDefaultValue = true;

const uint32 MAX_VERTICES = 1024;
const uint32 MAX_INDECES = MAX_VERTICES * 2;
const float32 SEGMENT_LENGTH = 15.0f;
}

const FastName RenderSystem2D::RENDER_PASS_NAME("2d");
const FastName RenderSystem2D::FLAG_COLOR_OP("COLOR_OP");
const FastName RenderSystem2D::FLAG_GRADIENT_MODE = FastName("GRADIENT_MODE");

NMaterial* RenderSystem2D::DEFAULT_2D_COLOR_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_2D_FILL_ALPHA_MATERIAL = nullptr;
NMaterial* RenderSystem2D::DEFAULT_COMPOSIT_MATERIAL[] = { nullptr };

RenderSystem2D::RenderSystem2D()
{
    viewMatrixSemantic = 8; //0 is bad idea as it is same as UPDATE_SEMANTIC_ALWAYS. why 8 - see comment in Setup2DMatrixes
    projMatrixSemantic = 8;

    spriteClipping = true;
}

void RenderSystem2D::Init()
{
    DEFAULT_2D_COLOR_MATERIAL = new NMaterial();
    DEFAULT_2D_COLOR_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Color.material"));
    DEFAULT_2D_COLOR_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.Alphablend.material"));
    DEFAULT_2D_TEXTURE_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.Alphablend.material"));
    DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL->AddFlag(NMaterialFlagName::FLAG_BLENDING, BLENDING_PREMULTIPLIED_ALPHA);
    DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.Alphablend.material"));
    DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL->AddFlag(NMaterialFlagName::FLAG_BLENDING, BLENDING_ADDITIVE);
    DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.material"));
    DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.Alpha8.material"));
    DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL = new NMaterial();
    DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL->SetFXName(FastName("~res:/Materials/2d.Textured.Grayscale.material"));
    DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    DEFAULT_2D_FILL_ALPHA_MATERIAL = new NMaterial();
    DEFAULT_2D_FILL_ALPHA_MATERIAL->SetFXName(FastName("~res:/Materials/2d.AlphaFill.material"));
    DEFAULT_2D_FILL_ALPHA_MATERIAL->PreBuildMaterial(RENDER_PASS_NAME);

    for (int32 i = 0; i < GRADIENT_BLEND_MODE_COUNT; i++)
    {
        DEFAULT_COMPOSIT_MATERIAL[i] = new NMaterial();
        DEFAULT_COMPOSIT_MATERIAL[i]->SetFXName(FastName("~res:/Materials/2d.Composit.material"));
        DEFAULT_COMPOSIT_MATERIAL[i]->AddFlag(FLAG_GRADIENT_MODE, i);
        DEFAULT_COMPOSIT_MATERIAL[i]->PreBuildMaterial(RENDER_PASS_NAME);
    }

    rhi::VertexLayout noTextureLayout;
    noTextureLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    noTextureLayout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);
    vertexLayouts2d[0] = rhi::VertexLayout::UniqueId(noTextureLayout);
    VBO_STRIDE[0] = 3 * sizeof(float32) + 4;

    rhi::VertexLayout layout;
    layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    layout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    layout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);
    vertexLayouts2d[1] = rhi::VertexLayout::UniqueId(layout);
    VBO_STRIDE[1] = 3 * sizeof(float32) + 2 * sizeof(float32) + 4; //position, uv, color
    for (uint32 i = 2; i <= BatchDescriptor2D::MAX_TEXTURE_STREAMS_COUNT; ++i)
    {
        layout.AddElement(rhi::VS_TEXCOORD, i - 1, rhi::VDT_FLOAT, 2);
        vertexLayouts2d[i] = rhi::VertexLayout::UniqueId(layout);
        VBO_STRIDE[i] = 3 * sizeof(float32) + 2 * sizeof(float32) * i + 4;
    }

    currentPacket.primitiveCount = 0;
    currentPacket.vertexStreamCount = 1;
    currentPacket.options = 0;
    currentPacket.vertexLayoutUID = vertexLayouts2d[1];

    vertexIndex = 0;
    indexIndex = 0;

    lastMaterial = nullptr;
    lastUsedCustomWorldMatrix = false;
    lastCustomWorldMatrix = Matrix4::IDENTITY;
    lastClip = Rect(0, 0, -1, -1);

    currentVertexBuffer.reserve(MAX_VERTICES * VBO_STRIDE[1]);
    currentIndexBuffer.reserve(MAX_INDECES);
}

RenderSystem2D::~RenderSystem2D()
{
    SafeRelease(DEFAULT_2D_COLOR_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL);
    SafeRelease(DEFAULT_2D_FILL_ALPHA_MATERIAL);
    SafeRelease(DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL);
}

void RenderSystem2D::BeginFrame()
{
    currentClip.x = 0;
    currentClip.y = 0;
    currentClip.dx = -1;
    currentClip.dy = -1;

    defaultSpriteDrawState.Reset();
    defaultSpriteDrawState.material = DEFAULT_2D_COLOR_MATERIAL;

    rhi::RenderPassConfig renderPass2DConfig;
    renderPass2DConfig.priority = PRIORITY_MAIN_2D + mainTargetDescriptor.priority;
    renderPass2DConfig.colorBuffer[0].texture = mainTargetDescriptor.colorAttachment;
    renderPass2DConfig.colorBuffer[0].loadAction = mainTargetDescriptor.clearTarget ? rhi::LOADACTION_CLEAR : rhi::LOADACTION_LOAD;
    renderPass2DConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    Memcpy(renderPass2DConfig.colorBuffer[0].clearColor, mainTargetDescriptor.clearColor.color, sizeof(Color));
    renderPass2DConfig.depthStencilBuffer.texture = mainTargetDescriptor.depthAttachment ? mainTargetDescriptor.depthAttachment : rhi::DefaultDepthBuffer;
    renderPass2DConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    renderPass2DConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    renderPass2DConfig.viewport.x = renderPass2DConfig.viewport.y = 0;
    renderPass2DConfig.viewport.width = Renderer::GetFramebufferWidth();
    renderPass2DConfig.viewport.height = Renderer::GetFramebufferHeight();
    DAVA_PROFILER_GPU_RENDER_PASS(renderPass2DConfig, ProfilerGPUMarkerName::RENDER_PASS_2D);
#ifdef __DAVAENGINE_RENDERSTATS__
    renderPass2DConfig.queryBuffer = VisibilityQueryResults::GetQueryBuffer();
#endif

    pass2DHandle = rhi::AllocateRenderPass(renderPass2DConfig, 1, &packetList2DHandle);
    currentPacketListHandle = packetList2DHandle;

    if (pass2DHandle != rhi::InvalidHandle)
    {
        rhi::BeginRenderPass(pass2DHandle);
        rhi::BeginPacketList(currentPacketListHandle);
    }
    else
    {
        pass2DHandle = rhi::HRenderPass(rhi::InvalidHandle);
        packetList2DHandle = rhi::HPacketList(rhi::InvalidHandle);
        currentPacketListHandle = rhi::HPacketList(rhi::InvalidHandle);
    }

    Setup2DMatrices();

    globalTime += SystemTimer::GetFrameDelta();
}

void RenderSystem2D::EndFrame()
{
    if (pass2DHandle != rhi::InvalidHandle)
    {
        Flush();
        prevFrameErrorsFlags = currFrameErrorsFlags;
        currFrameErrorsFlags = 0;

        rhi::EndPacketList(currentPacketListHandle);
        rhi::EndRenderPass(pass2DHandle);
    }

    prevFrameErrorsFlags = currFrameErrorsFlags;
    currFrameErrorsFlags = 0;

    pass2DHandle = rhi::HRenderPass(rhi::InvalidHandle);
    packetList2DHandle = rhi::HPacketList(rhi::InvalidHandle);
    currentPacketListHandle = rhi::HPacketList(rhi::InvalidHandle);
}

const RenderSystem2D::RenderTargetPassDescriptor& RenderSystem2D::GetActiveTargetDescriptor()
{
    return IsRenderTargetPass() ? renderPassTargetDescriptor : mainTargetDescriptor;
}

const RenderSystem2D::RenderTargetPassDescriptor& RenderSystem2D::GetMainTargetDescriptor()
{
    return mainTargetDescriptor;
}

void RenderSystem2D::SetMainTargetDescriptor(const RenderSystem2D::RenderTargetPassDescriptor& descriptor)
{
    mainTargetDescriptor = descriptor;
}

void RenderSystem2D::BeginRenderTargetPass(Texture* target, bool needClear /* = true */, const Color& clearColor /* = Color::Clear */, int32 priority /* = PRIORITY_SERVICE_2D */)
{
    RenderTargetPassDescriptor desc;
    desc.colorAttachment = target->handle;
    desc.depthAttachment = target->handleDepthStencil;
    desc.format = target->GetFormat();
    desc.width = target->GetWidth();
    desc.height = target->GetHeight();
    desc.clearColor = clearColor;
    desc.priority = priority;
    desc.clearTarget = needClear;
    desc.transformVirtualToPhysical = true;
    BeginRenderTargetPass(desc);
}

void RenderSystem2D::BeginRenderTargetPass(const RenderTargetPassDescriptor& desc)
{
    DVASSERT(!IsRenderTargetPass());

    Flush();

    renderPassTargetDescriptor = desc;

    UpdateVirtualToPhysicalMatrix(desc.transformVirtualToPhysical);

    rhi::RenderPassConfig renderTargetPassConfig;
    renderTargetPassConfig.colorBuffer[0].texture = desc.colorAttachment;
    renderTargetPassConfig.colorBuffer[0].clearColor[0] = desc.clearColor.r;
    renderTargetPassConfig.colorBuffer[0].clearColor[1] = desc.clearColor.g;
    renderTargetPassConfig.colorBuffer[0].clearColor[2] = desc.clearColor.b;
    renderTargetPassConfig.colorBuffer[0].clearColor[3] = desc.clearColor.a;
    renderTargetPassConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    renderTargetPassConfig.colorBuffer[0].loadAction = desc.clearTarget ? rhi::LOADACTION_CLEAR : rhi::LOADACTION_LOAD;
    renderTargetPassConfig.depthStencilBuffer.texture = desc.depthAttachment;
    renderTargetPassConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    renderTargetPassConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    renderTargetPassConfig.priority = desc.priority;
    renderTargetPassConfig.viewport.width = desc.width;
    renderTargetPassConfig.viewport.height = desc.height;

    passTargetHandle = rhi::AllocateRenderPass(renderTargetPassConfig, 1, &currentPacketListHandle);

    rhi::BeginRenderPass(passTargetHandle);
    rhi::BeginPacketList(currentPacketListHandle);

    Setup2DMatrices();
}

void RenderSystem2D::EndRenderTargetPass()
{
    DVASSERT(IsRenderTargetPass());

    Flush();

    rhi::EndPacketList(currentPacketListHandle);
    rhi::EndRenderPass(passTargetHandle);

    currentPacketListHandle = packetList2DHandle;

    UpdateVirtualToPhysicalMatrix(virtualToPhysicalTransformEnabledDefaultValue);

    Setup2DMatrices();
}

void RenderSystem2D::SetViewMatrix(const Matrix4& _viewMatrix)
{
    Flush();

    viewMatrix = _viewMatrix;
    viewMatrixSemantic += 8; //cause the same as at Setup2DMatrices()
}

void RenderSystem2D::Setup2DMatrices()
{
    ShaderDescriptorCache::ClearDynamicBindigs();

    Size2f targetSize;
    const RenderTargetPassDescriptor& descr = GetActiveTargetDescriptor();
    targetSize.dx = static_cast<float32>(descr.width == 0 ? Renderer::GetFramebufferWidth() : descr.width);
    targetSize.dy = static_cast<float32>(descr.height == 0 ? Renderer::GetFramebufferHeight() : descr.height);

    if ((descr.colorAttachment != rhi::InvalidHandle) && (!rhi::DeviceCaps().isUpperLeftRTOrigin))
    {
        //invert projection
        projMatrix.BuildOrtho(0.0f, targetSize.dx, 0.0f, targetSize.dy, -1.0f, 1.0f, rhi::DeviceCaps().isZeroBaseClipRange);
    }
    else
    {
        projMatrix.BuildOrtho(0.0f, targetSize.dx, targetSize.dy, 0.0f, -1.0f, 1.0f, rhi::DeviceCaps().isZeroBaseClipRange);
    }

    if (rhi::DeviceCaps().isCenterPixelMapping)
    {
        // Make translation by half pixel for DirectX systems
        static Matrix4 pixelMappingMatrix = Matrix4::MakeTranslation(Vector3(-0.5f, -0.5f, 0.f));
        projMatrix = currentVirtualToPhysicalMatrix * pixelMappingMatrix * projMatrix;
    }
    else
    {
        projMatrix = currentVirtualToPhysicalMatrix * projMatrix;
    }

    projMatrixSemantic += 8; //cause eight is beautiful
    //actually, is not +=1 cause DynamicParams for UPADATE_ALWAYS_SEMANTIC increment by one last binded value.
    //TODO: need to rethink semantic for projection matrix in RenderSystem2D, or maybe need to rethink semantics for DynamicParams

    //initially setup dynamic params for 2d rendering
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJ, &projMatrix, static_cast<pointer_size>(projMatrixSemantic));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW, &viewMatrix, static_cast<pointer_size>(viewMatrixSemantic));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_GLOBAL_TIME, &globalTime, reinterpret_cast<pointer_size>(&globalTime));
}

void RenderSystem2D::AddPacket(rhi::Packet& packet)
{
    DVASSERT(currentPacketListHandle.IsValid());

#if defined(__DAVAENGINE_RENDERSTATS__)
    ++Renderer::GetRenderStats().packets2d;

#ifdef __DAVAENGINE_RENDERSTATS_ALPHABLEND__
    if (packet.userFlags & NMaterial::USER_FLAG_ALPHABLEND)
        packet.queryIndex = VisibilityQueryResults::QUERY_INDEX_ALPHABLEND;
    else
        packet.queryIndex = DAVA::InvalidIndex;
#else
    packet.queryIndex = VisibilityQueryResults::QUERY_INDEX_UI;
#endif
#endif

    rhi::AddPacket(currentPacketListHandle, packet);
}

void RenderSystem2D::ScreenSizeChanged()
{
    Matrix4 translateMx, scaleMx;

    Vector2 scale = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysical(Vector2(1.f, 1.f));
    Vector2 realDrawOffset = GetEngineContext()->uiControlSystem->vcs->GetPhysicalDrawOffset();

    translateMx.BuildTranslation(Vector3(realDrawOffset.x, realDrawOffset.y, 0.0f));
    scaleMx.BuildScale(Vector3(scale.x, scale.y, 1.0f));

    actualVirtualToPhysicalMatrix = scaleMx * translateMx;
    actualPhysicalToVirtualScale.x = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(1.0f);
    actualPhysicalToVirtualScale.y = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY(1.0f);
    if (GetActiveTargetDescriptor().transformVirtualToPhysical)
    {
        currentVirtualToPhysicalMatrix = actualVirtualToPhysicalMatrix;
        currentPhysicalToVirtualScale = actualPhysicalToVirtualScale;
    }
}

void RenderSystem2D::UpdateVirtualToPhysicalMatrix(bool value)
{
    currentVirtualToPhysicalMatrix = value ? actualVirtualToPhysicalMatrix : Matrix4::IDENTITY;
    currentPhysicalToVirtualScale = value ? actualPhysicalToVirtualScale : Vector2(1.0f, 1.0f);
}

void RenderSystem2D::SetClip(const Rect& rect)
{
    if ((currentClip == rect) || (currentClip.dx < 0 && rect.dx < 0) || (currentClip.dy < 0 && rect.dy < 0))
    {
        return;
    }
    currentClip = rect;
}

void RenderSystem2D::RemoveClip()
{
    SetClip(Rect(0.f, 0.f, -1.f, -1.f));
}

void RenderSystem2D::IntersectClipRect(const Rect& rect)
{
    if (currentClip.dx < 0 || currentClip.dy < 0)
    {
        VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;
        const RenderTargetPassDescriptor& descr = GetActiveTargetDescriptor();
        Rect screen(0.0f, 0.0f,
                    (descr.width == 0 ? vcs->GetVirtualScreenSize().dx : vcs->ConvertPhysicalToVirtualX(float32(descr.width))),
                    (descr.height == 0 ? vcs->GetVirtualScreenSize().dy : vcs->ConvertPhysicalToVirtualY(float32(descr.height))));
        Rect res = screen.Intersection(rect);
        SetClip(res);
    }
    else
    {
        SetClip(currentClip.Intersection(rect));
    }
}

void RenderSystem2D::PushClip()
{
    clipStack.push(currentClip);
}

void RenderSystem2D::PopClip()
{
    if (clipStack.empty())
    {
        Rect r(0, 0, -1, -1);
        SetClip(r);
    }
    else
    {
        Rect r = clipStack.top();
        SetClip(r);
    }
    clipStack.pop();
}

Rect RenderSystem2D::TransformClipRect(const Rect& rect, const Matrix4& transformMatrix)
{
    Vector3 clipTopLeftCorner(rect.x, rect.y, 0.f);
    Vector3 clipBottomRightCorner(rect.x + rect.dx, rect.y + rect.dy, 0.f);
    clipTopLeftCorner = clipTopLeftCorner * transformMatrix;
    clipBottomRightCorner = clipBottomRightCorner * transformMatrix;
    Rect resRect = Rect(Vector2(clipTopLeftCorner.data), Vector2((clipBottomRightCorner - clipTopLeftCorner).data));
    if (resRect.x < 0.f)
    {
        resRect.x = 0;
    }
    if (resRect.y < 0.f)
    {
        resRect.y = 0;
    }
    return resRect;
}

void RenderSystem2D::SetSpriteClipping(bool clipping)
{
    spriteClipping = clipping;
}

bool RenderSystem2D::GetSpriteClipping() const
{
    return spriteClipping;
}

bool RenderSystem2D::IsPreparedSpriteOnScreen(SpriteDrawState* drawState)
{
    Rect clipRect = currentClip;

    const RenderTargetPassDescriptor& descr = GetActiveTargetDescriptor();
    if (int32(clipRect.dx) == -1)
    {
        clipRect.dx = static_cast<float32>(descr.width == 0 ? GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dx : descr.width);
    }
    if (int32(clipRect.dy) == -1)
    {
        clipRect.dy = static_cast<float32>(descr.height == 0 ? GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize().dy : descr.height);
    }

    float32 left = Min(Min(spriteTempVertices[0], spriteTempVertices[2]), Min(spriteTempVertices[4], spriteTempVertices[6]));
    float32 right = Max(Max(spriteTempVertices[0], spriteTempVertices[2]), Max(spriteTempVertices[4], spriteTempVertices[6]));
    float32 top = Min(Min(spriteTempVertices[1], spriteTempVertices[3]), Min(spriteTempVertices[5], spriteTempVertices[7]));
    float32 bottom = Max(Max(spriteTempVertices[1], spriteTempVertices[3]), Max(spriteTempVertices[5], spriteTempVertices[7]));

    const Rect spriteRect(left, top, right - left, bottom - top);
    return clipRect.RectIntersects(spriteRect);
}

void RenderSystem2D::Flush()
{
    /*
    Called on each EndFrame, particle draw, screen transitions preparing, screen borders draw and changing state
    */

    if (vertexIndex == 0 && indexIndex == 0)
    {
        return;
    }

    DynamicBufferAllocator::AllocResultVB vertexBuffer = DynamicBufferAllocator::AllocateVertexBuffer(GetVBOStride(currentTexcoordStreamCount), vertexIndex);
    DynamicBufferAllocator::AllocResultIB indexBuffer = DynamicBufferAllocator::AllocateIndexBuffer(indexIndex);
    DVASSERT(vertexBuffer.allocatedVertices == vertexIndex);
    DVASSERT(indexBuffer.allocatedindices == indexIndex);
    Memcpy(vertexBuffer.data, currentVertexBuffer.data(), GetVBOStride(currentTexcoordStreamCount) * vertexIndex);
    Memcpy(indexBuffer.data, currentIndexBuffer.data(), indexIndex * 2);

    currentPacket.vertexStream[0] = vertexBuffer.buffer;
    currentPacket.vertexCount = vertexBuffer.allocatedVertices;
    currentPacket.baseVertex = vertexBuffer.baseVertex;
    currentPacket.indexBuffer = indexBuffer.buffer;
    currentPacket.startIndex = indexBuffer.baseIndex;

    if (currentPacketListHandle != rhi::InvalidHandle && currentPacket.primitiveCount > 0)
    {
        AddPacket(currentPacket);
    }

    currentVertexBuffer.clear();
    currentIndexBuffer.clear();

    currentPacket.vertexStream[0] = rhi::HVertexBuffer();
    currentPacket.vertexCount = 0;
    currentPacket.baseVertex = 0;
    currentPacket.indexBuffer = rhi::HIndexBuffer();
    currentPacket.primitiveCount = 0;

    vertexIndex = 0;
    indexIndex = 0;
    lastMaterial = nullptr;
}

void RenderSystem2D::DrawPacket(rhi::Packet& packet)
{
    if (currentClip.dx == 0.f || currentClip.dy == 0.f)
    {
        // Ignore draw if clip has zero width or height
        return;
    }
    Flush();
    if (currentClip.dx > 0.f && currentClip.dy > 0.f)
    {
        const Rect& transformedClipRect = TransformClipRect(currentClip, currentVirtualToPhysicalMatrix);
        packet.scissorRect.x = static_cast<int16>(transformedClipRect.x + 0.5f);
        packet.scissorRect.y = static_cast<int16>(transformedClipRect.y + 0.5f);
        packet.scissorRect.width = static_cast<int16>(std::ceil(transformedClipRect.dx));
        packet.scissorRect.height = static_cast<int16>(std::ceil(transformedClipRect.dy));
        packet.options |= rhi::Packet::OPT_OVERRIDE_SCISSOR;
    }

    if (currentPacketListHandle != rhi::InvalidHandle)
        AddPacket(packet);
}

void RenderSystem2D::PushBatch(const BatchDescriptor2D& batchDesc)
{
    DVASSERT(batchDesc.vertexPointer != nullptr && batchDesc.vertexStride > 0 && batchDesc.vertexCount > 0, "Incorrect vertex position data");
    DVASSERT(batchDesc.indexPointer != nullptr && batchDesc.indexCount > 0, "Incorrect index data");
    DVASSERT(batchDesc.material != nullptr, "Incorrect material");
    DVASSERT((batchDesc.samplerStateHandle != rhi::InvalidHandle && batchDesc.textureSetHandle != rhi::InvalidHandle) ||
             (batchDesc.samplerStateHandle == rhi::InvalidHandle && batchDesc.textureSetHandle == rhi::InvalidHandle),
             "Incorrect textureSet or samplerState handle");

    DVASSERT(batchDesc.texCoordPointer[0] == nullptr || batchDesc.texCoordStride > 0, "Incorrect vertex texture coordinates data");
    DVASSERT(batchDesc.colorPointer == nullptr || batchDesc.colorStride > 0, "Incorrect vertex color data");

    if (batchDesc.vertexCount == 0 && batchDesc.indexCount == 0)
    {
        // Ignore draw for empty geometry
        return;
    }

    if (currentClip.dx == 0.f || currentClip.dy == 0.f)
    {
        // Ignore draw if clip has zero width or height.
        // For disable clip and this check use Rect(0,0,-1,-1)
        return;
    }

#if defined(__DAVAENGINE_RENDERSTATS__)
    ++Renderer::GetRenderStats().batches2d;
#endif
    uint32 trimmedTexCoordCount = Max(batchDesc.texCoordCount, 1u); //for zero texCoordCount count we just use 1 empty texcoord stream for batching optimization
    if ((vertexIndex + batchDesc.vertexCount > MAX_VERTICES) || (indexIndex + batchDesc.indexCount > MAX_INDECES) || (trimmedTexCoordCount != currentTexcoordStreamCount))
    {
        // Buffer overflow or format changed. Switch to next VBO.
        Flush();
        currentTexcoordStreamCount = trimmedTexCoordCount;
        currentPacket.vertexLayoutUID = GetVertexLayoutId(currentTexcoordStreamCount);

        // TODO: Make draw for big buffers (bigger than buffers in pool)
        // Draw immediately if batch is too big to buffer
        if (batchDesc.vertexCount > MAX_VERTICES || batchDesc.indexCount > MAX_INDECES)
        {
            if (((prevFrameErrorsFlags & BUFFER_OVERFLOW_ERROR) != BUFFER_OVERFLOW_ERROR))
            {
                Logger::Warning("PushBatch: Too much vertices (%d of %d)! Direct draw.", batchDesc.vertexCount, MAX_VERTICES);
            }
            currFrameErrorsFlags |= BUFFER_OVERFLOW_ERROR;
        }
    }

    // Begin check world matrix
    bool needUpdateWorldMatrix = false;
    bool useCustomWorldMatrix = batchDesc.worldMatrix != nullptr;
    if (!useCustomWorldMatrix && !lastUsedCustomWorldMatrix) // Equal and False
    {
        // Skip check world matrices. Use Matrix4::IDENTITY. (the most frequent option)
    }
    else if (useCustomWorldMatrix && lastUsedCustomWorldMatrix) // Equal and True
    {
        if (lastCustomWorldMatrix != *batchDesc.worldMatrix) // Update only if matrices not equal
        {
            needUpdateWorldMatrix = true;
            lastCustomWorldMatrix = *batchDesc.worldMatrix;
        }
    }
    else // Not equal
    {
        needUpdateWorldMatrix = true;
        if (useCustomWorldMatrix)
        {
            lastCustomWorldMatrix = *batchDesc.worldMatrix;
        }
    }
    // End check world matrix

    // Begin new packet
    if (currentPacket.textureSet != batchDesc.textureSetHandle || currentPacket.primitiveType != batchDesc.primitiveType || lastMaterial != batchDesc.material || lastClip != currentClip || needUpdateWorldMatrix)
    {
        Flush();
        if (useCustomWorldMatrix)
        {
            Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, &lastCustomWorldMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
        }
        else
        {
            Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, &Matrix4::IDENTITY, reinterpret_cast<pointer_size>(&Matrix4::IDENTITY));
        }
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJ, &projMatrix, static_cast<pointer_size>(projMatrixSemantic));
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW, &viewMatrix, static_cast<pointer_size>(viewMatrixSemantic));
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_GLOBAL_TIME, &globalTime, reinterpret_cast<pointer_size>(&globalTime));

        if (currentClip.dx > 0.f && currentClip.dy > 0.f)
        {
            const Rect& transformedClipRect = TransformClipRect(currentClip, currentVirtualToPhysicalMatrix);
            currentPacket.scissorRect.x = static_cast<int16>(std::floor(transformedClipRect.x));
            currentPacket.scissorRect.y = static_cast<int16>(std::floor(transformedClipRect.y));
            currentPacket.scissorRect.width = static_cast<int16>(std::ceil(transformedClipRect.dx));
            currentPacket.scissorRect.height = static_cast<int16>(std::ceil(transformedClipRect.dy));
            currentPacket.options |= rhi::Packet::OPT_OVERRIDE_SCISSOR;
        }
        else
        {
            currentPacket.options &= ~rhi::Packet::OPT_OVERRIDE_SCISSOR;
        }
        lastClip = currentClip;

        currentPacket.primitiveType = batchDesc.primitiveType;

        DVASSERT(batchDesc.material);
        lastMaterial = batchDesc.material;
        lastMaterial->BindParams(currentPacket);
        currentPacket.textureSet = batchDesc.textureSetHandle;
        currentPacket.samplerState = batchDesc.samplerStateHandle;
    }
    // End new packet

    // Begin define draw color
    Color useColor = batchDesc.singleColor;
    if (highlightControlsVerticesLimit > 0 && batchDesc.vertexCount > highlightControlsVerticesLimit && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::HIGHLIGHT_HARD_CONTROLS))
    {
        // Highlight too big controls with magenta color
        static Color magenta = Color(1.f, 0.f, 1.f, 1.f);
        useColor = magenta;
    }
    uint32 useColorRGBA = rhi::NativeColorRGBA(useColor.r, useColor.g, useColor.b, useColor.a);
    // End define draw color

    // Prepare vertex color ptr (batchDesc.singleColor or batchDesc.colorPointer)
    const uint32* colorPtr = batchDesc.colorPointer;
    uint32 colorStride = batchDesc.colorStride;
    if (colorPtr == nullptr)
    {
        colorPtr = &useColorRGBA;
        colorStride = 0;
    }

    // Prepare texture coordinates ptr (batchDesc.texCoordPointer or zero vector)
    const float32* texPtr = batchDesc.texCoordPointer[0];
    uint32 texStride = batchDesc.texCoordStride;
    if ((texPtr == nullptr) || (batchDesc.texCoordCount == 0)) //for zero texCoordCount count we just use 1 empty texcoord stream for batching optimization
    {
        static float32 TEX_ZERO[2] = { 0.f, 0.f };
        texPtr = TEX_ZERO;
        texStride = 0;
    }

    // Begin fill vertex and index buffers
    struct BatchVertex
    {
        Vector3 pos;
        Vector2 uv;
        uint32 color;
        //optional explicit params
        Vector2 uv_ext[BatchDescriptor2D::MAX_TEXTURE_STREAMS_COUNT - 1];
    };

    uint32 vertexStride = GetVBOStride(currentTexcoordStreamCount);
    currentVertexBuffer.resize(vertexStride * (vertexIndex + batchDesc.vertexCount));
    currentIndexBuffer.resize(indexIndex + batchDesc.indexCount);

    for (uint32 i = 0; i < batchDesc.vertexCount; ++i)
    {
        BatchVertex& v = *OffsetPointer<BatchVertex>(currentVertexBuffer.data(), vertexStride * (vertexIndex + i));
        v.pos.x = batchDesc.vertexPointer[i * batchDesc.vertexStride];
        v.pos.y = batchDesc.vertexPointer[i * batchDesc.vertexStride + 1];
        //TODO: rethink do we still require z in rhi?
        v.pos.z = 0.f; // axis Z, empty but need for EVF_VERTEX format
        v.uv.x = texPtr[i * texStride];
        v.uv.y = texPtr[i * texStride + 1];
        v.color = colorPtr[i * colorStride];
    }
    //add optional texture streams
    for (uint32 texStream = 1; texStream < batchDesc.texCoordCount; ++texStream)
    {
        for (uint32 i = 0; i < batchDesc.vertexCount; ++i)
        {
            DVASSERT(batchDesc.texCoordPointer[texStream] != nullptr);
            BatchVertex& v = *OffsetPointer<BatchVertex>(currentVertexBuffer.data(), vertexStride * (vertexIndex + i));
            v.uv_ext[texStream - 1].x = batchDesc.texCoordPointer[texStream][i * texStride];
            v.uv_ext[texStream - 1].y = batchDesc.texCoordPointer[texStream][i * texStride + 1];
        }
    }

    uint32 ii = indexIndex;
    for (uint32 i = 0; i < batchDesc.indexCount; ++i)
    {
        currentIndexBuffer[ii++] = vertexIndex + batchDesc.indexPointer[i];
    }
    // End fill vertex and index buffers

    switch (currentPacket.primitiveType)
    {
    case rhi::PRIMITIVE_LINELIST:
        currentPacket.primitiveCount += batchDesc.indexCount / 2;
        break;
    case rhi::PRIMITIVE_TRIANGLELIST:
        currentPacket.primitiveCount += batchDesc.indexCount / 3;
        break;
    case rhi::PRIMITIVE_TRIANGLESTRIP:
        currentPacket.primitiveCount += batchDesc.indexCount - 2;
        break;
    }

    indexIndex += batchDesc.indexCount;
    vertexIndex += batchDesc.vertexCount;
}

void RenderSystem2D::Draw(Sprite* sprite, SpriteDrawState* drawState, const Color& color)
{
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    static uint16 spriteIndeces[] = { 0, 1, 2, 1, 3, 2 };
    Vector<uint16> spriteClippedIndecex;

    SpriteDrawState* state = drawState;
    if (!state)
    {
        state = &defaultSpriteDrawState;
    }

    DVASSERT(state != nullptr);

    float32 scaleX = 1.0f;
    float32 scaleY = 1.0f;

    sprite->flags = 0;
    if (state->flags != 0)
    {
        sprite->flags |= Sprite::EST_MODIFICATION;
    }

    if (state->scale.x != 1.f || state->scale.y != 1.f)
    {
        sprite->flags |= Sprite::EST_SCALE;
        scaleX = state->scale.x;
        scaleY = state->scale.y;
    }

    if (state->angle != 0.f)
        sprite->flags |= Sprite::EST_ROTATE;

    int32 frame = Clamp(state->frame, 0, sprite->frameCount - 1);

    float32 x = state->position.x - state->pivotPoint.x * state->scale.x;
    float32 y = state->position.y - state->pivotPoint.y * state->scale.y;

    float32** frameVertices = sprite->frameVertices;
    float32** rectsAndOffsets = sprite->rectsAndOffsets;
    Vector2 spriteSize = sprite->size;

    if (sprite->flags & Sprite::EST_MODIFICATION)
    {
        if ((state->flags & (ESM_HFLIP | ESM_VFLIP)) == (ESM_HFLIP | ESM_VFLIP))
        { //HFLIP|VFLIP
            if (sprite->flags & Sprite::EST_SCALE)
            { //SCALE
                x += (spriteSize.dx - rectsAndOffsets[frame][2] - rectsAndOffsets[frame][4] * 2) * scaleX;
                y += (spriteSize.dy - rectsAndOffsets[frame][3] - rectsAndOffsets[frame][5] * 2) * scaleY;
                if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                {
                    spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][0] * scaleX + x; //x2 do not change this sequence. This is because of the cache reason
                    spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][5] * scaleY + y; //y1
                    spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][2] * scaleX + x; //x1
                    spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][1] * scaleY + y; //y2
                }
                else
                {
                    spriteTempVertices[2] = spriteTempVertices[6] = AlignToX(frameVertices[frame][0] * scaleX + x); //x2
                    spriteTempVertices[5] = spriteTempVertices[7] = AlignToY(frameVertices[frame][1] * scaleY + y); //y2
                    spriteTempVertices[0] = spriteTempVertices[4] = (frameVertices[frame][2] - frameVertices[frame][0]) * scaleX + spriteTempVertices[2]; //x1
                    spriteTempVertices[1] = spriteTempVertices[3] = (frameVertices[frame][5] - frameVertices[frame][1]) * scaleY + spriteTempVertices[5]; //y1
                }
            }
            else
            { //NOT SCALE
                x += (spriteSize.dx - rectsAndOffsets[frame][2] - rectsAndOffsets[frame][4] * 2);
                y += (spriteSize.dy - rectsAndOffsets[frame][3] - rectsAndOffsets[frame][5] * 2);
                if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                {
                    spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][0] + x; //x2 do not change this sequence. This is because of the cache reason
                    spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][5] + y; //y1
                    spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][2] + x; //x1
                    spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][1] + y; //y2
                }
                else
                {
                    spriteTempVertices[2] = spriteTempVertices[6] = AlignToX(frameVertices[frame][0] + x); //x2
                    spriteTempVertices[5] = spriteTempVertices[7] = AlignToY(frameVertices[frame][1] + y); //y2
                    spriteTempVertices[0] = spriteTempVertices[4] = (frameVertices[frame][2] - frameVertices[frame][0]) + spriteTempVertices[2]; //x1
                    spriteTempVertices[1] = spriteTempVertices[3] = (frameVertices[frame][5] - frameVertices[frame][1]) + spriteTempVertices[5]; //y1
                }
            }
        }
        else
        {
            if (state->flags & ESM_HFLIP)
            { //HFLIP
                if (sprite->flags & Sprite::EST_SCALE)
                { //SCALE
                    x += (spriteSize.dx - rectsAndOffsets[frame][2] - rectsAndOffsets[frame][4] * 2) * scaleX;
                    if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][2] * scaleX + x; //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][5] * scaleY + y; //y2
                        spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][1] * scaleX + y; //y1 //WEIRD: maybe scaleY should be used?
                        spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][0] * scaleX + x; //x2
                    }
                    else
                    {
                        spriteTempVertices[2] = spriteTempVertices[6] = AlignToX(frameVertices[frame][0] * scaleX + x); //x2
                        spriteTempVertices[0] = spriteTempVertices[4] = (frameVertices[frame][2] - frameVertices[frame][0]) * scaleX + spriteTempVertices[2]; //x1
                        spriteTempVertices[1] = spriteTempVertices[3] = AlignToY(frameVertices[frame][1] * scaleY + y); //y1
                        spriteTempVertices[5] = spriteTempVertices[7] = (frameVertices[frame][5] - frameVertices[frame][1]) * scaleY + spriteTempVertices[1]; //y2
                    }
                }
                else
                { //NOT SCALE
                    x += (spriteSize.dx - rectsAndOffsets[frame][2] - rectsAndOffsets[frame][4] * 2);
                    if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][2] + x; //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][5] + y; //y2
                        spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][1] + y; //y1
                        spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][0] + x; //x2
                    }
                    else
                    {
                        spriteTempVertices[2] = spriteTempVertices[6] = AlignToX(frameVertices[frame][0] + x); //x2
                        spriteTempVertices[0] = spriteTempVertices[4] = (frameVertices[frame][2] - frameVertices[frame][0]) + spriteTempVertices[2]; //x1
                        spriteTempVertices[1] = spriteTempVertices[3] = AlignToY(frameVertices[frame][1] + y); //y1
                        spriteTempVertices[5] = spriteTempVertices[7] = (frameVertices[frame][5] - frameVertices[frame][1]) + spriteTempVertices[1]; //y2
                    }
                }
            }
            else
            { //VFLIP
                if (sprite->flags & Sprite::EST_SCALE)
                { //SCALE
                    y += (spriteSize.dy - rectsAndOffsets[frame][3] - rectsAndOffsets[frame][5] * 2) * scaleY;
                    if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][0] * scaleX + x; //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][1] * scaleY + y; //y2
                        spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][5] * scaleY + y; //y1
                        spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][2] * scaleX + x; //x2
                    }
                    else
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = AlignToX(frameVertices[frame][0] * scaleX + x); //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = AlignToY(frameVertices[frame][1] * scaleY + y); //y2
                        spriteTempVertices[2] = spriteTempVertices[6] = (frameVertices[frame][2] - frameVertices[frame][0]) * scaleX + spriteTempVertices[0]; //x2
                        spriteTempVertices[1] = spriteTempVertices[3] = (frameVertices[frame][5] - frameVertices[frame][1]) * scaleY + spriteTempVertices[5]; //y1
                    }
                }
                else
                { //NOT SCALE
                    y += (spriteSize.dy - rectsAndOffsets[frame][3] - rectsAndOffsets[frame][5] * 2);
                    if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][0] + x; //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][1] + y; //y2
                        spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][5] + y; //y1
                        spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][2] + x; //x2
                    }
                    else
                    {
                        spriteTempVertices[0] = spriteTempVertices[4] = AlignToX(frameVertices[frame][0] + x); //x1
                        spriteTempVertices[5] = spriteTempVertices[7] = AlignToY(frameVertices[frame][1] + y); //y2
                        spriteTempVertices[2] = spriteTempVertices[6] = (frameVertices[frame][2] - frameVertices[frame][0]) + spriteTempVertices[0]; //x2
                        spriteTempVertices[1] = spriteTempVertices[3] = (frameVertices[frame][5] - frameVertices[frame][1]) + spriteTempVertices[5]; //y1
                    }
                }
            }
        }
    }
    else
    { //NO MODIFERS
        if (sprite->flags & Sprite::EST_SCALE)
        { //SCALE
            if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
            {
                spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][0] * scaleX + x; //x1
                spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][5] * scaleY + y; //y2
                spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][1] * scaleY + y; //y1
                spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][2] * scaleX + x; //x2 do not change this sequence. This is because of the cache reason
            }
            else
            {
                spriteTempVertices[0] = spriteTempVertices[4] = AlignToX(frameVertices[frame][0] * scaleX + x); //x1
                spriteTempVertices[1] = spriteTempVertices[3] = AlignToY(frameVertices[frame][1] * scaleY + y); //y1
                spriteTempVertices[2] = spriteTempVertices[6] = (frameVertices[frame][2] - frameVertices[frame][0]) * scaleX + spriteTempVertices[0]; //x2
                spriteTempVertices[5] = spriteTempVertices[7] = (frameVertices[frame][5] - frameVertices[frame][1]) * scaleY + spriteTempVertices[1]; //y2
            }
        }
        else
        { //NOT SCALE
            if (!state->usePerPixelAccuracy || (sprite->flags & Sprite::EST_ROTATE))
            {
                spriteTempVertices[0] = spriteTempVertices[4] = frameVertices[frame][0] + x; //x1
                spriteTempVertices[5] = spriteTempVertices[7] = frameVertices[frame][5] + y; //y2
                spriteTempVertices[1] = spriteTempVertices[3] = frameVertices[frame][1] + y; //y1
                spriteTempVertices[2] = spriteTempVertices[6] = frameVertices[frame][2] + x; //x2 do not change this sequence. This is because of the cache reason
            }
            else
            {
                spriteTempVertices[0] = spriteTempVertices[4] = AlignToX(frameVertices[frame][0] + x); //x1
                spriteTempVertices[1] = spriteTempVertices[3] = AlignToY(frameVertices[frame][1] + y); //y1
                spriteTempVertices[2] = spriteTempVertices[6] = (frameVertices[frame][2] - frameVertices[frame][0]) + spriteTempVertices[0]; //x2
                spriteTempVertices[5] = spriteTempVertices[7] = (frameVertices[frame][5] - frameVertices[frame][1]) + spriteTempVertices[1]; //y2
            }
        }
    }

    if (!sprite->clipPolygon)
    {
        if (sprite->flags & Sprite::EST_ROTATE)
        {
            //SLOW CODE
            //			glPushMatrix();
            //			glTranslatef(drawCoord.x, drawCoord.y, 0);
            //			glRotatef(RadToDeg(rotateAngle), 0.0f, 0.0f, 1.0f);
            //			glTranslatef(-drawCoord.x, -drawCoord.y, 0);
            //			RenderManager::Instance()->DrawArrays(PRIMITIVETYPE_TRIANGLESTRIP, 0, 4);
            //			glPopMatrix();

            // Optimized code
            float32 sinA = std::sin(state->angle);
            float32 cosA = std::cos(state->angle);
            for (int32 k = 0; k < 4; ++k)
            {
                float32 x = spriteTempVertices[(k << 1)] - state->position.x;
                float32 y = spriteTempVertices[(k << 1) + 1] - state->position.y;

                float32 nx = (x)*cosA - (y)*sinA + state->position.x;
                float32 ny = (x)*sinA + (y)*cosA + state->position.y;

                spriteTempVertices[(k << 1)] = nx;
                spriteTempVertices[(k << 1) + 1] = ny;
            }
        }

        if (spriteClipping && !IsPreparedSpriteOnScreen(state))
        {
            // Skip draw for sprites out of screen
            return;
        }

        spriteVertexCount = 4;
        spriteIndexCount = 6;
    }
    else
    {
        spriteClippedVertices.clear();
        spriteClippedVertices.reserve(sprite->clipPolygon->GetPointCount());
        spriteClippedTexCoords.clear();
        spriteClippedTexCoords.reserve(sprite->clipPolygon->GetPointCount());

        Texture* t = sprite->GetTexture(frame);

        Vector2 virtualTexSize = Vector2(float32(t->width), float32(t->height));
        if (GetActiveTargetDescriptor().transformVirtualToPhysical)
        {
            if (sprite->type == Sprite::SPRITE_FROM_FILE)
            {
                virtualTexSize = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtual(virtualTexSize, sprite->GetResourceSizeIndex());
            }
            else if (!sprite->textureInVirtualSpace)
            {
                virtualTexSize = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtual(virtualTexSize);
            }
        }
        float32 adjWidth = 1.f / virtualTexSize.x;
        float32 adjHeight = 1.f / virtualTexSize.y;

        if (sprite->flags & Sprite::EST_SCALE)
        {
            if (state && state->usePerPixelAccuracy)
            {
                x += AlignToX(frameVertices[frame][0] * scaleX + x) - frameVertices[frame][0] * scaleX - x;
                y += AlignToY(frameVertices[frame][1] * scaleY + y) - frameVertices[frame][1] * scaleY - y;
            }

            for (int32 i = 0; i < sprite->clipPolygon->GetPointCount(); ++i)
            {
                const Vector2& point = sprite->clipPolygon->GetPoints()[i];
                spriteClippedVertices.push_back(Vector2(point.x * scaleX + x, point.y * scaleY + y));
            }
        }
        else
        {
            if (state && state->usePerPixelAccuracy)
            {
                x += AlignToX(frameVertices[frame][0] + x) - frameVertices[frame][0] - x;
                y += AlignToY(frameVertices[frame][1] + y) - frameVertices[frame][1] - y;
            }

            Vector2 pos(x, y);
            for (int32 i = 0; i < sprite->clipPolygon->GetPointCount(); ++i)
            {
                const Vector2& point = sprite->clipPolygon->GetPoints()[i];
                spriteClippedVertices.push_back(point + pos);
            }
        }

        for (int32 i = 0; i < sprite->clipPolygon->GetPointCount(); ++i)
        {
            const Vector2& point = sprite->clipPolygon->GetPoints()[i];
            Vector2 texCoord((point.x - frameVertices[frame][0]) * adjWidth, (point.y - frameVertices[frame][1]) * adjHeight);
            spriteClippedTexCoords.push_back(Vector2(sprite->texCoords[frame][0] + texCoord.x, sprite->texCoords[frame][1] + texCoord.y));
        }

        spriteVertexCount = sprite->clipPolygon->GetPointCount();
        DVASSERT(spriteVertexCount > 2); // Clip polygon should contain 3 points or more
        spriteIndexCount = (spriteVertexCount - 2) * 3;

        spriteClippedIndecex.clear();
        spriteClippedIndecex.reserve(spriteIndexCount);

        for (int32 i = 2; i < spriteVertexCount; ++i)
        {
            spriteClippedIndecex.push_back(0);
            spriteClippedIndecex.push_back(i - 1);
            spriteClippedIndecex.push_back(i);
        }
    }

    BatchDescriptor2D batch;
    batch.material = state->GetMaterial();
    batch.textureSetHandle = sprite->GetTexture(frame)->singleTextureSet;
    batch.samplerStateHandle = sprite->GetTexture(frame)->samplerStateHandle;
    batch.singleColor = color;
    batch.vertexCount = spriteVertexCount;
    batch.indexCount = spriteIndexCount;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;

    if (!sprite->clipPolygon)
    {
        batch.vertexPointer = spriteTempVertices.data();
        batch.texCoordPointer[0] = sprite->texCoords[frame];
        batch.indexPointer = spriteIndeces;
    }
    else
    {
        batch.vertexPointer = spriteClippedVertices.data()->data;
        batch.texCoordPointer[0] = spriteClippedTexCoords.data()->data;
        batch.indexPointer = spriteClippedIndecex.data();
    }
    PushBatch(batch);
}

void RenderSystem2D::DrawStretched(Sprite* sprite, SpriteDrawState* state, Vector2 stretchCapVector, int32 type, const UIGeometricData& gd, StretchDrawData** pStreachData, const Color& color)
{
    if (!sprite)
        return;
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    int32 frame = Clamp(state->frame, 0, sprite->frameCount - 1);
    const Vector2& size = gd.size;

    if (stretchCapVector.x < 0.0f || stretchCapVector.y < 0.0f ||
        size.x <= 0.0f || size.y <= 0.0f)
        return;

    Vector2 stretchCap(Min(size.x * 0.5f, stretchCapVector.x),
                       Min(size.y * 0.5f, stretchCapVector.y));

    bool needGenerateData = false;
    StretchDrawData* stretchData = 0;
    if (pStreachData)
    {
        stretchData = *pStreachData;
        if (!stretchData)
        {
            stretchData = new StretchDrawData();
            needGenerateData = true;
            *pStreachData = stretchData;
        }
        else
        {
            needGenerateData |= sprite->textures[0] != stretchData->texture;
            needGenerateData |= sprite != stretchData->sprite;
            needGenerateData |= frame != stretchData->frame;
            needGenerateData |= gd.size != stretchData->size;
            needGenerateData |= type != stretchData->type;
            needGenerateData |= stretchCap != stretchData->stretchCap;
        }
    }
    else
    {
        stretchData = new StretchDrawData();
        needGenerateData = true;
    }

    StretchDrawData& sd = *stretchData;

    if (needGenerateData)
    {
        sd.sprite = sprite;
        sd.texture = sprite->textures[0];
        sd.frame = frame;
        sd.size = gd.size;
        sd.type = type;
        sd.stretchCap = stretchCap;
        sd.GenerateStretchData();
    }

    Matrix3 transformMatr;
    gd.BuildTransformMatrix(transformMatr);

    Matrix3 flipMatrix;
    if ((state->flags & ESM_HFLIP) && (state->flags & ESM_VFLIP))
    {
        flipMatrix = Matrix3(-1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, sd.size.x, sd.size.y, 1.0f);
    }
    else if (state->flags & ESM_HFLIP)
    {
        flipMatrix = Matrix3(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, sd.size.x, 0.0f, 1.0f);
    }
    else if (state->flags & ESM_VFLIP)
    {
        flipMatrix = Matrix3(1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, sd.size.y, 1.0f);
    }

    transformMatr = flipMatrix * transformMatr;
    if (needGenerateData || sd.transformMatr != transformMatr || sd.usePerPixelAccuracy != state->usePerPixelAccuracy)
    {
        sd.transformMatr = transformMatr;
        sd.usePerPixelAccuracy = state->usePerPixelAccuracy;
        sd.GenerateTransformData();
    }

    spriteVertexCount = int32(sd.transformedVertices.size());
    spriteIndexCount = sd.GetVertexInTrianglesCount();

    if (spriteVertexCount > 0 && spriteIndexCount > 0) // Ignore incorrect streched data
    {
        BatchDescriptor2D batch;
        batch.singleColor = color;
        batch.textureSetHandle = sprite->GetTexture(frame)->singleTextureSet;
        batch.samplerStateHandle = sprite->GetTexture(frame)->samplerStateHandle;
        batch.material = state->GetMaterial();
        batch.vertexCount = spriteVertexCount;
        batch.indexCount = spriteIndexCount;
        batch.vertexStride = 2;
        batch.texCoordStride = 2;
        batch.vertexPointer = sd.transformedVertices.data()->data;
        batch.texCoordPointer[0] = sd.texCoords.data()->data;
        batch.indexPointer = sd.indeces;

        PushBatch(batch);
    }

    if (!pStreachData)
    {
        SafeDelete(stretchData);
    }
}

void RenderSystem2D::DrawTiled(Sprite* sprite, SpriteDrawState* state, const Vector2& stretchCapVector, const UIGeometricData& gd, TiledDrawData** pTiledData, const Color& color)
{
    if (!sprite)
        return;
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    int32 frame = Clamp(state->frame, 0, sprite->frameCount - 1);

    const Vector2& size = gd.size;

    if (stretchCapVector.x < 0.0f || stretchCapVector.y < 0.0f ||
        size.x <= 0.0f || size.y <= 0.0f)
        return;

    Vector2 stretchCap(Min(size.x, sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_WIDTH)),
                       Min(size.y, sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_HEIGHT)));

    stretchCap.x = Min(stretchCap.x * 0.5f, stretchCapVector.x);
    stretchCap.y = Min(stretchCap.y * 0.5f, stretchCapVector.y);

    bool needGenerateData = false;

    TiledDrawData* tiledData = 0;
    if (pTiledData)
    {
        tiledData = *pTiledData;
        if (!tiledData)
        {
            tiledData = new TiledDrawData();
            needGenerateData = true;
            *pTiledData = tiledData;
        }
        else
        {
            needGenerateData |= stretchCap != tiledData->stretchCap;
            needGenerateData |= frame != tiledData->frame;
            needGenerateData |= sprite != tiledData->sprite;
            needGenerateData |= sprite->textures[0] != tiledData->texture;
            needGenerateData |= size != tiledData->size;
        }
    }
    else
    {
        tiledData = new TiledDrawData();
        needGenerateData = true;
    }

    TiledDrawData& td = *tiledData;

    if (needGenerateData)
    {
        td.stretchCap = stretchCap;
        td.size = size;
        td.frame = frame;
        td.sprite = sprite;
        td.texture = sprite->textures[0];
        td.GenerateTileData();
    }

    Matrix3 transformMatr;
    gd.BuildTransformMatrix(transformMatr);

    if (needGenerateData || td.transformMatr != transformMatr)
    {
        td.transformMatr = transformMatr;
        td.GenerateTransformData();
    }

    const uint32 uCount = static_cast<uint32>(td.units.size());
    for (uint32 uIndex = 0; uIndex < uCount; ++uIndex)
    {
        TiledDrawData::Unit& unit = td.units[uIndex];
        BatchDescriptor2D batch;
        batch.singleColor = color;
        batch.material = state->GetMaterial();
        batch.textureSetHandle = sprite->GetTexture(frame)->singleTextureSet;
        batch.samplerStateHandle = sprite->GetTexture(frame)->samplerStateHandle;
        batch.vertexCount = static_cast<int32>(unit.transformedVertices.size());
        batch.indexCount = static_cast<int32>(unit.indeces.size());
        batch.vertexStride = 2;
        batch.texCoordStride = 2;
        batch.vertexPointer = unit.transformedVertices.data()->data;
        batch.texCoordPointer[0] = unit.texCoords.data()->data;
        batch.indexPointer = unit.indeces.data();
        PushBatch(batch);
    }

    if (!pTiledData)
    {
        SafeDelete(tiledData);
    }
}

void RenderSystem2D::DrawTiledMultylayer(Sprite* mask, Sprite* detail, Sprite* gradient, Sprite* contour,
                                         SpriteDrawState* state, const Vector2& stretchCapVector, const UIGeometricData& gd, TiledMultilayerData** pTileData, const Color& color)
{
    if (!contour || !mask || !detail || !gradient)
        return;

    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    const Vector2& size = gd.size;

    if (stretchCapVector.x < 0.0f || stretchCapVector.y < 0.0f ||
        size.x <= 0.0f || size.y <= 0.0f)
        return;

    Vector2 stretchCap(Min(size.x, contour->GetRectOffsetValueForFrame(0, Sprite::ACTIVE_WIDTH)),
                       Min(size.y, contour->GetRectOffsetValueForFrame(0, Sprite::ACTIVE_HEIGHT)));

    stretchCap.x = Min(stretchCap.x * 0.5f, stretchCapVector.x);
    stretchCap.y = Min(stretchCap.y * 0.5f, stretchCapVector.y);

    bool needGenerateData = false;

    TiledMultilayerData* tiledData = 0;
    if (pTileData)
    {
        tiledData = *pTileData;
        if (!tiledData)
        {
            tiledData = new TiledMultilayerData();
            needGenerateData = true;
            *pTileData = tiledData;
        }
        else
        {
            needGenerateData |= stretchCap != tiledData->stretchCap;
            needGenerateData |= mask != tiledData->mask;
            needGenerateData |= detail != tiledData->detail;
            needGenerateData |= gradient != tiledData->gradient;
            needGenerateData |= contour != tiledData->contour;
            needGenerateData |= size != tiledData->size;
            needGenerateData |= mask->textures[0] != tiledData->mask_texture;
            needGenerateData |= detail->textures[0] != tiledData->detail_texture;
            needGenerateData |= gradient->textures[0] != tiledData->gradient_texture;
            needGenerateData |= contour->textures[0] != tiledData->contour_texture;
        }
    }
    else
    {
        tiledData = new TiledMultilayerData();
        needGenerateData = true;
    }

    TiledMultilayerData& td = *tiledData;

    if (needGenerateData)
    {
        td.stretchCap = stretchCap;
        td.size = size;
        td.mask = mask;
        td.detail = detail;
        td.gradient = gradient;
        td.contour = contour;

        td.mask_texture = mask->textures[0];
        td.detail_texture = detail->textures[0];
        td.gradient_texture = gradient->textures[0];
        td.contour_texture = contour->textures[0];

        rhi::TextureSetDescriptor textureSetDescriptor;
        textureSetDescriptor.fragmentTextureCount = 4;
        textureSetDescriptor.fragmentTexture[0] = mask->GetTexture(0)->handle;
        textureSetDescriptor.fragmentTexture[1] = detail->GetTexture(0)->handle;
        textureSetDescriptor.fragmentTexture[2] = gradient->GetTexture(0)->handle;
        textureSetDescriptor.fragmentTexture[3] = contour->GetTexture(0)->handle;
        rhi::HTextureSet textureSet = rhi::AcquireTextureSet(textureSetDescriptor);

        rhi::SamplerState::Descriptor samplerStateSetDescriptor;
        samplerStateSetDescriptor.fragmentSamplerCount = 4;
        samplerStateSetDescriptor.fragmentSampler[0] = mask->GetTexture(0)->samplerState;
        samplerStateSetDescriptor.fragmentSampler[1] = detail->GetTexture(0)->samplerState;
        samplerStateSetDescriptor.fragmentSampler[2] = gradient->GetTexture(0)->samplerState;
        samplerStateSetDescriptor.fragmentSampler[3] = contour->GetTexture(0)->samplerState;
        rhi::HSamplerState samplerState = rhi::AcquireSamplerState(samplerStateSetDescriptor);

        rhi::ReleaseTextureSet(td.textureSet);
        td.textureSet = textureSet;
        rhi::ReleaseSamplerState(td.samplerState);
        td.samplerState = samplerState;

        td.GenerateTileData();
    }

    Matrix3 transformMatr;
    gd.BuildTransformMatrix(transformMatr);

    if (needGenerateData || (td.transformMatr != transformMatr) || (td.usePerPixelAccuracy != state->usePerPixelAccuracy))
    {
        td.transformMatr = transformMatr;
        td.usePerPixelAccuracy = state->usePerPixelAccuracy;
        td.GenerateTransformData(td.usePerPixelAccuracy);
    }

    spriteVertexCount = static_cast<int32>(td.transformedVertices.size());
    spriteIndexCount = static_cast<int32>(td.indices.size());

    if (td.transformedVertices.size() != 0)
    {
        BatchDescriptor2D batch;
        batch.singleColor = color;
        batch.material = state->GetMaterial();
        batch.textureSetHandle = td.textureSet;
        batch.samplerStateHandle = td.samplerState;
        batch.texCoordCount = 4;
        batch.vertexCount = static_cast<int32>(td.transformedVertices.size());
        batch.indexCount = static_cast<int32>(td.indices.size());
        batch.vertexStride = 2;
        batch.texCoordStride = 2;
        batch.vertexPointer = td.transformedVertices.data()->data;
        batch.texCoordPointer[0] = td.texCoordsMask.data()->data;
        batch.texCoordPointer[1] = td.texCoordsDetail.data()->data;
        batch.texCoordPointer[2] = td.texCoordsGradient.data()->data;
        batch.texCoordPointer[3] = td.texCoordsContour.data()->data;
        batch.indexPointer = td.indices.data();
        PushBatch(batch);
    }
    if (!pTileData)
    {
        SafeDelete(tiledData);
    }
}

/* RenderSyste2D Draw Helper Functions */

void RenderSystem2D::FillRect(const Rect& rect, const Color& color, NMaterial* material)
{
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    static uint16 indices[6] = { 0, 1, 2, 1, 3, 2 };
    spriteTempVertices[0] = rect.x;
    spriteTempVertices[1] = rect.y;
    spriteTempVertices[2] = rect.x + rect.dx;
    spriteTempVertices[3] = rect.y;
    spriteTempVertices[4] = rect.x;
    spriteTempVertices[5] = rect.y + rect.dy;
    spriteTempVertices[6] = rect.x + rect.dx;
    spriteTempVertices[7] = rect.y + rect.dy;

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = material;
    batch.vertexCount = 4;
    batch.indexCount = 6;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.indexPointer = indices;
    PushBatch(batch);
}

void RenderSystem2D::FillGradientRect(const Rect& rect, const Color& xy, const Color& wy, const Color& xh, const Color& wh)
{
    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::SPRITE_DRAW))
    {
        return;
    }

    static uint16 indices[6] = { 0, 1, 2, 1, 3, 2 };
    spriteTempVertices[0] = rect.x;
    spriteTempVertices[1] = rect.y;

    spriteTempVertices[2] = rect.x + rect.dx;
    spriteTempVertices[3] = rect.y;

    spriteTempVertices[4] = rect.x;
    spriteTempVertices[5] = rect.y + rect.dy;

    spriteTempVertices[6] = rect.x + rect.dx;
    spriteTempVertices[7] = rect.y + rect.dy;

    spriteTempColors[0] = xy.GetRGBA();
    spriteTempColors[1] = wy.GetRGBA();
    spriteTempColors[2] = xh.GetRGBA();
    spriteTempColors[3] = wh.GetRGBA();

    BatchDescriptor2D batch;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = 4;
    batch.colorStride = 1;
    batch.indexCount = 6;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.colorPointer = spriteTempColors.data();
    batch.indexPointer = indices;
    PushBatch(batch);
}

void RenderSystem2D::DrawRect(const Rect& rect, const Color& color)
{
    static uint16 indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
    spriteTempVertices[0] = rect.x;
    spriteTempVertices[1] = rect.y;
    spriteTempVertices[2] = rect.x + rect.dx;
    spriteTempVertices[3] = rect.y;
    spriteTempVertices[4] = rect.x + rect.dx;
    spriteTempVertices[5] = rect.y + rect.dy;
    spriteTempVertices[6] = rect.x;
    spriteTempVertices[7] = rect.y + rect.dy;

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = 4;
    batch.indexCount = 8;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.indexPointer = indices;
    batch.primitiveType = rhi::PRIMITIVE_LINELIST;
    PushBatch(batch);
}

void RenderSystem2D::DrawGrid(const Rect& rect, const Vector2& gridSize, const Color& color)
{
    // TODO! review with Ivan/Victor whether it is not performance problem!
    Vector<float32> gridVertices;
    int32 verLinesCount = static_cast<int32>(std::ceil(rect.dx / gridSize.x));
    int32 horLinesCount = static_cast<int32>(std::ceil(rect.dy / gridSize.y));
    gridVertices.resize((horLinesCount + verLinesCount) * 4);

    float32 curPos = 0;
    int32 curVertexIndex = 0;
    for (int i = 0; i < horLinesCount; i++)
    {
        gridVertices[curVertexIndex++] = rect.x;
        gridVertices[curVertexIndex++] = rect.y + curPos;
        gridVertices[curVertexIndex++] = rect.x + rect.dx;
        gridVertices[curVertexIndex++] = rect.y + curPos;
    }

    curPos = 0.0f;
    for (int i = 0; i < verLinesCount; i++)
    {
        gridVertices[curVertexIndex++] = rect.x + curPos;
        gridVertices[curVertexIndex++] = rect.y;
        gridVertices[curVertexIndex++] = rect.x + curPos;
        gridVertices[curVertexIndex++] = rect.y + rect.dy;
    }

    Vector<uint16> indices;
    for (int i = 0; i < curVertexIndex; ++i)
    {
        indices.push_back(i);
    }

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = curVertexIndex / 2;
    batch.indexCount = curVertexIndex;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = gridVertices.data();
    batch.indexPointer = indices.data();
    batch.primitiveType = rhi::PRIMITIVE_LINELIST;
    PushBatch(batch);
}

void RenderSystem2D::DrawLine(const Vector2& start, const Vector2& end, const Color& color)
{
    static uint16 indices[2] = { 0, 1 };
    spriteTempVertices[0] = start.x;
    spriteTempVertices[1] = start.y;
    spriteTempVertices[2] = end.x;
    spriteTempVertices[3] = end.y;

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = 2;
    batch.indexCount = 2;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.indexPointer = indices;
    batch.primitiveType = rhi::PRIMITIVE_LINELIST;
    PushBatch(batch);
}

void RenderSystem2D::DrawLine(const Vector2& start, const Vector2& end, float32 lineWidth, const Color& color)
{
    // TODO: Create list of lines for emulating line with width >1px
    static uint16 indices[2] = { 0, 1 };
    spriteTempVertices[0] = start.x;
    spriteTempVertices[1] = start.y;
    spriteTempVertices[2] = end.x;
    spriteTempVertices[3] = end.y;

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = 2;
    batch.indexCount = 2;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.indexPointer = indices;
    batch.primitiveType = rhi::PRIMITIVE_LINELIST;
    PushBatch(batch);
}

void RenderSystem2D::DrawLines(const Vector<float32>& linePoints, const Color& color)
{
    auto ptCount = linePoints.size() / 2; // linePoints are pairs of XY
    if (ptCount < 2)
    {
        return;
    }

    Vector<uint16> indices;
    indices.reserve(ptCount);
    for (auto i = 0U; i < ptCount; ++i)
    {
        indices.push_back(i);
    }

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.material = DEFAULT_2D_COLOR_MATERIAL;
    batch.vertexCount = static_cast<uint32>(ptCount);
    batch.indexCount = static_cast<uint32>(indices.size());
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = linePoints.data();
    batch.indexPointer = indices.data();
    batch.primitiveType = rhi::PRIMITIVE_LINELIST;
    PushBatch(batch);
}

void RenderSystem2D::DrawCircle(const Vector2& center, float32 radius, const Color& color)
{
    Polygon2 pts;
    float32 angle = Min(PI / 6.0f, SEGMENT_LENGTH / radius); // maximum angle 30 degrees
    int32 ptsCount = int32(2 * PI / angle) + 1;

    pts.points.reserve(ptsCount);
    for (int32 k = 0; k < ptsCount; ++k)
    {
        float32 angle = (float32(k) / (ptsCount - 1)) * 2 * PI;
        float32 sinA = std::sin(angle);
        float32 cosA = std::cos(angle);
        Vector2 pos = center - Vector2(sinA * radius, cosA * radius);

        pts.AddPoint(pos);
    }

    DrawPolygon(pts, false, color);
}

void RenderSystem2D::DrawPolygon(const Polygon2& polygon, bool closed, const Color& color)
{
    auto ptCount = polygon.GetPointCount();
    if (ptCount >= 2)
    {
        Vector<uint16> indices;
        indices.reserve(ptCount + 1);
        auto i = 0;
        for (; i < ptCount - 1; ++i)
        {
            indices.push_back(i);
            indices.push_back(i + 1);
        }
        if (closed)
        {
            indices.push_back(i);
            indices.push_back(0);
        }
        auto pointsPtr = static_cast<const float32*>(static_cast<const void*>(polygon.GetPoints()));

        BatchDescriptor2D batch;
        batch.singleColor = color;
        batch.material = DEFAULT_2D_COLOR_MATERIAL;
        batch.vertexCount = ptCount;
        batch.indexCount = static_cast<uint32>(indices.size());
        batch.vertexStride = 2;
        batch.texCoordStride = 2;
        batch.vertexPointer = pointsPtr;
        batch.indexPointer = indices.data();
        batch.primitiveType = rhi::PRIMITIVE_LINELIST;
        PushBatch(batch);
    }
}

void RenderSystem2D::FillPolygon(const Polygon2& polygon, const Color& color)
{
    auto ptCount = polygon.GetPointCount();
    if (ptCount >= 3)
    {
        Vector<uint16> indices;
        for (auto i = 1; i < ptCount - 1; ++i)
        {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i + 1);
        }
        auto pointsPtr = static_cast<const float32*>(static_cast<const void*>(polygon.GetPoints()));

        BatchDescriptor2D batch;
        batch.singleColor = color;
        batch.material = DEFAULT_2D_COLOR_MATERIAL;
        batch.vertexCount = ptCount;
        batch.indexCount = static_cast<uint32>(indices.size());
        batch.vertexPointer = pointsPtr;
        batch.vertexStride = 2;
        batch.texCoordStride = 2;
        batch.indexPointer = indices.data();
        PushBatch(batch);
    }
}

void RenderSystem2D::DrawPolygonTransformed(const Polygon2& polygon, bool closed, const Matrix3& transform, const Color& color)
{
    Polygon2 copyPoly = polygon;
    copyPoly.Transform(transform);
    DrawPolygon(copyPoly, closed, color);
}

void RenderSystem2D::DrawTextureWithoutAdjustingRects(Texture* texture, NMaterial* material, const Color& color,
                                                      const Rect& destRect, const Rect& srcRect)
{
    spriteTempVertices[0] = spriteTempVertices[4] = destRect.x; //x1
    spriteTempVertices[5] = spriteTempVertices[7] = destRect.y; //y2
    spriteTempVertices[1] = spriteTempVertices[3] = destRect.y + destRect.dy; //y1
    spriteTempVertices[2] = spriteTempVertices[6] = destRect.x + destRect.dx; //x2

    float32 texCoords[8];
    texCoords[0] = texCoords[4] = srcRect.x; //x1
    texCoords[5] = texCoords[7] = srcRect.y; //y2
    texCoords[1] = texCoords[3] = srcRect.y + srcRect.dy; //y1
    texCoords[2] = texCoords[6] = srcRect.x + srcRect.dx; //x2

    static uint16 indices[6] = { 0, 1, 2, 1, 3, 2 };

    BatchDescriptor2D batch;
    batch.singleColor = color;
    batch.textureSetHandle = texture->singleTextureSet;
    batch.samplerStateHandle = texture->samplerStateHandle;
    batch.material = material;
    batch.vertexCount = 4;
    batch.indexCount = 6;
    batch.vertexStride = 2;
    batch.texCoordStride = 2;
    batch.vertexPointer = spriteTempVertices.data();
    batch.texCoordPointer[0] = texCoords;
    batch.indexPointer = indices;
    PushBatch(batch);
}

void RenderSystem2D::DrawTexture(Texture* texture, NMaterial* material, const Color& color, const Rect& _dstRect /* = Rect(0.f, 0.f, -1.f, -1.f) */, const Rect& _srcRect /* = Rect(0.f, 0.f, -1.f, -1.f) */)
{
    Rect destRect(_dstRect);
    const RenderTargetPassDescriptor& descr = GetActiveTargetDescriptor();
    if (destRect.dx < 0.f || destRect.dy < 0.f)
    {
        destRect.dx = static_cast<float32>(descr.width == 0 ? Renderer::GetFramebufferWidth() : descr.width);
        destRect.dy = static_cast<float32>(descr.height == 0 ? Renderer::GetFramebufferHeight() : descr.height);
        if (descr.transformVirtualToPhysical)
        {
            destRect = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtual(destRect);
        }
    }

    Rect srcRect;
    srcRect.x = _srcRect.x;
    srcRect.y = _srcRect.y;
    srcRect.dx = (_srcRect.dx < 0.f) ? 1.f : _srcRect.dx;
    srcRect.dy = (_srcRect.dy < 0.f) ? 1.f : _srcRect.dy;

    DrawTextureWithoutAdjustingRects(texture, material, color, destRect, srcRect);
}

/* TiledDrawData Implementation */

void TiledDrawData::GenerateTileData()
{
    Texture* texture = sprite->GetTexture(frame);

    Vector<Vector3> cellsWidth;
    GenerateAxisData(size.x, sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_WIDTH),
                     GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(float32(texture->GetWidth()), sprite->GetResourceSizeIndex()), stretchCap.x, cellsWidth);

    Vector<Vector3> cellsHeight;
    GenerateAxisData(size.y, sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_HEIGHT),
                     GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(float32(texture->GetHeight()), sprite->GetResourceSizeIndex()), stretchCap.y, cellsHeight);

    uint32 vertexLimitPerUnit = MAX_VERTICES - (MAX_VERTICES % 4); // Round for 4 vertexes
    uint32 indexLimitPerUnit = vertexLimitPerUnit / 4 * 6;
    uint32 vertexTotalCount = static_cast<uint32>(4 * cellsHeight.size() * cellsWidth.size());
    uint32 indexTotalCount = static_cast<uint32>(6 * cellsHeight.size() * cellsWidth.size());
    uint32 unitsCount = vertexTotalCount / vertexLimitPerUnit + (vertexTotalCount % vertexLimitPerUnit > 0 ? 1 : 0);

    {
        // Resize units
        units.resize(unitsCount);
        // Resize buffers for first part of units
        for (uint32 i = 0; i < unitsCount - 1; ++i)
        {
            Unit& u = units[i];
            u.vertices.resize(vertexLimitPerUnit);
            u.texCoords.resize(vertexLimitPerUnit);
            u.transformedVertices.resize(vertexLimitPerUnit);
            u.indeces.resize(indexLimitPerUnit);
        }
        // Resize buffers for last unit
        Unit& u = units[unitsCount - 1];
        uint32 lastUnitVertexCount = vertexTotalCount - vertexLimitPerUnit * (unitsCount - 1);
        uint32 lastUnitIndexCount = indexTotalCount - indexLimitPerUnit * (unitsCount - 1);
        u.vertices.resize(lastUnitVertexCount);
        u.texCoords.resize(lastUnitVertexCount);
        u.transformedVertices.resize(lastUnitVertexCount);
        u.indeces.resize(lastUnitIndexCount);
    }

    uint32 unitNumber = 0;
    uint32 offsetIndex = 0;
    const float32* textCoords = sprite->GetTextureCoordsForFrame(frame);
    Vector2 trasformOffset;
    const Vector2 tempTexCoordsPt(textCoords[0], textCoords[1]);
    for (uint32 row = 0; row < cellsHeight.size(); ++row)
    {
        Vector2 cellSize(0.0f, cellsHeight[row].x);
        Vector2 texCellSize(0.0f, cellsHeight[row].y);
        Vector2 texTrasformOffset(0.0f, cellsHeight[row].z);
        trasformOffset.x = 0.0f;

        for (uint32 column = 0; column < cellsWidth.size(); ++column, ++offsetIndex)
        {
            uint32 vertIndex = offsetIndex * 4;
            if (vertIndex >= vertexLimitPerUnit)
            {
                offsetIndex = 0;
                vertIndex = 0;
                ++unitNumber;
            }
            Vector<Vector2>& vertices = units[unitNumber].vertices;
            Vector<Vector2>& texCoords = units[unitNumber].texCoords;
            Vector<uint16>& indeces = units[unitNumber].indeces;

            cellSize.x = cellsWidth[column].x;
            texCellSize.x = cellsWidth[column].y;
            texTrasformOffset.x = cellsWidth[column].z;

            vertices[vertIndex + 0] = trasformOffset;
            vertices[vertIndex + 1] = trasformOffset + Vector2(cellSize.x, 0.0f);
            vertices[vertIndex + 2] = trasformOffset + Vector2(0.0f, cellSize.y);
            vertices[vertIndex + 3] = trasformOffset + cellSize;

            const Vector2 texel = tempTexCoordsPt + texTrasformOffset;
            texCoords[vertIndex + 0] = texel;
            texCoords[vertIndex + 1] = texel + Vector2(texCellSize.x, 0.0f);
            texCoords[vertIndex + 2] = texel + Vector2(0.0f, texCellSize.y);
            texCoords[vertIndex + 3] = texel + texCellSize;

            uint32 indecesIndex = offsetIndex * 6;
            indeces[indecesIndex + 0] = vertIndex;
            indeces[indecesIndex + 1] = vertIndex + 1;
            indeces[indecesIndex + 2] = vertIndex + 2;

            indeces[indecesIndex + 3] = vertIndex + 1;
            indeces[indecesIndex + 4] = vertIndex + 3;
            indeces[indecesIndex + 5] = vertIndex + 2;

            trasformOffset.x += cellSize.x;
        }
        trasformOffset.y += cellSize.y;
    }
}

void TiledDrawData::GenerateAxisData(float32 size, float32 spriteSize, float32 textureSize, float32 stretchCap, Vector<Vector3>& axisData)
{
    int32 gridSize = 0;

    float32 sideSize = stretchCap;
    float32 sideTexSize = sideSize / textureSize;

    float32 centerSize = spriteSize - sideSize * 2.0f;
    float32 centerTexSize = centerSize / textureSize;

    float32 partSize = 0.0f;

    if (centerSize > 0.0f)
    {
        gridSize = int32(std::ceil((size - sideSize * 2.0f) / centerSize));
        const float32 tileAreaSize = size - sideSize * 2.0f;
        partSize = tileAreaSize - std::floor(tileAreaSize / centerSize) * centerSize;
    }

    if (sideSize > 0.0f)
        gridSize += 2;

    DVASSERT(gridSize >= 0, "Incorrect grid size value!");

    axisData.resize(gridSize);

    int32 beginOffset = 0;
    int32 endOffset = 0;
    if (sideSize > 0.0f)
    {
        axisData.front() = Vector3(sideSize, sideTexSize, 0.0f);
        axisData.back() = Vector3(sideSize, sideTexSize, sideTexSize + centerTexSize);
        beginOffset = 1;
        endOffset = 1;
    }

    if (partSize > 0.0f)
    {
        ++endOffset;
        const int32 index = gridSize - endOffset;
        axisData[index].x = partSize;
        axisData[index].y = partSize / textureSize;
        axisData[index].z = sideTexSize;
    }

    if (centerSize > 0.0f)
    {
        std::fill(axisData.begin() + beginOffset, axisData.begin() + gridSize - endOffset, Vector3(centerSize, centerTexSize, sideTexSize));
    }
}

void TiledDrawData::GenerateTransformData()
{
    const uint32 uCount = static_cast<uint32>(units.size());
    for (uint32 uIndex = 0; uIndex < uCount; ++uIndex)
    {
        Unit& unit = units[uIndex];
        const uint32 vCount = static_cast<uint32>(unit.vertices.size());
        for (uint32 vIndex = 0; vIndex < vCount; ++vIndex)
        {
            unit.transformedVertices[vIndex] = unit.vertices[vIndex] * transformMatr;
        }
    }
}

/* StretchDrawData Implementation */

const uint16 StretchDrawData::indeces[18 * 3] = {
    0, 1, 4,
    1, 5, 4,
    1, 2, 5,
    2, 6, 5,
    2, 3, 6,
    3, 7, 6,

    4, 5, 8,
    5, 9, 8,
    5, 6, 9,
    6, 10, 9,
    6, 7, 10,
    7, 11, 10,

    8, 9, 12,
    9, 12, 13,
    9, 10, 13,
    10, 14, 13,
    10, 11, 14,
    11, 15, 14
};

uint32 StretchDrawData::GetVertexInTrianglesCount() const
{
    switch (type)
    {
    case UIControlBackground::DRAW_STRETCH_HORIZONTAL:
    case UIControlBackground::DRAW_STRETCH_VERTICAL:
        return 18;
    case UIControlBackground::DRAW_STRETCH_BOTH:
        return 18 * 3;
    default:
        DVASSERT(0);
        return 0;
    }
}

void StretchDrawData::GenerateTransformData()
{
    if (usePerPixelAccuracy)
    {
        for (size_t i = 0, sz = vertices.size(); i < sz; ++i)
            transformedVertices[i] = RenderSystem2D::Instance()->GetAlignedVertex(vertices[i] * transformMatr);
    }
    else
    {
        for (size_t i = 0, sz = vertices.size(); i < sz; ++i)
            transformedVertices[i] = vertices[i] * transformMatr;
    }
}

void StretchDrawData::GenerateStretchData()
{
    const Vector2 sizeInTex(sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_WIDTH), sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_HEIGHT));
    const Vector2 offsetInTex(sprite->GetRectOffsetValueForFrame(frame, Sprite::X_OFFSET_TO_ACTIVE), sprite->GetRectOffsetValueForFrame(frame, Sprite::Y_OFFSET_TO_ACTIVE));
    const Vector2& spriteSize = sprite->GetSize();

    const Vector2 xyLeftTopCap(offsetInTex - stretchCap);
    const Vector2 xyRightBottomCap(spriteSize - sizeInTex - offsetInTex - stretchCap);

    const Vector2 xyRealLeftTopCap(Max(0.0f, -xyLeftTopCap.x), Max(0.0f, -xyLeftTopCap.y));
    const Vector2 xyRealRightBottomCap(Max(0.0f, -xyRightBottomCap.x), Max(0.0f, -xyRightBottomCap.y));

    const Vector2 xyNegativeLeftTopCap(Max(0.0f, xyLeftTopCap.x), Max(0.0f, xyLeftTopCap.y));

    const Vector2 scaleFactor = (size - stretchCap * 2.0f) / (spriteSize - stretchCap * 2.0f);

    Vector2 xyPos;
    Vector2 xySize;

    if (UIControlBackground::DRAW_STRETCH_BOTH == type || UIControlBackground::DRAW_STRETCH_HORIZONTAL == type)
    {
        xySize.x = xyRealLeftTopCap.x + xyRealRightBottomCap.x + (sizeInTex.x - xyRealLeftTopCap.x - xyRealRightBottomCap.x) * scaleFactor.x;
        xyPos.x = stretchCap.x + xyNegativeLeftTopCap.x * scaleFactor.x - xyRealLeftTopCap.x;
    }
    else
    {
        xySize.x = sizeInTex.x;
        xyPos.x = offsetInTex.x + (size.x - spriteSize.x) * 0.5f;
    }

    if (UIControlBackground::DRAW_STRETCH_BOTH == type || UIControlBackground::DRAW_STRETCH_VERTICAL == type)
    {
        xySize.y = xyRealLeftTopCap.y + xyRealRightBottomCap.y + (sizeInTex.y - xyRealLeftTopCap.y - xyRealRightBottomCap.y) * scaleFactor.y;
        xyPos.y = stretchCap.y + xyNegativeLeftTopCap.y * scaleFactor.y - xyRealLeftTopCap.y;
    }
    else
    {
        xySize.y = sizeInTex.y;
        xyPos.y = offsetInTex.y + (size.y - spriteSize.y) * 0.5f;
    }

    const Texture* texture = sprite->GetTexture(frame);
    const Vector2 textureSize(float32(texture->GetWidth()), float32(texture->GetHeight()));

    const Vector2 uvPos(sprite->GetRectOffsetValueForFrame(frame, Sprite::X_POSITION_IN_TEXTURE) / textureSize.x,
                        sprite->GetRectOffsetValueForFrame(frame, Sprite::Y_POSITION_IN_TEXTURE) / textureSize.y);

    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Vector2 value(sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_WIDTH),
                  sprite->GetRectOffsetValueForFrame(frame, Sprite::ACTIVE_HEIGHT));
    const Vector2 uvSize = vcs->ConvertVirtualToResource(value, sprite->GetResourceSizeIndex()) / textureSize;
    const Vector2 uvLeftTopCap = vcs->ConvertVirtualToResource(xyRealLeftTopCap, sprite->GetResourceSizeIndex()) / textureSize;
    const Vector2 uvRightBottomCap = vcs->ConvertVirtualToResource(xyRealRightBottomCap, sprite->GetResourceSizeIndex()) / textureSize;

    switch (type)
    {
    case UIControlBackground::DRAW_STRETCH_HORIZONTAL:
    {
        vertices.resize(8);
        transformedVertices.resize(8);
        texCoords.resize(8);

        vertices[0] = Vector2(xyPos.x, xyPos.y);
        vertices[1] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y);
        vertices[2] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y);
        vertices[3] = Vector2(xyPos.x + xySize.x, xyPos.y);

        vertices[4] = Vector2(xyPos.x, xyPos.y + xySize.y);
        vertices[5] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y + xySize.y);
        vertices[6] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y + xySize.y);
        vertices[7] = Vector2(xyPos.x + xySize.x, xyPos.y + xySize.y);

        texCoords[0] = Vector2(uvPos.x, uvPos.y);
        texCoords[1] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y);
        texCoords[2] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y);
        texCoords[3] = Vector2(uvPos.x + uvSize.x, uvPos.y);

        texCoords[4] = Vector2(uvPos.x, uvPos.y + uvSize.y);
        texCoords[5] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y + uvSize.y);
        texCoords[6] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y + uvSize.y);
        texCoords[7] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvSize.y);
    }
    break;
    case UIControlBackground::DRAW_STRETCH_VERTICAL:
    {
        vertices.resize(8);
        transformedVertices.resize(8);
        texCoords.resize(8);

        vertices[0] = Vector2(xyPos.x, xyPos.y);
        vertices[1] = Vector2(xyPos.x, xyPos.y + xyRealLeftTopCap.y);
        vertices[2] = Vector2(xyPos.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);
        vertices[3] = Vector2(xyPos.x, xyPos.y + xySize.y);

        vertices[4] = Vector2(xyPos.x + xySize.x, xyPos.y);
        vertices[5] = Vector2(xyPos.x + xySize.x, xyPos.y + xyRealLeftTopCap.y);
        vertices[6] = Vector2(xyPos.x + xySize.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);
        vertices[7] = Vector2(xyPos.x + xySize.x, xyPos.y + xySize.y);

        texCoords[0] = Vector2(uvPos.x, uvPos.y);
        texCoords[1] = Vector2(uvPos.x, uvPos.y + uvLeftTopCap.y);
        texCoords[2] = Vector2(uvPos.x, uvPos.y + uvSize.y - uvRightBottomCap.y);
        texCoords[3] = Vector2(uvPos.x, uvPos.y + uvSize.y);

        texCoords[4] = Vector2(uvPos.x + uvSize.x, uvPos.y);
        texCoords[5] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvLeftTopCap.y);
        texCoords[6] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvSize.y - uvRightBottomCap.y);
        texCoords[7] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvSize.y);
    }
    break;
    case UIControlBackground::DRAW_STRETCH_BOTH:
    {
        vertices.resize(16);
        transformedVertices.resize(16);
        texCoords.resize(16);

        vertices[0] = Vector2(xyPos.x, xyPos.y);
        vertices[1] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y);
        vertices[2] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y);
        vertices[3] = Vector2(xyPos.x + xySize.x, xyPos.y);

        vertices[4] = Vector2(xyPos.x, xyPos.y + xyRealLeftTopCap.y);
        vertices[5] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y + xyRealLeftTopCap.y);
        vertices[6] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y + xyRealLeftTopCap.y);
        vertices[7] = Vector2(xyPos.x + xySize.x, xyPos.y + xyRealLeftTopCap.y);

        vertices[8] = Vector2(xyPos.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);
        vertices[9] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);
        vertices[10] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);
        vertices[11] = Vector2(xyPos.x + xySize.x, xyPos.y + xySize.y - xyRealRightBottomCap.y);

        vertices[12] = Vector2(xyPos.x, xyPos.y + xySize.y);
        vertices[13] = Vector2(xyPos.x + xyRealLeftTopCap.x, xyPos.y + xySize.y);
        vertices[14] = Vector2(xyPos.x + xySize.x - xyRealRightBottomCap.x, xyPos.y + xySize.y);
        vertices[15] = Vector2(xyPos.x + xySize.x, xyPos.y + xySize.y);

        texCoords[0] = Vector2(uvPos.x, uvPos.y);
        texCoords[1] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y);
        texCoords[2] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y);
        texCoords[3] = Vector2(uvPos.x + uvSize.x, uvPos.y);

        texCoords[4] = Vector2(uvPos.x, uvPos.y + uvLeftTopCap.y);
        texCoords[5] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y + uvLeftTopCap.y);
        texCoords[6] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y + uvLeftTopCap.y);
        texCoords[7] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvLeftTopCap.y);

        texCoords[8] = Vector2(uvPos.x, uvPos.y + uvSize.y - uvRightBottomCap.y);
        texCoords[9] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y + uvSize.y - uvRightBottomCap.y);
        texCoords[10] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y + uvSize.y - uvRightBottomCap.y);
        texCoords[11] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvSize.y - uvRightBottomCap.y);

        texCoords[12] = Vector2(uvPos.x, uvPos.y + uvSize.y);
        texCoords[13] = Vector2(uvPos.x + uvLeftTopCap.x, uvPos.y + uvSize.y);
        texCoords[14] = Vector2(uvPos.x + uvSize.x - uvRightBottomCap.x, uvPos.y + uvSize.y);
        texCoords[15] = Vector2(uvPos.x + uvSize.x, uvPos.y + uvSize.y);
    }
    break;
    }
}

TiledMultilayerData::~TiledMultilayerData()
{
    rhi::ReleaseTextureSet(textureSet);
    rhi::ReleaseSamplerState(samplerState);
}

Vector<TiledMultilayerData::AxisData> TiledMultilayerData::GenerateSingleAxisData(float32 inSize, float32 inTileSize, float32 inStratchCap,
                                                                                  float32 gradientBase, float32 gradientDelta, float32 detailBase, float32 detailDelta,
                                                                                  float32 contourBase, float32 contourStretchBase, float32 contourStretchMax, float32 contourMax,
                                                                                  float32 maskBase, float32 maskStretchBase, float32 maskStretchMax, float32 maskMax)
{
    Vector<AxisData> result;

    int32 tileCount = static_cast<int32>(std::ceil(inSize / inTileSize));
    int32 totalCount = tileCount * 2;
    if (inStratchCap > 0.0f)
        totalCount += 4;
    result.resize(totalCount);

    int32 vid = 0;
    int32 lastBefore = -1;
    int32 firstAfter = totalCount;
    //position tile and gradient
    for (int32 i = 0; i < tileCount; i++)
    {
        float32 posl = inTileSize * i;
        float32 posr = inTileSize * (i + 1);
        posr = Min(posr, inSize);

        result[vid].pos = posl;
        result[vid].texCoordsGradient = result[vid].pos / inSize * gradientDelta + gradientBase;
        result[vid].texCoordsDetail = detailBase;
        vid++;

        if ((posl < inStratchCap) && (posr >= inStratchCap)) //insert stretch line
        {
            result[vid].pos = result[vid + 1].pos = inStratchCap;
            result[vid].texCoordsGradient = result[vid + 1].texCoordsGradient = result[vid].pos / inSize * gradientDelta + gradientBase;
            result[vid].texCoordsDetail = result[vid + 1].texCoordsDetail = detailBase + (result[vid].pos - posl) / inTileSize * detailDelta;
            lastBefore = vid;
            vid += 2;
        }
        if ((posl <= (inSize - inStratchCap)) && (posr > (inSize - inStratchCap))) //insert stretch line
        {
            result[vid].pos = result[vid + 1].pos = inSize - inStratchCap;
            result[vid].texCoordsGradient = result[vid + 1].texCoordsGradient = result[vid].pos / inSize * gradientDelta + gradientBase;
            result[vid].texCoordsDetail = result[vid + 1].texCoordsDetail = detailBase + (result[vid].pos - posl) / inTileSize * detailDelta;
            firstAfter = vid + 1;
            vid += 2;
        }

        result[vid].pos = posr;
        result[vid].texCoordsGradient = result[vid].pos / inSize * gradientDelta + gradientBase;
        result[vid].texCoordsDetail = detailBase + (posr - posl) / inTileSize * detailDelta;
        vid++;
    }

    for (int32 i = 0; i < totalCount; i++)
    {
        if (i <= lastBefore) //before stretch
        {
            float32 val = result[i].pos / inStratchCap;
            result[i].texCoordsMask = maskBase + (maskStretchBase - maskBase) * val;
            result[i].texCoordsContour = contourBase + (contourStretchBase - contourBase) * val;
        }
        else if (i >= firstAfter) //after stretch
        {
            float32 val = (result[i].pos - (inSize - inStratchCap)) / inStratchCap;
            result[i].texCoordsMask = maskStretchMax + (maskMax - maskStretchMax) * val;
            result[i].texCoordsContour = contourStretchMax + (contourMax - contourStretchMax) * val;
        }
        else //in between
        {
            float32 val = (result[i].pos - inStratchCap) / (inSize - 2 * inStratchCap);
            result[i].texCoordsMask = maskStretchBase + (maskStretchMax - maskStretchBase) * val;
            result[i].texCoordsContour = contourStretchBase + (contourStretchMax - contourStretchBase) * val;
        }
    }

    return result;
}

TiledMultilayerData::SingleStretchData TiledMultilayerData::GenerateStretchData(Sprite* sprite)
{
    if ((sprite->GetRectOffsetValueForFrame(0, Sprite::X_OFFSET_TO_ACTIVE) != 0) || (sprite->GetRectOffsetValueForFrame(0, Sprite::Y_OFFSET_TO_ACTIVE) != 0))
    {
        Logger::Error("wrong sprite %s", Sprite::GetPathString(sprite).c_str());
        Logger::Error("texture for sprite atlas for tiled multi-layered should be packed with --disableCropAlpha flag");
    }
    SingleStretchData res;
    int32 resoureceSizeIndex = sprite->GetResourceSizeIndex();

    Vector2 origSize = Vector2(sprite->GetRectOffsetValueForFrame(0, Sprite::ACTIVE_WIDTH), sprite->GetRectOffsetValueForFrame(0, Sprite::ACTIVE_HEIGHT));
    res.uvBase = Vector2(sprite->GetTextureCoordsForFrame(0)[0], sprite->GetTextureCoordsForFrame(0)[1]);
    res.uvMax = Vector2(sprite->GetTextureCoordsForFrame(0)[2], sprite->GetTextureCoordsForFrame(0)[5]);
    Vector2 uvSize = res.uvMax - res.uvBase;
    res.uvCapMin = res.uvBase + uvSize * stretchCap / origSize;
    res.uvCapMax = res.uvBase + uvSize * (origSize - stretchCap) / origSize;

    return res;
}

void TiledMultilayerData::GenerateTileData()
{
    if ((size.x <= 0) || (size.y <= 0))
        return;

    //generate data
    SingleStretchData contourStretchData = GenerateStretchData(contour);
    SingleStretchData maskStretchData = GenerateStretchData(mask);
    Vector2 tileSize = detail->GetSize();
    Vector2 gradientBase = Vector2(gradient->GetTextureCoordsForFrame(0)[0], gradient->GetTextureCoordsForFrame(0)[1]);
    Vector2 gradientDelta = Vector2(gradient->GetTextureCoordsForFrame(0)[2], gradient->GetTextureCoordsForFrame(0)[5]) - gradientBase;
    Vector2 detailBase = Vector2(detail->GetTextureCoordsForFrame(0)[0], detail->GetTextureCoordsForFrame(0)[1]);
    Vector2 detailDelta = Vector2(detail->GetTextureCoordsForFrame(0)[2], detail->GetTextureCoordsForFrame(0)[5]) - detailBase;

    Vector<AxisData> xData = GenerateSingleAxisData(size.x, tileSize.x, stretchCap.x, gradientBase.x, gradientDelta.x, detailBase.x, detailDelta.x,
                                                    contourStretchData.uvBase.x, contourStretchData.uvCapMin.x, contourStretchData.uvCapMax.x, contourStretchData.uvMax.x,
                                                    maskStretchData.uvBase.x, maskStretchData.uvCapMin.x, maskStretchData.uvCapMax.x, maskStretchData.uvMax.x);
    Vector<AxisData> yData = GenerateSingleAxisData(size.y, tileSize.y, stretchCap.y, gradientBase.y, gradientDelta.y, detailBase.y, detailDelta.y,
                                                    contourStretchData.uvBase.y, contourStretchData.uvCapMin.y, contourStretchData.uvCapMax.y, contourStretchData.uvMax.y,
                                                    maskStretchData.uvBase.y, maskStretchData.uvCapMin.y, maskStretchData.uvCapMax.y, maskStretchData.uvMax.y);

    //fill geom
    size_t xLinesCount = xData.size();
    size_t yLinesCount = yData.size();

    DVASSERT(((xLinesCount % 2) == 0) && ((yLinesCount % 2) == 0));

    size_t verticesCount = xData.size() * yData.size();
    vertices.resize(verticesCount);
    transformedVertices.resize(verticesCount);
    texCoordsMask.resize(verticesCount);
    texCoordsDetail.resize(verticesCount);
    texCoordsGradient.resize(verticesCount);
    texCoordsContour.resize(verticesCount);
    size_t vertexId = 0;
    for (size_t y = 0, szy = yLinesCount; y < szy; ++y)
    {
        for (size_t x = 0, szx = xLinesCount; x < szx; ++x)
        {
            vertices[vertexId] = Vector2(xData[x].pos, yData[y].pos);
            texCoordsMask[vertexId] = Vector2(xData[x].texCoordsMask, yData[y].texCoordsMask);
            texCoordsDetail[vertexId] = Vector2(xData[x].texCoordsDetail, yData[y].texCoordsDetail);
            texCoordsGradient[vertexId] = Vector2(xData[x].texCoordsGradient, yData[y].texCoordsGradient);
            texCoordsContour[vertexId] = Vector2(xData[x].texCoordsContour, yData[y].texCoordsContour);
            ++vertexId;
        }
    }
    size_t indexCount = (xLinesCount / 2) * (yLinesCount / 2) * 6;
    indices.resize(indexCount);
    size_t indexId = 0;
    for (size_t y = 0, szy = yLinesCount / 2; y < szy; ++y)
    {
        size_t baseV = y * 2 * xLinesCount;
        size_t nextV = (y * 2 + 1) * xLinesCount;
        for (size_t x = 0, szx = xLinesCount / 2; x < szx; ++x)
        {
            indices[indexId++] = static_cast<uint16>(baseV + x * 2);
            indices[indexId++] = static_cast<uint16>(baseV + x * 2 + 1);
            indices[indexId++] = static_cast<uint16>(nextV + x * 2);
            indices[indexId++] = static_cast<uint16>(baseV + x * 2 + 1);
            indices[indexId++] = static_cast<uint16>(nextV + x * 2 + 1);
            indices[indexId++] = static_cast<uint16>(nextV + x * 2);
        }
    }
}

void TiledMultilayerData::GenerateTransformData(bool usePerPixelAccuracy)
{
    if (usePerPixelAccuracy)
    {
        for (size_t i = 0, sz = vertices.size(); i < sz; ++i)
            transformedVertices[i] = RenderSystem2D::Instance()->GetAlignedVertex(vertices[i] * transformMatr);
    }
    else
    {
        for (size_t i = 0, sz = vertices.size(); i < sz; ++i)
            transformedVertices[i] = vertices[i] * transformMatr;
    }
}
};
