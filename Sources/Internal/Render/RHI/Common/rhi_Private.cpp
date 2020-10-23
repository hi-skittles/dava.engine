#include "rhi_BackendImpl.h"
#include "rhi_Utils.h"
#include "../rhi_Public.h"
#if defined(__DAVAENGINE_WIN32__)
    #include "../DX9/rhi_DX9.h"
    #include "../DX11/rhi_DX11.h"
    #include "../GLES2/rhi_GLES2.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
    #include "../DX11/rhi_DX11.h"
#elif defined(__DAVAENGINE_MACOS__)
    #include "../GLES2/rhi_GLES2.h"
#elif defined(__DAVAENGINE_IPHONE__)
    #include "../Metal/rhi_Metal.h"
    #include "../GLES2/rhi_GLES2.h"
#elif defined(__DAVAENGINE_ANDROID__)
    #include "../GLES2/rhi_GLES2.h"
#else
#endif

#include "../NullRenderer/rhi_NullRenderer.h"

#include "Logger/Logger.h"
#include "Concurrency/Spinlock.h"
#include "Concurrency/Thread.h"
#include "MemoryManager/MemoryProfiler.h"

using DAVA::Logger;

namespace rhi
{
uint32 stat_DIP = DAVA::InvalidIndex;
uint32 stat_DP = DAVA::InvalidIndex;
uint32 stat_DTL = DAVA::InvalidIndex;
uint32 stat_DTS = DAVA::InvalidIndex;
uint32 stat_DLL = DAVA::InvalidIndex;
uint32 stat_SET_PS = DAVA::InvalidIndex;
uint32 stat_SET_SS = DAVA::InvalidIndex;
uint32 stat_SET_TEX = DAVA::InvalidIndex;
uint32 stat_SET_CB = DAVA::InvalidIndex;
uint32 stat_SET_VB = DAVA::InvalidIndex;
uint32 stat_SET_IB = DAVA::InvalidIndex;

static Dispatch _Impl = {};
static RenderDeviceCaps renderDeviceCaps;

void SetDispatchTable(const Dispatch& dispatch)
{
    _Impl = dispatch;
}

bool ApiIsSupported(Api api)
{
    bool supported = false;

    switch (api)
    {
    case RHI_DX9:
    {
        #if defined(__DAVAENGINE_WIN32__)
        supported = true;
        #endif
    }
    break;

    case RHI_DX11:
    {
        #if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
        supported = true;
        #endif
    }
    break;

    case RHI_METAL:
    {
        #if defined(__DAVAENGINE_IPHONE__) && TARGET_IPHONE_SIMULATOR != 1
        supported = rhi_MetalIsSupported();
        #endif
    }
    break;

    case RHI_GLES2:
        #if !defined(__DAVAENGINE_WIN_UAP__) && !defined(__DAVAENGINE_LINUX__)
        supported = true;
        #endif
        break;

    case RHI_NULL_RENDERER:
        supported = true;
        break;

    default:
        DVASSERT(!"kaboom!"); // to shut up goddamn warning
    }

    return supported;
}

void InitializeImplementation(Api api, const InitParam& param)
{
    switch (api)
    {
#if defined(__DAVAENGINE_WIN32__)
    case RHI_DX9:
        dx9_Initialize(param);
        break;
#endif

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    case RHI_DX11:
        dx11_Initialize(param);
        break;
#endif
            
#if !defined(__DAVAENGINE_WIN_UAP__) && !defined(__DAVAENGINE_LINUX__)
    case RHI_GLES2:
        gles2_Initialize(param);
        break;
#endif

#if defined(__DAVAENGINE_IPHONE__)
#if !(TARGET_IPHONE_SIMULATOR == 1)
    case RHI_METAL:
        metal_Initialize(param);
        break;
#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
#endif

    case RHI_NULL_RENDERER:
        nullRenderer_Initialize(param);
        break;

    default:
    {
        DVASSERT(false, "Unsupported rendering api");
    }
    }
}

void UninitializeImplementation()
{
    (*_Impl.impl_Uninitialize)();
}

void Reset(const ResetParam& param)
{
    (*_Impl.impl_Reset)(param);
}

bool NeedRestoreResources()
{
    return (*_Impl.impl_NeedRestoreResources)();
}

Api HostApi()
{
    return (*_Impl.impl_HostApi)();
}

bool TextureFormatSupported(TextureFormat format, ProgType progType)
{
    return (*_Impl.impl_TextureFormatSupported)(format, progType);
}

const RenderDeviceCaps& DeviceCaps()
{
    return renderDeviceCaps;
}

void InvalidateCache()
{
    if (_Impl.impl_InvalidateCache)
        (*_Impl.impl_InvalidateCache)();
}

namespace DispatchPlatform
{
void InitContext()
{
    (*_Impl.impl_InitContext)();
}
bool ValidateSurface()
{
    return (*_Impl.impl_ValidateSurface)();
}
void FinishRendering()
{
    (*_Impl.impl_FinishRendering)();
}
void ProcessImmediateCommand(CommonImpl::ImmediateCommand* command)
{
    (*_Impl.impl_ProcessImmediateCommand)(command);
}
void FinishFrame()
{
    (*_Impl.impl_FinishFrame)();
}
void ExecuteFrame(const CommonImpl::Frame& frame)
{
    (*_Impl.impl_ExecuteFrame)(frame);
}
void RejectFrame(const CommonImpl::Frame& frame)
{
    (*_Impl.impl_RejectFrame)(frame);
}
bool PresentBuffer()
{
    return (*_Impl.impl_PresentBuffer)();
}
void ResetBlock()
{
    (*_Impl.impl_ResetBlock)();
}
}

void SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
    _Impl.impl_SyncCPUGPU(cpuTimestamp, gpuTimestamp);
}

