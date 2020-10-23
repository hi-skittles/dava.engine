#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_CommonImpl.h"
#include "../Common/rhi_Private.h"
#include "../Common/RenderLoop.h"

#include <cstring>

namespace rhi
{
Dispatch DispatchNullRenderer = {};

//////////////////////////////////////////////////////////////////////////

void null_Reset(const ResetParam&)
{
}

void null_Uninitialize()
{
}

Api null_HostApi()
{
    return RHI_NULL_RENDERER;
}

bool null_NeedRestoreResources()
{
    return false;
}

bool null_TextureFormatSupported(TextureFormat format, ProgType)
{
    return true;
}

/////////////////////////////////////////////////////////////////////////

void null_InvalidateCache()
{
}
void null_SyncCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
    *cpuTimestamp = *gpuTimestamp = 0;
}

//////////////////////////////////////////////////////////////////////////

void null_InitContext()
{
    static const char* NULL_RENDERER_DEVICE = "NullRenderer Device";

    std::strncpy(MutableDeviceCaps::Get().deviceDescription, NULL_RENDERER_DEVICE, 127);
}

bool null_ValidateSurface()
{
    return true;
}

void null_FinishRendering()
{
}

void null_ProcessImmediateCommand(CommonImpl::ImmediateCommand* command)
{
}

void null_FinishFrame()
{
}

void null_ExecuteFrame(const CommonImpl::Frame&)
{
}

void null_RejectFrame(const CommonImpl::Frame&)
{
}

bool null_PresentBuffer()
{
    return true;
}

void null_ResetBlock()
{
}

//////////////////////////////////////////////////////////////////////////

void nullRenderer_Initialize(const InitParam& param)
{
    if (param.maxVertexBufferCount)
        VertexBufferNull::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferNull::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferNull::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureNull::Init(param.maxTextureCount);

    if (param.maxSamplerStateCount)
        SamplerStateNull::Init(param.maxSamplerStateCount);
    if (param.maxPipelineStateCount)
        PipelineStateNull::Init(param.maxPipelineStateCount);
    if (param.maxDepthStencilStateCount)
        DepthStencilStateNull::Init(param.maxDepthStencilStateCount);
    if (param.maxRenderPassCount)
        RenderPassNull::Init(param.maxRenderPassCount);
    if (param.maxCommandBuffer)
        CommandBufferNull::Init(param.maxCommandBuffer);

    DispatchNullRenderer.impl_Reset = null_Reset;
    DispatchNullRenderer.impl_Uninitialize = null_Uninitialize;
    DispatchNullRenderer.impl_HostApi = null_HostApi;
    DispatchNullRenderer.impl_NeedRestoreResources = null_NeedRestoreResources;
    DispatchNullRenderer.impl_TextureFormatSupported = null_TextureFormatSupported;

    DispatchNullRenderer.impl_InvalidateCache = null_InvalidateCache;
    DispatchNullRenderer.impl_SyncCPUGPU = null_SyncCPUGPU;

    DispatchNullRenderer.impl_InitContext = null_InitContext;
    DispatchNullRenderer.impl_ValidateSurface = null_ValidateSurface;
    DispatchNullRenderer.impl_FinishRendering = null_FinishRendering;
    DispatchNullRenderer.impl_ProcessImmediateCommand = null_ProcessImmediateCommand;
    DispatchNullRenderer.impl_FinishFrame = null_FinishFrame;
    DispatchNullRenderer.impl_ExecuteFrame = null_ExecuteFrame;
    DispatchNullRenderer.impl_RejectFrame = null_RejectFrame;
    DispatchNullRenderer.impl_PresentBuffer = null_PresentBuffer;
    DispatchNullRenderer.impl_ResetBlock = null_ResetBlock;

    VertexBufferNull::SetupDispatch(&DispatchNullRenderer);
    IndexBufferNull::SetupDispatch(&DispatchNullRenderer);
    QueryBufferNull::SetupDispatch(&DispatchNullRenderer);
    PerfQueryNull::SetupDispatch(&DispatchNullRenderer);
    TextureNull::SetupDispatch(&DispatchNullRenderer);
    PipelineStateNull::SetupDispatch(&DispatchNullRenderer);
    ConstBufferNull::SetupDispatch(&DispatchNullRenderer);
    DepthStencilStateNull::SetupDispatch(&DispatchNullRenderer);
    SamplerStateNull::SetupDispatch(&DispatchNullRenderer);
    RenderPassNull::SetupDispatch(&DispatchNullRenderer);
    SyncObjectNull::SetupDispatch(&DispatchNullRenderer);
    CommandBufferNull::SetupDispatch(&DispatchNullRenderer);

    SetDispatchTable(DispatchNullRenderer);
}
}
