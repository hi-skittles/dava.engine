#ifndef __RHI_DX9_H__
#define __RHI_DX9_H__

#include "../rhi_Public.h"
#include "../Common/rhi_Private.h"
#include "../Common/rhi_BackendImpl.h"

namespace rhi
{
void dx9_Initialize(const InitParam& param);

struct DX9Command;

namespace VertexBufferDX9
{
void SetupDispatch(Dispatch* dispatch);
void Init(uint32 maxCount);
void SetToRHI(Handle vb, unsigned stream_i, unsigned offset, unsigned stride);
void ReleaseAll();
void ReCreateAll();
void LogUnrestoredBacktraces();
unsigned NeedRestoreCount();
}

namespace IndexBufferDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle vb);
void ReleaseAll();
void ReCreateAll();
void LogUnrestoredBacktraces();
unsigned NeedRestoreCount();
}

namespace QueryBufferDX9
{
void SetupDispatch(Dispatch* dispatch);

void SetQueryIndex(Handle buf, uint32 objectIndex);
void QueryComplete(Handle buf);
bool QueryIsCompleted(Handle buf);

void ReleaseQueryPool();
void ReleaseAll();
}

namespace PerfQueryDX9
{
void SetupDispatch(Dispatch* dispatch);

void IssueTimestampQuery(Handle handle);
void BeginMeasurment();
void EndMeasurment();

void ReleaseAll();

void ObtainPerfQueryMeasurment();
void ReleasePerfQueryPool();
}

namespace PipelineStateDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
unsigned VertexLayoutStride(Handle ps, uint32 stream);
void SetToRHI(Handle ps, uint32 layoutUID);
void SetupVertexStreams(Handle ps, uint32 layoutUID, uint32 instCount);
}

namespace ConstBufferDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void InitializeRingBuffer(uint32 size);
const void* Instance(Handle cb);
void SetToRHI(Handle cb, const void* instData);
void InvalidateAllConstBufferInstances();
}

namespace TextureDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle tex, unsigned unitIndex);
void SetAsRenderTarget(Handle tex, unsigned target_i = 0, TextureFace face = TEXTURE_FACE_NONE);
void SetAsDepthStencil(Handle tex);
void ReleaseAll();
void ReCreateAll();
void LogUnrestoredBacktraces();
void ResolveMultisampling(Handle from, Handle to);
unsigned NeedRestoreCount();
}

namespace DepthStencilStateDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle state);
}

namespace SamplerStateDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle state);
}

namespace RenderPassDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace CommandBufferDX9
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}


#define DX9_CALL(code, name) \
{ \
    HRESULT hr = code; \
\
    if (FAILED(hr)) \
    { \
        Logger::Error("%s failed (%08X):\n%s\n", name, hr, D3D9ErrorText(hr)); \
    } \
}

struct
DX9Command
{
    enum Func
    {
        NOP = 0,

        CREATE_VERTEX_BUFFER = 11,
        LOCK_VERTEX_BUFFER = 12,
        UNLOCK_VERTEX_BUFFER = 13,
        UPDATE_VERTEX_BUFFER = 14,

        CREATE_INDEX_BUFFER = 21,
        LOCK_INDEX_BUFFER = 22,
        UNLOCK_INDEX_BUFFER = 23,
        UPDATE_INDEX_BUFFER = 24,

        CREATE_TEXTURE = 30,
        CREATE_CUBE_TEXTURE = 31,
        GET_TEXTURE_SURFACE_LEVEL = 32,
        GET_CUBE_SURFACE_LEVEL = 33,
        SET_TEXTURE_AUTOGEN_FILTER_TYPE = 34,
        LOCK_TEXTURE_RECT = 35,
        UNLOCK_TEXTURE_RECT = 36,
        LOCK_CUBETEXTURE_RECT = 37,
        UNLOCK_CUBETEXTURE_RECT = 38,
        GET_RENDERTARGET_DATA = 39,
        UPDATE_TEXTURE_LEVEL = 40,
        UPDATE_CUBETEXTURE_LEVEL = 41,

        CREATE_VERTEX_SHADER = 51,
        CREATE_PIXEL_SHADER = 52,
        CREATE_VERTEX_DECLARATION = 53,

        GET_QUERY_DATA = 61,

        QUERY_INTERFACE = 101,
        RELEASE = 102,

        READ_TEXTURE_LEVEL,
        READ_CUBETEXTURE_LEVEL,

        CREATE_RENDER_TARGET,
        CREARE_DEPTHSTENCIL_SURFACE,

        SYNC_CPU_GPU,
    };

    Func func;
    uint64 arg[12];
    long retval;
};

void ExecDX9(DX9Command* cmd, uint32 cmdCount, bool forceExecute);

//==============================================================================
}
#endif // __RHI_DX9_H__