//////////////////////////////////////////////////////////////////////////

namespace VertexBuffer
{
Handle Create(const Descriptor& desc)
{
#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    return (*_Impl.impl_VertexBuffer_Create)(desc);
#else
    Handle handle = (*_Impl.impl_VertexBuffer_Create)(desc);
    if (handle != rhi::InvalidHandle)
    {
        DAVA_MEMORY_PROFILER_GPU_ALLOC(handle, desc.size, DAVA::ALLOC_GPU_RDO_VERTEX);
    }
    return handle;
#endif
}

void Delete(Handle vb, bool forceExecute)
{
    if (vb != rhi::InvalidHandle)
    {
        #if defined(DAVA_MEMORY_PROFILING_ENABLE)
        DAVA_MEMORY_PROFILER_GPU_DEALLOC(vb, DAVA::ALLOC_GPU_RDO_VERTEX);        
        #endif
        (*_Impl.impl_VertexBuffer_Delete)(vb, forceExecute);
    }
}

bool Update(Handle vb, const void* data, uint32 offset, uint32 size)
{
    return (*_Impl.impl_VertexBuffer_Update)(vb, data, offset, size);
}

void* Map(Handle vb, uint32 offset, uint32 size)
{
    DAVA_MEMORY_PROFILER_ALLOC_SCOPE(DAVA::ALLOC_POOL_RHI_VERTEX_MAP);
    return (*_Impl.impl_VertexBuffer_Map)(vb, offset, size);
}

void Unmap(Handle vb)
{
    return (*_Impl.impl_VertexBuffer_Unmap)(vb);
}

bool NeedRestore(Handle vb)
{
    return (*_Impl.impl_VertexBuffer_NeedRestore)(vb);
}

} // namespace VertexBuffer

//////////////////////////////////////////////////////////////////////////

