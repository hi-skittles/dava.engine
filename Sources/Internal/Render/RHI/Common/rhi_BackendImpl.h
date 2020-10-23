#ifndef __RHI_IMPL_H__
#define __RHI_IMPL_H__

#include "rhi_Private.h"
#include "rhi_CommonImpl.h"

namespace rhi
{
struct ResetParam;

struct Dispatch
{
    void (*impl_Reset)(const ResetParam&);
    void (*impl_Uninitialize)();
    Api (*impl_HostApi)();
    bool (*impl_NeedRestoreResources)();
    bool (*impl_TextureFormatSupported)(TextureFormat, ProgType);

    void (*impl_InvalidateCache)();
    void (*impl_SyncCPUGPU)(uint64* cpuTimestamp, uint64* gpuTimestamp);

    void (*impl_InitContext)();
    bool (*impl_ValidateSurface)(); //TODO - may be this should be part of opengl only?
    void (*impl_FinishRendering)(); //perform finalization before going to suspend
    void (*impl_ProcessImmediateCommand)(CommonImpl::ImmediateCommand* command); //called from render thread
    void (*impl_FinishFrame)(); //this functions is called from main thread
    void (*impl_ExecuteFrame)(const CommonImpl::Frame&); //should also handle command buffer sync here
    void (*impl_RejectFrame)(const CommonImpl::Frame&); //should also handle command buffer sync here
    bool (*impl_PresentBuffer)();
    void (*impl_ResetBlock)();

    Handle (*impl_VertexBuffer_Create)(const VertexBuffer::Descriptor& desc);
    void (*impl_VertexBuffer_Delete)(Handle, bool);
    bool (*impl_VertexBuffer_Update)(Handle, const void*, uint32, uint32);
    void* (*impl_VertexBuffer_Map)(Handle, uint32, uint32);
    void (*impl_VertexBuffer_Unmap)(Handle);
    bool (*impl_VertexBuffer_NeedRestore)(Handle);

    Handle (*impl_IndexBuffer_Create)(const IndexBuffer::Descriptor& desc);
    void (*impl_IndexBuffer_Delete)(Handle, bool);
    bool (*impl_IndexBuffer_Update)(Handle, const void*, uint32, uint32);
    void* (*impl_IndexBuffer_Map)(Handle, uint32, uint32);
    void (*impl_IndexBuffer_Unmap)(Handle);
    bool (*impl_IndexBuffer_NeedRestore)(Handle);

    Handle (*impl_QueryBuffer_Create)(unsigned maxObjectCount);
    void (*impl_QueryBuffer_Reset)(Handle buf);
    void (*impl_QueryBuffer_Delete)(Handle buf);
    bool (*impl_QueryBuffer_IsReady)(Handle buf);
    bool (*impl_QueryBuffer_ObjectIsReady)(Handle buf, uint32 objectIndex);
    int32 (*impl_QueryBuffer_Value)(Handle buf, uint32 objectIndex);

    Handle (*impl_PerfQuery_Create)();
    void (*impl_PerfQuery_Delete)(Handle query);
    void (*impl_PerfQuery_Reset)(Handle query);
    bool (*impl_PerfQuery_IsReady)(Handle query);
    uint64 (*impl_PerfQuery_Value)(Handle query);

    Handle (*impl_Texture_Create)(const Texture::Descriptor& desc);
    void (*impl_Texture_Delete)(Handle, bool);
    void* (*impl_Texture_Map)(Handle, unsigned, TextureFace);
    void (*impl_Texture_Unmap)(Handle);
    void (*impl_Texture_Update)(Handle, const void*, uint32, TextureFace);
    bool (*impl_Texture_NeedRestore)(Handle);

    Handle (*impl_PipelineState_Create)(const PipelineState::Descriptor&);
    void (*impl_PipelineState_Delete)(Handle);
    Handle (*impl_PipelineState_CreateVertexConstBuffer)(Handle, uint32);
    Handle (*impl_PipelineState_CreateFragmentConstBuffer)(Handle, uint32);

    bool (*impl_ConstBuffer_SetConst)(Handle, uint32, uint32, const float*);
    bool (*impl_ConstBuffer_SetConst1fv)(Handle, uint32, uint32, const float*, uint32);
    void (*impl_ConstBuffer_Delete)(Handle);

    Handle (*impl_DepthStencilState_Create)(const DepthStencilState::Descriptor&);
    void (*impl_DepthStencilState_Delete)(Handle);

    Handle (*impl_SamplerState_Create)(const SamplerState::Descriptor&);
    void (*impl_SamplerState_Delete)(Handle);

    Handle (*impl_Renderpass_Allocate)(const RenderPassConfig&, uint32, Handle*);
    void (*impl_Renderpass_Begin)(Handle);
    void (*impl_Renderpass_End)(Handle);

    Handle (*impl_SyncObject_Create)();
    void (*impl_SyncObject_Delete)(Handle);
    bool (*impl_SyncObject_IsSignaled)(Handle);

    void (*impl_CommandBuffer_Begin)(Handle);
    void (*impl_CommandBuffer_End)(Handle, Handle);
    void (*impl_CommandBuffer_SetPipelineState)(Handle, Handle, uint32 vdecl);
    void (*impl_CommandBuffer_SetCullMode)(Handle, CullMode);
    void (*impl_CommandBuffer_SetScissorRect)(Handle, ScissorRect);
    void (*impl_CommandBuffer_SetViewport)(Handle, Viewport);
    void (*impl_CommandBuffer_SetFillMode)(Handle, FillMode);
    void (*impl_CommandBuffer_SetVertexData)(Handle, Handle, uint32);
    void (*impl_CommandBuffer_SetVertexConstBuffer)(Handle, uint32, Handle);
    void (*impl_CommandBuffer_SetVertexTexture)(Handle, uint32, Handle);
    void (*impl_CommandBuffer_SetIndices)(Handle, Handle);
    void (*impl_CommandBuffer_SetQueryIndex)(Handle, uint32);
    void (*impl_CommandBuffer_SetQueryBuffer)(Handle, Handle);
    void (*impl_CommandBuffer_IssueTimestampQuery)(Handle, Handle);
    void (*impl_CommandBuffer_SetFragmentConstBuffer)(Handle, uint32, Handle);
    void (*impl_CommandBuffer_SetFragmentTexture)(Handle, uint32, Handle);
    void (*impl_CommandBuffer_SetDepthStencilState)(Handle, Handle);
    void (*impl_CommandBuffer_SetSamplerState)(Handle, const Handle);
    void (*impl_CommandBuffer_DrawPrimitive)(Handle, PrimitiveType, uint32);
    void (*impl_CommandBuffer_DrawIndexedPrimitive)(Handle, PrimitiveType, uint32, uint32, uint32, uint32);
    void (*impl_CommandBuffer_DrawInstancedPrimitive)(Handle, PrimitiveType, uint32, uint32);
    void (*impl_CommandBuffer_DrawInstancedIndexedPrimitive)(Handle, PrimitiveType, uint32, uint32, uint32, uint32, uint32, uint32);
    void (*impl_CommandBuffer_SetMarker)(Handle, const char*);
};

void SetDispatchTable(const Dispatch& dispatch);

//------------------------------------------------------------------------------
#if defined(__DAVAENGINE_IPHONE__)
bool rhi_MetalIsSupported(); 
#endif

} // namespace rhi


#endif // __RHI_IMPL_H__
