#ifndef __RHI_GLES2_H__
#define __RHI_GLES2_H__

#include "../Common/rhi_Private.h"
#include "../Common/rhi_BackendImpl.h"

namespace rhi
{
struct InitParam;
struct GLCommand;

void gles2_Initialize(const InitParam& param);

namespace VertexBufferGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle vb);
void ReCreateAll();
unsigned NeedRestoreCount();
}

namespace IndexBufferGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
IndexSize SetToRHI(Handle ib);
void ReCreateAll();
unsigned NeedRestoreCount();
}

namespace QueryBufferGLES2
{
void SetupDispatch(Dispatch* dispatch);

void SetQueryIndex(Handle buf, uint32 objectIndex);
void QueryComplete(Handle buf);
bool QueryIsCompleted(Handle buf);

void ReleaseQueryObjectsPool();
}
namespace PerfQueryGLES2
{
void SetupDispatch(Dispatch* dispatch);

void ObtainPerfQueryResults();

void IssueQuery(Handle handle);
void SkipQuery(Handle handle);

void ReleaseQueryObjectsPool();
}

namespace TextureGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle tex, unsigned unit_i, uint32 base_i = DAVA::InvalidIndex);
unsigned GetFrameBuffer(const Handle* color, const TextureFace* face, const unsigned* level, uint32 colorCount, Handle depth);
void ResolveMultisampling(Handle from, Handle to);
Size2i Size(Handle tex);
void ReCreateAll();
unsigned NeedRestoreCount();
void InvalidateCache();
}

namespace SamplerStateGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle hstate);
}

namespace PipelineStateGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle ps);
void SetVertexDeclToRHI(Handle ps, uint32 vdeclUID, uint32 firstVertex, uint32 vertexStreamCount, const Handle* vb);
uint32 VertexSamplerCount(Handle ps);
uint32 ProgramUid(Handle ps);
void InvalidateCache();
void InvalidateVattrCache();
}

namespace DepthStencilStateGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle hstate);
void InvalidateCache();
}
namespace ConstBufferGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void InitializeRingBuffer(uint32 size);

void SetToRHI(Handle cb, uint32 progUid, const void* instData);
const void* Instance(Handle cb);
}

namespace RenderPassGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace CommandBufferGLES2
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

struct GLCommand
{
    enum Func
    {
        NOP,

        GEN_BUFFERS,
        BIND_BUFFER,
        RESTORE_VERTEX_BUFFER,
        RESTORE_INDEX_BUFFER,
        DELETE_BUFFERS,
        BUFFER_DATA,
        BUFFER_SUBDATA,

        GEN_FRAMEBUFFERS,
        GEN_RENDERBUFFERS,
        BIND_FRAMEBUFFER,
        FRAMEBUFFER_TEXTURE,
        FRAMEBUFFER_STATUS,
        BIND_RENDERBUFFER,
        RENDERBUFFER_STORAGE,
        DELETE_RENDERBUFFERS,
        FRAMEBUFFER_RENDERBUFFER,
        DRAWBUFFERS,
        DELETE_FRAMEBUFFERS,

        GEN_TEXTURES,
        SET_ACTIVE_TEXTURE,
        BIND_TEXTURE,
        RESTORE_TEXTURE0,
        DELETE_TEXTURES,
        TEX_PARAMETER_I,
        TEX_IMAGE2D,
        GENERATE_MIPMAP,
        READ_PIXELS,
        PIXEL_STORE_I,

        CREATE_PROGRAM,
        CREATE_SHADER,
        SHADER_SOURCE,
        COMPILE_SHADER,
        ATTACH_SHADER,
        LINK_PROGRAM,
        DETACH_SHADER,
        GET_SHADER_IV,
        GET_SHADER_INFO_LOG,
        GET_PROGRAM_IV,
        GET_ATTRIB_LOCATION,
        GET_ACTIVE_UNIFORM,
        GET_UNIFORM_LOCATION,

        SET_UNIFORM_1I,

        GET_QUERYOBJECT_UIV,
        DELETE_QUERIES,

        GET_QUERY_RESULT_NO_WAIT,

        SYNC_CPU_GPU,

        VALIDATE_PROGRAM,
        GET_CURRENT_PROGRAM_PTR,
        SET_CURRENT_PROGRAM_PTR,
    };

    Func func;
    uint64 arg[12];
    int retval;
    int status;
};

void ExecGL(GLCommand* cmd, uint32 cmdCount, bool forceExecute = false);

//==============================================================================
}
#endif // __RHI_GLES2_H__