namespace IndexBuffer
{
Handle Create(const Descriptor& desc)
{
#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    return (*_Impl.impl_IndexBuffer_Create)(desc);
#else
    Handle handle = (*_Impl.impl_IndexBuffer_Create)(desc);
    if (handle != rhi::InvalidHandle)
    {
        DAVA_MEMORY_PROFILER_GPU_ALLOC(handle, desc.size, DAVA::ALLOC_GPU_RDO_INDEX);
    }
    return handle;
#endif
}

void Delete(Handle ib, bool forceExecute)
{
    if (ib != InvalidHandle)
    {
        #if defined(DAVA_MEMORY_PROFILING_ENABLE)
        DAVA_MEMORY_PROFILER_GPU_DEALLOC(ib, DAVA::ALLOC_GPU_RDO_INDEX);        
        #endif
        (*_Impl.impl_IndexBuffer_Delete)(ib, forceExecute);
    }
}

bool Update(Handle vb, const void* data, uint32 offset, uint32 size)
{
    return (*_Impl.impl_IndexBuffer_Update)(vb, data, offset, size);
}

void* Map(Handle vb, uint32 offset, uint32 size)
{
    DAVA_MEMORY_PROFILER_ALLOC_SCOPE(DAVA::ALLOC_POOL_RHI_INDEX_MAP);
    return (*_Impl.impl_IndexBuffer_Map)(vb, offset, size);
}

void Unmap(Handle vb)
{
    return (*_Impl.impl_IndexBuffer_Unmap)(vb);
}

bool NeedRestore(Handle ib)
{
    return (*_Impl.impl_IndexBuffer_NeedRestore)(ib);
}

} // namespace IndexBuffer

////////////////////////////////////////////////////////////////////////////////

namespace QueryBuffer
{
Handle Create(uint32 maxObjectCount)
{
    return (*_Impl.impl_QueryBuffer_Create)(maxObjectCount);
}

void Reset(Handle buf)
{
    (*_Impl.impl_QueryBuffer_Reset)(buf);
}

void Delete(Handle buf)
{
    (*_Impl.impl_QueryBuffer_Delete)(buf);
}

bool BufferIsReady(Handle buf)
{
    return (*_Impl.impl_QueryBuffer_IsReady)(buf);
}

bool IsReady(Handle buf, uint32 objectIndex)
{
    return (*_Impl.impl_QueryBuffer_ObjectIsReady)(buf, objectIndex);
}

int32 Value(Handle buf, uint32 objectIndex)
{
    return (*_Impl.impl_QueryBuffer_Value)(buf, objectIndex);
}
}

////////////////////////////////////////////////////////////////////////////////

namespace PerfQuery
{
Handle Create()
{
    return (*_Impl.impl_PerfQuery_Create)();
}

void Delete(Handle query)
{
    (*_Impl.impl_PerfQuery_Delete)(query);
}

void Reset(Handle query)
{
    (*_Impl.impl_PerfQuery_Reset)(query);
}

bool IsReady(Handle query)
{
    return (*_Impl.impl_PerfQuery_IsReady)(query);
}

uint64 Value(Handle query)
{
    return (*_Impl.impl_PerfQuery_Value)(query);
}
}

////////////////////////////////////////////////////////////////////////////////

namespace Texture
{
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
uint32 TextureSizeForProfiling(Handle handle, const Texture::Descriptor& desc)
{
    uint32 size = 0;
    uint32 nfaces = desc.type == TEXTURE_TYPE_CUBE ? 6 : 1;
    for (uint32 curFace = 0; curFace < nfaces; ++curFace)
    {
        for (uint32 curLevel = 0; curLevel < desc.levelCount; ++curLevel)
        {
            if (desc.initialData[curFace * desc.levelCount + curLevel] != nullptr)
            {
                Size2i sz = TextureExtents(Size2i(desc.width, desc.height), curLevel);
                uint32 n = TextureSize(desc.format, sz.dx, sz.dy);
                size += n;
            }
        }
    }
    return size;
}
#endif

Handle Create(const Texture::Descriptor& desc)
{
#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    return (*_Impl.impl_Texture_Create)(desc);
#else
    Handle handle = (*_Impl.impl_Texture_Create)(desc);
    if (handle != rhi::InvalidHandle)
    {
        uint32 size = TextureSizeForProfiling(handle, desc);
        DAVA_MEMORY_PROFILER_GPU_ALLOC(handle, size, DAVA::ALLOC_GPU_TEXTURE);
    }
    return handle;
#endif
}

void Delete(Handle tex, bool forceExecute)
{
    if (tex != InvalidHandle)
    {
        #if defined(DAVA_MEMORY_PROFILING_ENABLE)
        DAVA_MEMORY_PROFILER_GPU_DEALLOC(tex, DAVA::ALLOC_GPU_TEXTURE);    
        #endif
        (*_Impl.impl_Texture_Delete)(tex, forceExecute);
    }
}

void* Map(Handle tex, unsigned level, TextureFace face)
{
    DAVA_MEMORY_PROFILER_ALLOC_SCOPE(DAVA::ALLOC_POOL_RHI_TEXTURE_MAP);
    return (*_Impl.impl_Texture_Map)(tex, level, face);
}

void Unmap(Handle tex)
{
    return (*_Impl.impl_Texture_Unmap)(tex);
}

void Update(Handle tex, const void* data, uint32 level, TextureFace face)
{
    return (*_Impl.impl_Texture_Update)(tex, data, level, face);
}

bool NeedRestore(Handle tex)
{
    return (*_Impl.impl_Texture_NeedRestore)(tex);
}
};

////////////////////////////////////////////////////////////////////////////////

namespace PipelineState
{
Handle Create(const Descriptor& desc)
{
    return (*_Impl.impl_PipelineState_Create)(desc);
}

void Delete(Handle ps)
{
    return (*_Impl.impl_PipelineState_Delete)(ps);
}

Handle CreateVertexConstBuffer(Handle ps, uint32 bufIndex)
{
    return (*_Impl.impl_PipelineState_CreateVertexConstBuffer)(ps, bufIndex);
}

Handle CreateFragmentConstBuffer(Handle ps, uint32 bufIndex)
{
    return (*_Impl.impl_PipelineState_CreateFragmentConstBuffer)(ps, bufIndex);
}

} // namespace PipelineState

//////////////////////////////////////////////////////////////////////////

namespace ConstBuffer
{
bool SetConst(Handle cb, uint32 constIndex, uint32 constCount, const float* data)
{
    return (*_Impl.impl_ConstBuffer_SetConst)(cb, constIndex, constCount, data);
}

bool SetConst(Handle cb, uint32 constIndex, uint32 constSubIndex, const float* data, uint32 dataCount)
{
    return (*_Impl.impl_ConstBuffer_SetConst1fv)(cb, constIndex, constSubIndex, data, dataCount);
}

void Delete(Handle cb)
{
    if (cb != InvalidHandle)
        (*_Impl.impl_ConstBuffer_Delete)(cb);
}

} // namespace ConstBuffer

//////////////////////////////////////////////////////////////////////////

namespace DepthStencilState
{
Handle Create(const Descriptor& desc)
{
    return (*_Impl.impl_DepthStencilState_Create)(desc);
}

void Delete(Handle state)
{
    (*_Impl.impl_DepthStencilState_Delete)(state);
}
}

//////////////////////////////////////////////////////////////////////////

namespace SamplerState
{
Handle Create(const Descriptor& desc)
{
    return (*_Impl.impl_SamplerState_Create)(desc);
}

void Delete(Handle state)
{
    (*_Impl.impl_SamplerState_Delete)(state);
}
}

//////////////////////////////////////////////////////////////////////////

namespace RenderPass
{
Handle Allocate(const RenderPassConfig& passDesc, uint32 cmdBufCount, Handle* cmdBuf)
{
    return (*_Impl.impl_Renderpass_Allocate)(passDesc, cmdBufCount, cmdBuf);
}

void Begin(Handle pass)
{
    return (*_Impl.impl_Renderpass_Begin)(pass);
}

void End(Handle pass)
{
    return (*_Impl.impl_Renderpass_End)(pass);
}
}

//////////////////////////////////////////////////////////////////////////

namespace SyncObject
{
Handle Create()
{
    return (*_Impl.impl_SyncObject_Create)();
}

void Delete(Handle obj)
{
    (*_Impl.impl_SyncObject_Delete)(obj);
}

bool IsSignaled(Handle obj)
{
    return (*_Impl.impl_SyncObject_IsSignaled)(obj);
}
}

//////////////////////////////////////////////////////////////////////////

namespace CommandBuffer
{
void Begin(Handle cmdBuf)
{
    (*_Impl.impl_CommandBuffer_Begin)(cmdBuf);
}

void End(Handle cmdBuf, Handle syncObject)
{
    (*_Impl.impl_CommandBuffer_End)(cmdBuf, syncObject);
}

void SetPipelineState(Handle cmdBuf, Handle ps, uint32 layout)
{
    (*_Impl.impl_CommandBuffer_SetPipelineState)(cmdBuf, ps, layout);
}

void SetCullMode(Handle cmdBuf, CullMode mode)
{
    (*_Impl.impl_CommandBuffer_SetCullMode)(cmdBuf, mode);
}

void SetScissorRect(Handle cmdBuf, ScissorRect rect)
{
    (*_Impl.impl_CommandBuffer_SetScissorRect)(cmdBuf, rect);
}

void SetViewport(Handle cmdBuf, Viewport vp)
{
    (*_Impl.impl_CommandBuffer_SetViewport)(cmdBuf, vp);
}

void SetFillMode(Handle cmdBuf, FillMode mode)
{
    (*_Impl.impl_CommandBuffer_SetFillMode)(cmdBuf, mode);
}

void SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    (*_Impl.impl_CommandBuffer_SetVertexData)(cmdBuf, vb, streamIndex);
}

void SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    (*_Impl.impl_CommandBuffer_SetVertexConstBuffer)(cmdBuf, bufIndex, buffer);
}

void SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    (*_Impl.impl_CommandBuffer_SetVertexTexture)(cmdBuf, unitIndex, tex);
}

void SetIndices(Handle cmdBuf, Handle ib)
{
    (*_Impl.impl_CommandBuffer_SetIndices)(cmdBuf, ib);
}

void SetQueryBuffer(Handle cmdBuf, Handle queryBuf)
{
    (*_Impl.impl_CommandBuffer_SetQueryBuffer)(cmdBuf, queryBuf);
}

void SetQueryIndex(Handle cmdBuf, uint32 index)
{
    (*_Impl.impl_CommandBuffer_SetQueryIndex)(cmdBuf, index);
}
void IssueTimestampQuery(Handle cmdBuf, Handle perfQuery)
{
    (*_Impl.impl_CommandBuffer_IssueTimestampQuery)(cmdBuf, perfQuery);
}

void SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buf)
{
    (*_Impl.impl_CommandBuffer_SetFragmentConstBuffer)(cmdBuf, bufIndex, buf);
}

void SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    (*_Impl.impl_CommandBuffer_SetFragmentTexture)(cmdBuf, unitIndex, tex);
}

void SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    (*_Impl.impl_CommandBuffer_SetDepthStencilState)(cmdBuf, depthStencilState);
}

void SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    (*_Impl.impl_CommandBuffer_SetSamplerState)(cmdBuf, samplerState);
}

void DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    (*_Impl.impl_CommandBuffer_DrawPrimitive)(cmdBuf, type, count);
}

void DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 primCount, uint32 vertexCount, uint32 firstVertex, uint32 startIndex)
{
    (*_Impl.impl_CommandBuffer_DrawIndexedPrimitive)(cmdBuf, type, primCount, vertexCount, firstVertex, startIndex);
}

void DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count)
{
    (*_Impl.impl_CommandBuffer_DrawInstancedPrimitive)(cmdBuf, type, instCount, count);
}

void DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 primCount, uint32 vertexCount, uint32 firstVertex, uint32 startIndex, uint32 baseInstance)
{
    (*_Impl.impl_CommandBuffer_DrawInstancedIndexedPrimitive)(cmdBuf, type, instCount, primCount, vertexCount, firstVertex, startIndex, baseInstance);
}

void SetMarker(Handle cmdBuf, const char* text)
{
    (*_Impl.impl_CommandBuffer_SetMarker)(cmdBuf, text);
}

} // namespace CommandBuffer

namespace MutableDeviceCaps
{
RenderDeviceCaps& Get()
{
    return renderDeviceCaps;
}
}

} //namespace rhi
