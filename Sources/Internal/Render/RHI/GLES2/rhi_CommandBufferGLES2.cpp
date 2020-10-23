#include "../Common/rhi_Pool.h"
#include "rhi_GLES2.h"
#include "rhi_ProgGLES2.h"

#include "../Common/rhi_Private.h"
#include "../Common/rhi_RingBuffer.h"
#include "../Common/dbg_StatSet.h"
#include "../Common/rhi_CommonImpl.h"
#include "../Common/SoftwareCommandBuffer.h"
#include "../Common/RenderLoop.h"

#include "../rhi_Public.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"

using DAVA::Logger;

#include "Concurrency/Thread.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/ConditionVariable.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/AutoResetEvent.h"
#include "Concurrency/ManualResetEvent.h"
#include "Time/SystemTimer.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"

#include "_gl.h"

#if defined(DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME)
    #define ACQUIRE_CONTEXT() _GLES2_AcquireContext()
    #define RELEASE_CONTEXT() _GLES2_ReleaseContext()
#else
    #define ACQUIRE_CONTEXT()
    #define RELEASE_CONTEXT()
#endif

namespace rhi
{
struct RenderPassGLES2_t
{
    std::vector<Handle> cmdBuf;
    int priority;
    Handle perfQueryStart;
    Handle perfQueryEnd;
    bool skipPerfQueries;
};

struct CommandBufferGLES2_t : public SoftwareCommandBuffer
{
public:
    CommandBufferGLES2_t();
    ~CommandBufferGLES2_t();
    void Execute();

    RenderPassConfig passCfg; //-V730_NOINIT
    uint32 isFirstInPass : 1;
    uint32 isLastInPass : 1;
    uint32 usingDefaultFrameBuffer : 1;
    uint32 skipPassPerfQueries : 1;
    Handle sync;
};

struct SyncObjectGLES2_t
{
    uint32 frame;
    uint32 is_signaled : 1;
    uint32 is_used : 1;
};

typedef ResourcePool<CommandBufferGLES2_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false> CommandBufferPoolGLES2;
typedef ResourcePool<RenderPassGLES2_t, RESOURCE_RENDER_PASS, RenderPassConfig, false> RenderPassPoolGLES2;
typedef ResourcePool<SyncObjectGLES2_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false> SyncObjectPoolGLES2;

RHI_IMPL_POOL(CommandBufferGLES2_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false);
RHI_IMPL_POOL(RenderPassGLES2_t, RESOURCE_RENDER_PASS, RenderPassConfig, false);
RHI_IMPL_POOL(SyncObjectGLES2_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false);

static DAVA::Mutex _GLES2_SyncObjectsSync;

static Handle gles2_RenderPass_Allocate(const RenderPassConfig& passConf, uint32 cmdBufCount, Handle* cmdBuf)
{
    DVASSERT(cmdBufCount);
    DVASSERT(passConf.IsValid());

    Handle handle = RenderPassPoolGLES2::Alloc();
    RenderPassGLES2_t* pass = RenderPassPoolGLES2::Get(handle);

    pass->cmdBuf.resize(cmdBufCount);
    pass->priority = passConf.priority;
    pass->perfQueryStart = passConf.perfQueryStart;
    pass->perfQueryEnd = passConf.perfQueryEnd;
    pass->skipPerfQueries = false;

    for (uint32 i = 0; i != cmdBufCount; ++i)
    {
        Handle h = CommandBufferPoolGLES2::Alloc();
        CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(h);

        cb->passCfg = passConf;
        cb->isFirstInPass = i == 0;
        cb->isLastInPass = i == cmdBufCount - 1;
        cb->usingDefaultFrameBuffer = passConf.colorBuffer[0].texture == InvalidHandle;
        cb->skipPassPerfQueries = false;

        pass->cmdBuf[i] = h;
        cmdBuf[i] = h;
    }

    return handle;
}

static void gles2_RenderPass_Begin(Handle pass)
{
}

static void gles2_RenderPass_End(Handle pass)
{
}

namespace RenderPassGLES2
{
void Init(uint32 maxCount)
{
    RenderPassPoolGLES2::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Renderpass_Allocate = &gles2_RenderPass_Allocate;
    dispatch->impl_Renderpass_Begin = &gles2_RenderPass_Begin;
    dispatch->impl_Renderpass_End = &gles2_RenderPass_End;
}
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_Begin(Handle cmdBuf)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    cb->curUsedSize = 0;
    cb->allocCmd<SWCommand_Begin>();
}
//------------------------------------------------------------------------------

static void gles2_CommandBuffer_End(Handle cmdBuf, Handle syncObject)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_End* cmd = cb->allocCmd<SWCommand_End>();
    cmd->syncObject = syncObject;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetPipelineState(Handle cmdBuf, Handle ps, uint32 vdecl)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetPipelineState* cmd = cb->allocCmd<SWCommand_SetPipelineState>();
    cmd->vdecl = uint16(vdecl);
    cmd->ps = ps;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetCullMode(Handle cmdBuf, CullMode mode)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetCullMode* cmd = cb->allocCmd<SWCommand_SetCullMode>();
    cmd->mode = mode;
}

//------------------------------------------------------------------------------

void gles2_CommandBuffer_SetScissorRect(Handle cmdBuf, ScissorRect rect)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetScissorRect* cmd = cb->allocCmd<SWCommand_SetScissorRect>();
    cmd->x = rect.x;
    cmd->y = rect.y;
    cmd->width = rect.width;
    cmd->height = rect.height;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetViewport(Handle cmdBuf, Viewport vp)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetViewport* cmd = cb->allocCmd<SWCommand_SetViewport>();
    cmd->x = uint16(vp.x);
    cmd->y = uint16(vp.y);
    cmd->width = uint16(vp.width);
    cmd->height = uint16(vp.height);
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetFillMode(Handle cmdBuf, FillMode mode)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetFillMode* cmd = cb->allocCmd<SWCommand_SetFillMode>();
    cmd->mode = mode;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetVertexData* cmd = cb->allocCmd<SWCommand_SetVertexData>();
    cmd->vb = vb;
    cmd->streamIndex = uint16(streamIndex);
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);

    if (buffer != InvalidHandle)
    {
        CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
        SWCommand_SetVertexProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetVertexProgConstBuffer>();
        cmd->buffer = buffer;
        cmd->bufIndex = bufIndex;
        cmd->inst = ConstBufferGLES2::Instance(buffer);
    }
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    if (tex != InvalidHandle)
    {
        CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
        SWCommand_SetVertexTexture* cmd = cb->allocCmd<SWCommand_SetVertexTexture>();
        cmd->unitIndex = unitIndex;
        cmd->tex = tex;
    }
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetIndices(Handle cmdBuf, Handle ib)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetIndices* cmd = cb->allocCmd<SWCommand_SetIndices>();
    cmd->ib = ib;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetQueryIndex(Handle cmdBuf, uint32 objectIndex)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetQueryIndex* cmd = cb->allocCmd<SWCommand_SetQueryIndex>();
    cmd->objectIndex = objectIndex;
}

static void gles2_CommandBuffer_IssueTimestampQuery(Handle cmdBuf, Handle query)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    cb->skipPassPerfQueries = !_GLES2_TimeStampQuerySupported;

    SWCommand_IssueTimestamptQuery* cmd = cb->allocCmd<SWCommand_IssueTimestamptQuery>();
    cmd->perfQuery = query;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetQueryBuffer(Handle cmdBuf, Handle queryBuf)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetQueryBuffer* cmd = cb->allocCmd<SWCommand_SetQueryBuffer>();
    cmd->queryBuf = queryBuf;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    //    L_ASSERT(buffer);
    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);

    if (buffer != InvalidHandle)
    {
        CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
        SWCommand_SetFragmentProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetFragmentProgConstBuffer>();
        cmd->bufIndex = bufIndex;
        cmd->buffer = buffer;
        cmd->inst = ConstBufferGLES2::Instance(buffer);
    }
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    if (tex != InvalidHandle)
    {
        CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
        SWCommand_SetFragmentTexture* cmd = cb->allocCmd<SWCommand_SetFragmentTexture>();
        cmd->unitIndex = unitIndex;
        cmd->tex = tex;
    }
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetDepthStencilState* cmd = cb->allocCmd<SWCommand_SetDepthStencilState>();
    cmd->depthStencilState = depthStencilState;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    // NOTE: expected to be called BEFORE SetFragmentTexture
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_SetSamplerState* cmd = cb->allocCmd<SWCommand_SetSamplerState>();
    cmd->samplerState = samplerState;
}

//------------------------------------------------------------------------------

static int32 _GLES2_GetDrawMode(PrimitiveType primType, uint32 primCount, uint32* v_cnt)
{
    int32 mode = GL_TRIANGLES;

    switch (primType)
    {
    case PRIMITIVE_TRIANGLELIST:
        *v_cnt = primCount * 3;
        mode = GL_TRIANGLES;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        *v_cnt = 2 + primCount;
        mode = GL_TRIANGLE_STRIP;
        break;

    case PRIMITIVE_LINELIST:
        *v_cnt = primCount * 2;
        mode = GL_LINES;
        break;
    }

    return mode;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    uint32 v_cnt = 0;
    int32 mode = _GLES2_GetDrawMode(type, count, &v_cnt);
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_DrawPrimitive* cmd = cb->allocCmd<SWCommand_DrawPrimitive>();
    cmd->mode = mode;
    cmd->vertexCount = v_cnt;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count, uint32 /*vertexCount*/, uint32 firstVertex, uint32 startIndex)
{
    uint32 i_cnt = 0;
    int32 mode = _GLES2_GetDrawMode(type, count, &i_cnt);

    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_DrawIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawIndexedPrimitive>();
    cmd->mode = mode;
    cmd->indexCount = i_cnt;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count)
{
    uint32 v_cnt = 0;
    int32 mode = _GLES2_GetDrawMode(type, count, &v_cnt);

    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_DrawInstancedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedPrimitive>();
    cmd->mode = mode;
    cmd->vertexCount = v_cnt;
    cmd->instanceCount = instCount;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count, uint32 /*vertexCount*/, uint32 firstVertex, uint32 startIndex, uint32 baseInstance)
{
    uint32 v_cnt = 0;
    int32 mode = _GLES2_GetDrawMode(type, count, &v_cnt);

    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);
    SWCommand_DrawInstancedIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedIndexedPrimitive>();
    cmd->mode = mode;
    cmd->indexCount = v_cnt;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;
    cmd->instanceCount = instCount;
    cmd->baseInstance = baseInstance;
}

//------------------------------------------------------------------------------

static void gles2_CommandBuffer_SetMarker(Handle cmdBuf, const char* text)
{
#ifdef __DAVAENGINE_DEBUG__
    CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cmdBuf);

    if (!cb->text)
    {
        cb->text = new RingBuffer();
        cb->text->Initialize(64 * 1024);
    }

    int len = static_cast<int>(strlen(text));
    char* txt = reinterpret_cast<char*>(cb->text->Alloc(len / sizeof(float) + 2));

    memcpy(txt, text, len);
    txt[len] = '\n';
    txt[len + 1] = '\0';

    SWCommand_SetMarker* cmd = cb->allocCmd<SWCommand_SetMarker>();
    cmd->text = text;

#endif
}

//------------------------------------------------------------------------------

static Handle gles2_SyncObject_Create()
{
    _GLES2_SyncObjectsSync.Lock();

    Handle handle = SyncObjectPoolGLES2::Alloc();
    SyncObjectGLES2_t* sync = SyncObjectPoolGLES2::Get(handle);

    sync->is_signaled = false;
    sync->is_used = false;

    _GLES2_SyncObjectsSync.Unlock();

    return handle;
}

//------------------------------------------------------------------------------

static void gles2_SyncObject_Delete(Handle obj)
{
    _GLES2_SyncObjectsSync.Lock();

    SyncObjectPoolGLES2::Free(obj);

    _GLES2_SyncObjectsSync.Unlock();
}

//------------------------------------------------------------------------------

static bool gles2_SyncObject_IsSignaled(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_GLES2_SyncObjectsSync);

    bool signaled = false;

    if (SyncObjectPoolGLES2::IsAlive(obj))
    {
        SyncObjectGLES2_t* sync = SyncObjectPoolGLES2::Get(obj);

        if (sync)
            signaled = sync->is_signaled;
    }
    else
    {
        signaled = true;
    }

    return signaled;
}

CommandBufferGLES2_t::CommandBufferGLES2_t()
    : isFirstInPass(true)
    , isLastInPass(true)
    , usingDefaultFrameBuffer(true)
    , skipPassPerfQueries(false)
    , sync(InvalidHandle)
{
}

//------------------------------------------------------------------------------

CommandBufferGLES2_t::~CommandBufferGLES2_t()
{
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void CommandBufferGLES2_t::Execute()
{
    ACQUIRE_CONTEXT();

    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_CMD_BUFFER_EXECUTE);

    Handle cur_ps = InvalidHandle;
    uint32 cur_vdecl = VertexLayout::InvalidUID;
    uint32 cur_base_vert = 0;
    Handle last_ps = InvalidHandle;
    uint32 cur_gl_prog = 0;

    Handle vp_const[MAX_CONST_BUFFER_COUNT] = {};
    const void* vp_const_data[MAX_CONST_BUFFER_COUNT] = {};

    Handle fp_const[MAX_CONST_BUFFER_COUNT] = {};
    const void* fp_const_data[MAX_CONST_BUFFER_COUNT] = {};

    Handle cur_vb[MAX_VERTEX_STREAM_COUNT] = {};
    Handle cur_ib = InvalidHandle;

    bool vdecl_pending = true;
    IndexSize idx_size = INDEX_SIZE_16BIT;
    uint32 tex_unit_0 = 0;
    Handle cur_query_buf = InvalidHandle;
    GLint def_viewport[4] = {};

    int immediate_cmd_ttw = 10;

    sync = InvalidHandle;

    for (const uint8 *c = cmdData, *c_end = cmdData + curUsedSize; c != c_end;)
    {
        const SWCommand* cmd = reinterpret_cast<const SWCommand*>(c);

        switch (SoftwareCommandType(cmd->type))
        {
        case CMD_BEGIN:
        {
            GL_CALL(glFrontFace(GL_CW));
            GL_CALL(glEnable(GL_CULL_FACE));
            GL_CALL(glCullFace(GL_BACK));

            GL_CALL(glEnable(GL_DEPTH_TEST));
            GL_CALL(glDepthFunc(GL_LEQUAL));
            GL_CALL(glDepthMask(GL_TRUE));
            GL_CALL(glDisable(GL_SCISSOR_TEST));

            if (isFirstInPass)
            {
#if defined(__DAVAENGINE_IPHONE__)
                ios_gl_begin_frame();
#endif
                Handle rt[MAX_RENDER_TARGET_COUNT] = {};
                TextureFace rt_face[MAX_RENDER_TARGET_COUNT] = {};
                uint32 rt_level[MAX_RENDER_TARGET_COUNT] = {};
                uint32 rt_count = 0;
                bool apply_fb = true;
                bool do_clear = true;

                def_viewport[0] = 0;
                def_viewport[1] = 0;

                for (uint32 i = 0; i != countof(passCfg.colorBuffer); ++i)
                {
                    if (passCfg.colorBuffer[i].texture != InvalidHandle)
                    {
                        rt[i] = (passCfg.UsingMSAA()) ? passCfg.colorBuffer[i].multisampleTexture : passCfg.colorBuffer[i].texture;
                        rt_face[i] = passCfg.colorBuffer[i].textureFace;
                        rt_level[i] = passCfg.colorBuffer[i].textureLevel;
                        ++rt_count;

                        if (i == 0)
                        {
                            Size2i sz = TextureGLES2::Size(passCfg.colorBuffer[i].texture);
                            def_viewport[2] = sz.dx;
                            def_viewport[3] = sz.dy;
                        }
                    }
                    else
                    {
                        if (i == 0)
                        {
                            if (passCfg.UsingMSAA())
                            {
                                DVASSERT(passCfg.colorBuffer[i].multisampleTexture != InvalidHandle);
                                rt[0] = passCfg.colorBuffer[i].multisampleTexture;
                                rt_count = 1;
                                apply_fb = true;
                            }
                            else
                            {
                                GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, _GLES2_Default_FrameBuffer));
                                _GLES2_Bound_FrameBuffer = _GLES2_Default_FrameBuffer;
                                def_viewport[2] = _GLES2_DefaultFrameBuffer_Width;
                                def_viewport[3] = _GLES2_DefaultFrameBuffer_Height;
                                rt_count = 1;
                                apply_fb = false;
                                do_clear = true;
                            }
                        }
                        break;
                    }
                }

                GL_CALL(glViewport(def_viewport[0], def_viewport[1], def_viewport[2], def_viewport[3]));

                if (apply_fb)
                {
                    Handle ds = (passCfg.UsingMSAA()) ? passCfg.depthStencilBuffer.multisampleTexture : passCfg.depthStencilBuffer.texture;
                    GLuint fbo = TextureGLES2::GetFrameBuffer(rt, rt_face, rt_level, rt_count, ds);
                    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
                    _GLES2_Bound_FrameBuffer = fbo;

                    #if !defined(__DAVAENGINE_ANDROID__)
                    for (uint32 i = 0; i != rt_count; ++i)
                    {
                        if (passCfg.colorBuffer[i].loadAction == LOADACTION_CLEAR)
                            glClearBufferfv(GL_COLOR, i, passCfg.colorBuffer[i].clearColor);
                    }

                    if (passCfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR)
                    {
                        glClearBufferfi(GL_DEPTH_STENCIL, 0, passCfg.depthStencilBuffer.clearDepth, 0);
                    }
                    do_clear = false;
                    #endif

                    #if defined(__DAVAENGINE_MACOS__)
                    /*                    
                    if (passCfg.colorBuffer[0].loadAction == LOADACTION_CLEAR)
                    {
                        // since glClearBuffer doesn't work on MacOS, clear buffers with the same color at least
                        GL_CALL(glClearColor(passCfg.colorBuffer[0].clearColor[0], passCfg.colorBuffer[0].clearColor[1], passCfg.colorBuffer[0].clearColor[2], passCfg.colorBuffer[0].clearColor[3]));
                        GL_CALL(glClearDepthf(passCfg.depthStencilBuffer.clearDepth));
                        GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
                    }
                    do_clear = false;
*/
                    do_clear = true;
                    #endif
                }


                #if defined(__DAVAENGINE_IPHONE__)
                if (rt_count == 1)
                    do_clear = true;
                #endif

                if (do_clear)
                {
                    GLuint flags = 0;

                    if (passCfg.colorBuffer[0].loadAction == LOADACTION_CLEAR)
                    {
                        GL_CALL(glClearColor(passCfg.colorBuffer[0].clearColor[0], passCfg.colorBuffer[0].clearColor[1], passCfg.colorBuffer[0].clearColor[2], passCfg.colorBuffer[0].clearColor[3]));
                        flags |= GL_COLOR_BUFFER_BIT;
                    }

                    if (passCfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR)
                    {
                        #if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
                        GL_CALL(glStencilMask(0xFFFFFFFF));
                        GL_CALL(glClearDepthf(passCfg.depthStencilBuffer.clearDepth));
                        #else
                        GL_CALL(glClearDepth(passCfg.depthStencilBuffer.clearDepth));
                        GL_CALL(glStencilMask(0xFFFFFFFF));
                        GL_CALL(glClearStencil(0));
                        #endif

                        flags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
                    }

                    if (flags)
                        GL_CALL(glClear(flags));
                }

                DVASSERT(cur_query_buf == InvalidHandle || !QueryBufferGLES2::QueryIsCompleted(cur_query_buf));
            }
        }
        break;

        case CMD_END:
        {
            sync = (static_cast<const SWCommand_End*>(cmd))->syncObject;
            if (isLastInPass)
            {
                if (cur_query_buf != InvalidHandle)
                {
                    QueryBufferGLES2::QueryComplete(cur_query_buf);
                }

                if (passCfg.colorBuffer[0].storeAction == rhi::STOREACTION_RESOLVE)
                {
                    TextureGLES2::ResolveMultisampling(passCfg.colorBuffer[0].multisampleTexture, passCfg.colorBuffer[0].texture);
                }

                if (passCfg.colorBuffer[1].storeAction == rhi::STOREACTION_RESOLVE)
                {
                    TextureGLES2::ResolveMultisampling(passCfg.colorBuffer[1].multisampleTexture, passCfg.colorBuffer[1].texture);
                }

            #if defined(__DAVAENGINE_IPHONE__)
                if (_GLES2_Bound_FrameBuffer != _GLES2_Default_FrameBuffer) // defualt framebuffer is discard once after frame
                {
                    bool discardColor = (passCfg.colorBuffer[0].storeAction != STOREACTION_STORE);
                    bool discardDepthStencil = (passCfg.depthStencilBuffer.storeAction != STOREACTION_STORE);
                    ios_gl_discard_framebuffer(discardColor, discardDepthStencil);
                }
            #endif
            }
        }
        break;

        case CMD_SET_VERTEX_DATA:
        {
            Handle vb = (static_cast<const SWCommand_SetVertexData*>(cmd))->vb;
            uint32 stream_i = (static_cast<const SWCommand_SetVertexData*>(cmd))->streamIndex;
            if (cur_vb[stream_i] != vb)
            {
                if (stream_i == 0)
                    VertexBufferGLES2::SetToRHI(vb);

                PipelineStateGLES2::InvalidateVattrCache();
                vdecl_pending = true;
                cur_base_vert = 0;

                StatSet::IncStat(stat_SET_VB, 1);

                cur_vb[stream_i] = vb;
            }
        }
        break;

        case CMD_SET_INDICES:
        {
            Handle ib = (static_cast<const SWCommand_SetIndices*>(cmd))->ib;
            if (ib != cur_ib)
            {
                idx_size = IndexBufferGLES2::SetToRHI(ib);
                StatSet::IncStat(stat_SET_IB, 1);
                cur_ib = ib;
            }
        }
        break;

        case CMD_SET_QUERY_BUFFER:
        {
            DVASSERT(cur_query_buf == InvalidHandle);
            cur_query_buf = (static_cast<const SWCommand_SetQueryBuffer*>(cmd))->queryBuf;
        }
        break;

        case CMD_SET_QUERY_INDEX:
        {
            if (cur_query_buf != InvalidHandle)
                QueryBufferGLES2::SetQueryIndex(cur_query_buf, (static_cast<const SWCommand_SetQueryIndex*>(cmd))->objectIndex);
        }
        break;

        case CMD_ISSUE_TIMESTAMP_QUERY:
        {
            Handle query = (static_cast<const SWCommand_IssueTimestamptQuery*>(cmd))->perfQuery;
            PerfQueryGLES2::IssueQuery(query);
        }
        break;

        case CMD_SET_PIPELINE_STATE:
        {
            Handle ps = (static_cast<const SWCommand_SetPipelineState*>(cmd))->ps;
            uint32 vdecl = (static_cast<const SWCommand_SetPipelineState*>(cmd))->vdecl;
            if (cur_ps != ps || cur_vdecl != vdecl)
            {
                for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
                {
                    vp_const[i] = InvalidHandle;
                    fp_const[i] = InvalidHandle;
                }
                memset(vp_const_data, 0, sizeof(vp_const_data));
                memset(fp_const_data, 0, sizeof(fp_const_data));

                cur_ps = ps;
                cur_vdecl = vdecl;
                cur_base_vert = 0;
                last_ps = InvalidHandle;
                cur_gl_prog = PipelineStateGLES2::ProgramUid(ps);
                vdecl_pending = true;
            }

            tex_unit_0 = PipelineStateGLES2::VertexSamplerCount(ps);
        }
        break;

        case CMD_SET_CULL_MODE:
        {
            CullMode mode = CullMode((static_cast<const SWCommand_SetCullMode*>(cmd))->mode);

            switch (mode)
            {
            case CULL_NONE:
                GL_CALL(glDisable(GL_CULL_FACE));
                break;

            case CULL_CCW:
                GL_CALL(glEnable(GL_CULL_FACE));
                GL_CALL(glFrontFace(GL_CW));
                GL_CALL(glCullFace(GL_BACK));
                break;

            case CULL_CW:
                GL_CALL(glEnable(GL_CULL_FACE));
                GL_CALL(glFrontFace(GL_CW));
                GL_CALL(glCullFace(GL_FRONT));
                break;
            }
        }
        break;

        case CMD_SET_SCISSOR_RECT:
        {
            GLint x = (static_cast<const SWCommand_SetScissorRect*>(cmd))->x;
            GLint y = (static_cast<const SWCommand_SetScissorRect*>(cmd))->y;
            GLsizei w = (static_cast<const SWCommand_SetScissorRect*>(cmd))->width;
            GLsizei h = (static_cast<const SWCommand_SetScissorRect*>(cmd))->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                if (usingDefaultFrameBuffer)
                    y = _GLES2_DefaultFrameBuffer_Height - y - h;

                GL_CALL(glEnable(GL_SCISSOR_TEST));
                GL_CALL(glScissor(x, y, w, h));
            }
            else
            {
                GL_CALL(glDisable(GL_SCISSOR_TEST));
            }
        }
        break;

        case CMD_SET_VIEWPORT:
        {
            GLint x = (static_cast<const SWCommand_SetViewport*>(cmd))->x;
            GLint y = (static_cast<const SWCommand_SetViewport*>(cmd))->y;
            GLsizei w = (static_cast<const SWCommand_SetViewport*>(cmd))->width;
            GLsizei h = (static_cast<const SWCommand_SetViewport*>(cmd))->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                if (usingDefaultFrameBuffer)
                    y = _GLES2_DefaultFrameBuffer_Height - y - h;

                GL_CALL(glViewport(x, y, w, h));
            }
            else
            {
                GL_CALL(glViewport(def_viewport[0], def_viewport[1], def_viewport[2], def_viewport[3]));
            }
        }
        break;

        case CMD_SET_FILLMODE:
        {
            FillMode mode = FillMode((static_cast<const SWCommand_SetFillMode*>(cmd))->mode);            
            #if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
            GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, (mode == FILLMODE_WIREFRAME) ? GL_LINE : GL_FILL));
            #endif
        }
        break;

        case CMD_SET_DEPTHSTENCIL_STATE:
        {
            Handle state = (static_cast<const SWCommand_SetDepthStencilState*>(cmd))->depthStencilState;
            DepthStencilStateGLES2::SetToRHI(state);
        }
        break;

        case CMD_SET_SAMPLER_STATE:
        {
            Handle state = (static_cast<const SWCommand_SetSamplerState*>(cmd))->samplerState;
            SamplerStateGLES2::SetToRHI(state);
            StatSet::IncStat(stat_SET_SS, 1);
        }
        break;

        case CMD_SET_VERTEX_PROG_CONST_BUFFER:
        {
            //++stcb_cnt;
            uint32 buf_i = (static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd))->bufIndex;
            const void* inst = (static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd))->inst;
            Handle buf = (static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd))->buffer;
            vp_const[buf_i] = buf;
            vp_const_data[buf_i] = inst;
        }
        break;

        case CMD_SET_FRAGMENT_PROG_CONST_BUFFER:
        {
            //++stcb_cnt;
            uint32 buf_i = (static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd))->bufIndex;
            const void* inst = (static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd))->inst;
            Handle buf = (static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd))->buffer;
            fp_const[buf_i] = buf;
            fp_const_data[buf_i] = inst;
        }
        break;

        case CMD_SET_FRAGMENT_TEXTURE:
        {
            //++sttx_cnt;
            Handle tex = (static_cast<const SWCommand_SetFragmentTexture*>(cmd))->tex;
            uint32 unit_i = (static_cast<const SWCommand_SetFragmentTexture*>(cmd))->unitIndex;
            TextureGLES2::SetToRHI(tex, unit_i, tex_unit_0);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_SET_VERTEX_TEXTURE:
        {
            //++sttx_cnt;
            Handle tex = (static_cast<const SWCommand_SetVertexTexture*>(cmd))->tex;
            uint32 unit_i = (static_cast<const SWCommand_SetVertexTexture*>(cmd))->unitIndex;

            TextureGLES2::SetToRHI(tex, unit_i, DAVA::InvalidIndex);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_DRAW_PRIMITIVE:
        {
            //++dip_cnt;
            //{SCOPED_NAMED_TIMING("gl.DP")}
            uint32 v_cnt = (static_cast<const SWCommand_DrawPrimitive*>(cmd))->vertexCount;
            int mode = (static_cast<const SWCommand_DrawPrimitive*>(cmd))->mode;
            if (last_ps != cur_ps)
            {
                PipelineStateGLES2::SetToRHI(cur_ps);
                StatSet::IncStat(stat_SET_PS, 1);
                last_ps = cur_ps;
            }

            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (vp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(vp_const[i], cur_gl_prog, vp_const_data[i]);
            }
            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (fp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(fp_const[i], cur_gl_prog, fp_const_data[i]);
            }

            if (vdecl_pending)
            {
                PipelineStateGLES2::SetVertexDeclToRHI(cur_ps, cur_vdecl, 0, countof(cur_vb), cur_vb);
                vdecl_pending = false;
            }

            GL_CALL(glDrawArrays(mode, 0, v_cnt));
            StatSet::IncStat(stat_DP, 1);
            switch (mode)
            {
            case GL_TRIANGLES:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case GL_TRIANGLE_STRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case GL_LINES:
                StatSet::IncStat(stat_DLL, 1);
                break;
            }
        }
        break;

        case CMD_DRAW_INDEXED_PRIMITIVE:
        {
            //++dip_cnt;
            //{SCOPED_NAMED_TIMING("gl.DIP")}
            uint32 v_cnt = (static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd))->indexCount;
            int mode = (static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd))->mode;
            uint32 firstVertex = (static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd))->firstVertex;
            uint32 startIndex = (static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd))->startIndex;

            if (last_ps != cur_ps)
            {
                PipelineStateGLES2::SetToRHI(cur_ps);
                StatSet::IncStat(stat_SET_PS, 1);
                last_ps = cur_ps;
            }

            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (vp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(vp_const[i], cur_gl_prog, vp_const_data[i]);
            }
            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (fp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(fp_const[i], cur_gl_prog, fp_const_data[i]);
            }

            if (vdecl_pending || firstVertex != cur_base_vert)
            {
                PipelineStateGLES2::SetVertexDeclToRHI(cur_ps, cur_vdecl, firstVertex, countof(cur_vb), cur_vb);
                vdecl_pending = false;
                cur_base_vert = firstVertex;
            }

            int i_sz = GL_UNSIGNED_SHORT;
            int i_off = startIndex * sizeof(uint16);

            if (idx_size == INDEX_SIZE_32BIT)
            {
                i_sz = GL_UNSIGNED_INT;
                i_off = startIndex * sizeof(uint32);
            }

            GL_CALL(glDrawElements(mode, v_cnt, i_sz, _GLES2_LastSetIndices + i_off));
            StatSet::IncStat(stat_DIP, 1);
            switch (mode)
            {
            case GL_TRIANGLES:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case GL_TRIANGLE_STRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case GL_LINES:
                StatSet::IncStat(stat_DLL, 1);
                break;
            }
        }
        break;

        case CMD_DRAW_INSTANCED_PRIMITIVE:
        {
            uint32 v_cnt = (static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->vertexCount;
            int mode = (static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->mode;
            uint32 instCount = (static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->instanceCount;

            if (last_ps != cur_ps)
            {
                PipelineStateGLES2::SetToRHI(cur_ps);
                StatSet::IncStat(stat_SET_PS, 1);
                last_ps = cur_ps;
            }

            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (vp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(vp_const[i], cur_gl_prog, vp_const_data[i]);
            }
            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (fp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(fp_const[i], cur_gl_prog, fp_const_data[i]);
            }

            if (vdecl_pending)
            {
                PipelineStateGLES2::SetVertexDeclToRHI(cur_ps, cur_vdecl, 0, countof(cur_vb), cur_vb);
                vdecl_pending = false;
            }

            #if defined(__DAVAENGINE_IPHONE__)
            GL_CALL(glDrawArraysInstancedEXT(mode, 0, v_cnt, instCount));
            #elif defined(__DAVAENGINE_ANDROID__)
            if (glDrawArraysInstanced)
            {
                GL_CALL(glDrawArraysInstanced(mode, 0, v_cnt, instCount));
            }
            #elif defined(__DAVAENGINE_MACOS__)
            GL_CALL(glDrawArraysInstancedARB(mode, 0, v_cnt, instCount));
            #else
            GL_CALL(glDrawArraysInstanced(mode, 0, v_cnt, instCount));
            #endif
            StatSet::IncStat(stat_DP, 1);
            switch (mode)
            {
            case GL_TRIANGLES:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case GL_TRIANGLE_STRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case GL_LINES:
                StatSet::IncStat(stat_DLL, 1);
                break;
            }
        }
        break;

        case CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE:
        {
            uint32 v_cnt = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->indexCount;
            int mode = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->mode;
            uint32 instCount = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->instanceCount;
            uint32 firstVertex = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->firstVertex;
            uint32 startIndex = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->startIndex;
            uint32 baseInst = (static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd))->baseInstance;
            //{SCOPED_NAMED_TIMING("gl.DIP")}

            if (last_ps != cur_ps)
            {
                PipelineStateGLES2::SetToRHI(cur_ps);
                StatSet::IncStat(stat_SET_PS, 1);
                last_ps = cur_ps;
            }

            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (vp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(vp_const[i], cur_gl_prog, vp_const_data[i]);
            }
            for (uint32 i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (fp_const[i] != InvalidHandle)
                    ConstBufferGLES2::SetToRHI(fp_const[i], cur_gl_prog, fp_const_data[i]);
            }

            if (vdecl_pending || firstVertex != cur_base_vert)
            {
                PipelineStateGLES2::SetVertexDeclToRHI(cur_ps, cur_vdecl, firstVertex, countof(cur_vb), cur_vb);
                vdecl_pending = false;
                cur_base_vert = firstVertex;
            }

            int i_sz = GL_UNSIGNED_SHORT;
            int i_off = startIndex * sizeof(uint16);

            if (idx_size == INDEX_SIZE_32BIT)
            {
                i_sz = GL_UNSIGNED_INT;
                i_off = startIndex * sizeof(uint32);
            }

            #if defined(__DAVAENGINE_IPHONE__)
            DVASSERT(baseInst == 0); // it's not supported in GLES
            GL_CALL(glDrawElementsInstancedEXT(mode, v_cnt, i_sz, reinterpret_cast<void*>(static_cast<uint64>(i_off)), instCount));
            #elif defined(__DAVAENGINE_ANDROID__)
            DVASSERT(baseInst == 0); // it's not supported in GLES
            if (glDrawElementsInstanced)
            {
                GL_CALL(glDrawElementsInstanced(mode, v_cnt, i_sz, reinterpret_cast<void*>(static_cast<uint64>(i_off)), instCount));
            }
            #elif defined(__DAVAENGINE_MACOS__)
            GL_CALL(glDrawElementsInstancedBaseVertex(mode, v_cnt, i_sz, reinterpret_cast<void*>(static_cast<uint64>(i_off)), instCount, baseInst));
            #else
            GL_CALL(glDrawElementsInstancedBaseInstance(mode, v_cnt, i_sz, reinterpret_cast<void*>(static_cast<uint64>(i_off)), instCount, baseInst));
            #endif
            StatSet::IncStat(stat_DIP, 1);
            switch (mode)
            {
            case GL_TRIANGLES:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case GL_TRIANGLE_STRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case GL_LINES:
                StatSet::IncStat(stat_DLL, 1);
                break;
            }
        }
        break;

        case CMD_SET_MARKER:
        {
            const char* text = (static_cast<const SWCommand_SetMarker*>(cmd))->text;
            Trace(text);
        }
        break;

        default:
            Logger::Error("unsupported command: %d", cmd->type);
            DVASSERT(false, "unsupported command");
        }

        if (--immediate_cmd_ttw <= 0)
        {
            RenderLoop::CheckImmediateCommand();
            immediate_cmd_ttw = 10;
        }

        if (cmd->type == CMD_END)
            break;

        c += cmd->size;
    }

    RELEASE_CONTEXT();
    //Logger::Info("exec cb  = %.2f Kb  in %u cmds (DIP=%u  STCB=%u  STTX=%u)",float(curUsedSize)/1024.0f,cmd_cnt,dip_cnt,stcb_cnt,sttx_cnt);
}

//------------------------------------------------------------------------------

static void _GLES2_RejectFrame(const CommonImpl::Frame& frame)
{
    if (frame.sync != InvalidHandle)
    {
        SyncObjectGLES2_t* s = SyncObjectPoolGLES2::Get(frame.sync);
        s->is_signaled = true;
        s->is_used = true;
    }
    for (Handle p : frame.pass)
    {
        RenderPassGLES2_t* pp = RenderPassPoolGLES2::Get(p);

        for (std::vector<Handle>::iterator c = pp->cmdBuf.begin(), c_end = pp->cmdBuf.end(); c != c_end; ++c)
        {
            CommandBufferGLES2_t* cc = CommandBufferPoolGLES2::Get(*c);
            if (cc->sync != InvalidHandle)
            {
                SyncObjectGLES2_t* s = SyncObjectPoolGLES2::Get(cc->sync);
                s->is_signaled = true;
                s->is_used = true;
            }
            cc->curUsedSize = 0;
            CommandBufferPoolGLES2::Free(*c);
        }
        RenderPassPoolGLES2::Free(p);
    }
}

//------------------------------------------------------------------------------

static void _GLES2_ExecuteQueuedCommands(const CommonImpl::Frame& frame)
{
    StatSet::ResetAll();

    std::vector<RenderPassGLES2_t*> pass;
    uint32 frame_n = 0;
    Handle framePerfQueryStart = InvalidHandle;
    Handle framePerfQueryEnd = InvalidHandle;
    bool skipFramePerfQueries = false;

    for (Handle p : frame.pass)
    {
        RenderPassGLES2_t* pp = RenderPassPoolGLES2::Get(p);
        bool do_add = true;

        for (uint32 i = 0; i != pass.size(); ++i)
        {
            if (pp->priority > pass[i]->priority)
            {
                pass.insert(pass.begin() + i, 1, pp);
                do_add = false;
                break;
            }
        }
        if (do_add)
            pass.push_back(pp);

        if (DeviceCaps().isPerfQuerySupported && !_GLES2_TimeStampQuerySupported)
        {
            for (uint32 b = 0; b != pp->cmdBuf.size(); ++b)
            {
                pp->skipPerfQueries |= CommandBufferPoolGLES2::Get(pp->cmdBuf[b])->skipPassPerfQueries;
            }

            skipFramePerfQueries |= (pp->perfQueryStart != InvalidHandle) || pp->skipPerfQueries;
            skipFramePerfQueries |= (pp->perfQueryEnd != InvalidHandle) || pp->skipPerfQueries;
        }
    }

    frame_n = frame.frameNumber;
    framePerfQueryStart = frame.perfQueryStart;
    framePerfQueryEnd = frame.perfQueryEnd;

    if (frame.sync != InvalidHandle)
    {
        SyncObjectGLES2_t* sync = SyncObjectPoolGLES2::Get(frame.sync);
        sync->frame = frame_n;
        sync->is_signaled = false;
        sync->is_used = true;
    }

    if (skipFramePerfQueries)
    {
        if (framePerfQueryStart != InvalidHandle)
            PerfQueryGLES2::SkipQuery(framePerfQueryStart);
        if (framePerfQueryEnd != InvalidHandle)
            PerfQueryGLES2::SkipQuery(framePerfQueryEnd);

        framePerfQueryStart = InvalidHandle;
        framePerfQueryEnd = InvalidHandle;
    }

    PerfQueryGLES2::ObtainPerfQueryResults();

    if (framePerfQueryStart != InvalidHandle)
    {
        PerfQueryGLES2::IssueQuery(framePerfQueryStart);
    }

    Trace("\n\n-------------------------------\nexecuting frame %u\n", frame_n);
    for (std::vector<RenderPassGLES2_t *>::iterator p = pass.begin(), p_end = pass.end(); p != p_end; ++p)
    {
        RenderPassGLES2_t* pp = *p;

        for (uint32 b = 0; b != pp->cmdBuf.size(); ++b)
        {
            Handle cb_h = pp->cmdBuf[b];
            CommandBufferGLES2_t* cb = CommandBufferPoolGLES2::Get(cb_h);

            if (pp->perfQueryStart != InvalidHandle)
            {
                if (pp->skipPerfQueries)
                    PerfQueryGLES2::SkipQuery(pp->perfQueryStart);
                else
                    PerfQueryGLES2::IssueQuery(pp->perfQueryStart);
            }

            cb->Execute();

            if (cb->sync != InvalidHandle)
            {
                SyncObjectGLES2_t* sync = SyncObjectPoolGLES2::Get(cb->sync);

                sync->frame = frame_n;
                sync->is_signaled = false;
                sync->is_used = true;
            }

            CommandBufferPoolGLES2::Free(cb_h);

            if (pp->perfQueryEnd != InvalidHandle)
            {
                if (pp->skipPerfQueries)
                    PerfQueryGLES2::SkipQuery(pp->perfQueryEnd);
                else
                    PerfQueryGLES2::IssueQuery(pp->perfQueryEnd);
            }
        }
    }
    Trace("\n\n-------------------------------\nframe %u executed(submitted to GPU)\n", frame_n);

    if (framePerfQueryEnd != InvalidHandle)
    {
        PerfQueryGLES2::IssueQuery(framePerfQueryEnd);
    }

    for (Handle p : frame.pass)
        RenderPassPoolGLES2::Free(p);

    //update sync objects
    _GLES2_SyncObjectsSync.Lock();
    for (SyncObjectPoolGLES2::Iterator s = SyncObjectPoolGLES2::Begin(), s_end = SyncObjectPoolGLES2::End(); s != s_end; ++s)
    {
        if (s->is_used && (frame_n - s->frame >= 2))
            s->is_signaled = true;
    }
    _GLES2_SyncObjectsSync.Unlock();
}

bool _GLES2_PresentBuffer()
{
    bool success = true;
    if (!_GLES2_Context) //this is special case when rendering is done inside other app render loop (eg: QT loop in ResEditor)
        return true;

// do swap-buffers
#if defined(__DAVAENGINE_WIN32__)
    win32_gl_end_frame();
#elif defined(__DAVAENGINE_MACOS__)
    macos_gl_end_frame();
#elif defined(__DAVAENGINE_IPHONE__)
    ios_gl_end_frame();
#elif defined(__DAVAENGINE_ANDROID__)
    success = android_gl_end_frame();        
    #endif

    return success;
}

void _GLES2_ResetBlock()
{
#if defined(__DAVAENGINE_ANDROID__)

    TextureGLES2::ReCreateAll();
    VertexBufferGLES2::ReCreateAll();
    IndexBufferGLES2::ReCreateAll();

    // update sync-objects, as pre-reset state is not actual anymore, also resolve constant reset causing already executed frame being never synced
    for (SyncObjectPoolGLES2::Iterator s = SyncObjectPoolGLES2::Begin(), s_end = SyncObjectPoolGLES2::End(); s != s_end; ++s)
    {
        if (s->is_used)
            s->is_signaled = true;
    }
#endif
}

void _GLES2_FinishFrame()
{
    ProgGLES2::InvalidateAllConstBufferInstances();
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static void _GLES2_ExecImmediateCommand(CommonImpl::ImmediateCommand* command)
{
    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_EXECUTE_IMMEDIATE_CMDS);

    int err = GL_NO_ERROR;

    ACQUIRE_CONTEXT();

#if 0

    do 
    {
        err = glGetError();
    } 
    while ( err != GL_NO_ERROR );

    #define EXEC_GL(expr) \
    expr; \
    err = glGetError(); \

#else

    #define EXEC_GL(expr) expr 

#endif

    GLCommand* commandData = reinterpret_cast<GLCommand*>(command->cmdData);
    for (GLCommand *cmd = commandData, *cmdEnd = commandData + command->cmdCount; cmd != cmdEnd; ++cmd)
    {
        const uint64* arg = cmd->arg;

        switch (cmd->func)
        {
        case GLCommand::NOP:
        {
            // do NOTHING
        }
        break;

        case GLCommand::GEN_BUFFERS:
        {
            GL_CALL(glGenBuffers(GLsizei(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            DVASSERT(*(reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::BIND_BUFFER:
        {
            GL_CALL(glBindBuffer(GLenum(arg[0]), *(reinterpret_cast<GLuint*>(arg[1]))));
            cmd->status = err;
        }
        break;

        case GLCommand::RESTORE_VERTEX_BUFFER:
        {
            GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, _GLES2_LastSetVB));
            cmd->status = err;
        }
        break;

        case GLCommand::RESTORE_INDEX_BUFFER:
        {
            GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _GLES2_LastSetIB));
            cmd->status = err;
        }
        break;

        case GLCommand::DELETE_BUFFERS:
        {
            GL_CALL(glDeleteBuffers(GLsizei(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::BUFFER_DATA:
        {
            GL_CALL(glBufferData(GLenum(arg[0]), GLsizei(arg[1]), reinterpret_cast<const void*>(arg[2]), GLenum(arg[3])));
            cmd->status = err;
        }
        break;

        case GLCommand::BUFFER_SUBDATA:
        {
            GL_CALL(glBufferSubData(GLenum(arg[0]), GLintptr(arg[1]), GLsizei(arg[2]), reinterpret_cast<const void*>(arg[3])));
            cmd->status = err;
        }
        break;

        case GLCommand::GEN_TEXTURES:
        {
            GL_CALL(glGenTextures(GLsizei(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            DVASSERT(*(reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::DELETE_TEXTURES:
        {
            GL_CALL(glDeleteTextures(GLsizei(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::SET_ACTIVE_TEXTURE:
        {
            int t = int(arg[0]);

            if (t != _GLES2_LastActiveTexture)
            {
                GL_CALL(glActiveTexture(GLenum(t)));
                _GLES2_LastActiveTexture = t;
                cmd->status = err;
            }
        }
        break;

        case GLCommand::BIND_TEXTURE:
        {
            GL_CALL(glBindTexture(GLenum(cmd->arg[0]), *(reinterpret_cast<GLuint*>(cmd->arg[1]))));
            cmd->status = err;
        }
        break;

        case GLCommand::RESTORE_TEXTURE0:
        {
            GL_CALL(glBindTexture(_GLES2_LastSetTex0Target, _GLES2_LastSetTex0));
            cmd->status = err;
        }
        break;

        case GLCommand::TEX_PARAMETER_I:
        {
            GL_CALL(glTexParameteri(GLenum(arg[0]), GLenum(arg[1]), GLuint(arg[2])));
            cmd->status = err;
        }
        break;

        case GLCommand::TEX_IMAGE2D:
        {
            if (arg[10])
            {
                GL_CALL(glCompressedTexImage2D(GLenum(arg[0]), GLint(arg[1]), GLenum(arg[2]), GLsizei(arg[3]), GLsizei(arg[4]), GLint(arg[5]), GLsizei(arg[8]), reinterpret_cast<const GLvoid*>(arg[9])));
            }
            else
            {
                GL_CALL(glTexImage2D(GLenum(arg[0]), GLint(arg[1]), GLint(arg[2]), GLsizei(arg[3]), GLsizei(arg[4]), GLint(arg[5]), GLenum(arg[6]), GLenum(arg[7]), reinterpret_cast<const GLvoid*>(arg[9])));
            }
            cmd->status = err;
        }
        break;

        case GLCommand::GENERATE_MIPMAP:
        {
            GL_CALL(glGenerateMipmap(GLenum(arg[0])));
            cmd->status = err;
        }
        break;

        case GLCommand::PIXEL_STORE_I:
        {
            GL_CALL(glPixelStorei(GLenum(arg[0]), GLint(arg[1])));
            cmd->retval = 0;
            cmd->status = err;
        }
        break;

        case GLCommand::READ_PIXELS:
        {
            GL_CALL(glReadPixels(GLint(arg[0]), GLint(arg[1]), GLsizei(arg[2]), GLsizei(arg[3]), GLenum(arg[4]), GLenum(arg[5]), reinterpret_cast<GLvoid*>(arg[6])));
            cmd->retval = 0;
            cmd->status = err;
        }
        break;

        case GLCommand::CREATE_PROGRAM:
        {
            GL_CALL(cmd->retval = glCreateProgram());
            cmd->status = 0;
        }
        break;

        case GLCommand::CREATE_SHADER:
        {
            GL_CALL(cmd->retval = glCreateShader(GLenum(arg[0])));
            cmd->status = 0;
        }
        break;

        case GLCommand::ATTACH_SHADER:
        {
            GL_CALL(glAttachShader(GLuint(arg[0]), GLuint(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::DETACH_SHADER:
        {
            GL_CALL(glDetachShader(GLuint(arg[0]), GLuint(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::LINK_PROGRAM:
        {
            GLint linkStatus = GL_FALSE;
            GLuint program = static_cast<GLuint>(arg[0]);
            GL_CALL(glLinkProgram(program));
            GL_CALL(glGetProgramiv(program, GL_LINK_STATUS, &linkStatus));
            if (linkStatus == GL_FALSE)
            {
                char info[2048] = {};
                GLsizei linkLogLength = 0;
                GL_CALL(glGetProgramInfoLog(program, sizeof(info), &linkLogLength, info));
                Logger::Error("Failed to link program:\n%s", info);
            }
            cmd->retval = linkStatus;
            cmd->status = err;
        }
        break;

        case GLCommand::VALIDATE_PROGRAM:
        {
            GLuint program = static_cast<GLuint>(arg[0]);

            GLint validateStatus = 0;
            GLchar validateLog[2048] = {};
            GLsizei validateLogLength = 0;
            GL_CALL(glUseProgram(program));
            GL_CALL(glValidateProgram(program));
            GL_CALL(glGetProgramiv(program, GL_VALIDATE_STATUS, &validateStatus));
            GL_CALL(glGetProgramInfoLog(program, 2048, &validateLogLength, validateLog));
        }
        break;

        case GLCommand::GET_CURRENT_PROGRAM_PTR:
        {
            GLint result = 0;
            GL_CALL(glGetIntegerv(GL_CURRENT_PROGRAM, &result));
            *(reinterpret_cast<GLint*>(arg[0])) = result;
        }
        break;

        case GLCommand::SET_CURRENT_PROGRAM_PTR:
        {
            GLint program = *(reinterpret_cast<GLint*>(arg[0]));
            GL_CALL(glUseProgram(program));
        }
        break;

        case GLCommand::SHADER_SOURCE:
        {
            GL_CALL(glShaderSource(GLuint(arg[0]), GLsizei(arg[1]), reinterpret_cast<const GLchar**>(arg[2]), reinterpret_cast<const GLint*>(arg[3])));
            cmd->status = err;
        }
        break;

        case GLCommand::COMPILE_SHADER:
        {
            GL_CALL(glCompileShader(GLuint(arg[0])));
            cmd->status = err;
        }
        break;

        case GLCommand::GET_SHADER_IV:
        {
            GL_CALL(glGetShaderiv(GLuint(arg[0]), GLenum(arg[1]), reinterpret_cast<GLint*>(arg[2])));
            cmd->status = err;
        }
        break;

        case GLCommand::GET_SHADER_INFO_LOG:
        {
            GL_CALL(glGetShaderInfoLog(GLuint(arg[0]), GLsizei(arg[1]), reinterpret_cast<GLsizei*>(arg[2]), reinterpret_cast<GLchar*>(arg[3])));
            cmd->status = err;
        }
        break;

        case GLCommand::GET_PROGRAM_IV:
        {
            GL_CALL(glGetProgramiv(GLuint(arg[0]), GLenum(arg[1]), reinterpret_cast<GLint*>(arg[2])));
            cmd->status = err;
        }
        break;

        case GLCommand::GET_ATTRIB_LOCATION:
        {
            GL_CALL(cmd->retval = glGetAttribLocation(GLuint(arg[0]), reinterpret_cast<const GLchar*>(arg[1])));
            cmd->status = 0;
        }
        break;

        case GLCommand::GET_ACTIVE_UNIFORM:
        {
            GL_CALL(glGetActiveUniform(GLuint(arg[0]), GLuint(arg[1]), GLsizei(arg[2]), reinterpret_cast<GLsizei*>(arg[3]), reinterpret_cast<GLint*>(arg[4]), reinterpret_cast<GLenum*>(arg[5]), reinterpret_cast<GLchar*>(arg[6])));
            cmd->status = err;
        }
        break;

        case GLCommand::GET_UNIFORM_LOCATION:
        {
            GL_CALL(cmd->retval = glGetUniformLocation(GLuint(arg[0]), reinterpret_cast<const GLchar*>(arg[1])));
            cmd->status = 0;
        }
        break;

        case GLCommand::SET_UNIFORM_1I:
        {
            GL_CALL(glUniform1i(GLint(arg[0]), GLint(arg[1])));
        }
        break;

        case GLCommand::GEN_FRAMEBUFFERS:
        {
            GL_CALL(glGenFramebuffers(GLuint(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::GEN_RENDERBUFFERS:
        {
            GL_CALL(glGenRenderbuffers(GLuint(arg[0]), reinterpret_cast<GLuint*>(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::DELETE_RENDERBUFFERS:
        {
            GL_CALL(glDeleteRenderbuffers(GLsizei(arg[0]), reinterpret_cast<const GLuint*>(arg[1])));
        }
        break;

        case GLCommand::BIND_FRAMEBUFFER:
        {
            GL_CALL(glBindFramebuffer(GLenum(arg[0]), GLuint(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::BIND_RENDERBUFFER:
        {
            GL_CALL(glBindRenderbuffer(GLenum(arg[0]), GLuint(arg[1])));
            cmd->status = err;
        }
        break;

        case GLCommand::FRAMEBUFFER_TEXTURE:
        {
            GL_CALL(glFramebufferTexture2D(GLenum(arg[0]), GLenum(arg[1]), GLenum(arg[2]), GLuint(arg[3]), GLint(arg[4])));
            cmd->status = err;
        }
        break;

        case GLCommand::RENDERBUFFER_STORAGE:
        {
            GLenum target = static_cast<GLenum>(arg[0]);
            GLenum internalFormat = static_cast<GLenum>(arg[1]);
            GLsizei width = static_cast<GLenum>(arg[2]);
            GLsizei height = static_cast<GLenum>(arg[3]);
            GLuint samples = static_cast<GLsizei>(arg[4]);
            if (samples > 1)
            {
                GL_CALL(glRenderbufferStorageMultisample(target, samples, internalFormat, width, height));
            }
            else
            {
                GL_CALL(glRenderbufferStorage(target, internalFormat, width, height));
            }
            cmd->status = err;
        }
        break;

        case GLCommand::FRAMEBUFFER_RENDERBUFFER:
        {
            GL_CALL(glFramebufferRenderbuffer(GLenum(arg[0]), GLenum(arg[1]), GLenum(arg[2]), GLuint(arg[3])));
            cmd->status = err;
        }
        break;

        case GLCommand::FRAMEBUFFER_STATUS:
        {
            GL_CALL(cmd->retval = glCheckFramebufferStatus(GLenum(arg[0])));
            cmd->status = 0;
        }
        break;

        case GLCommand::DELETE_FRAMEBUFFERS:
        {
            GL_CALL(glDeleteFramebuffers(GLsizei(arg[0]), reinterpret_cast<const GLuint*>(arg[1])));
            cmd->retval = 0;
            cmd->status = err;
        }
        break;

        case GLCommand::DRAWBUFFERS:
        {
                #if defined __DAVAENGINE_IPHONE__ || defined __DAVAENGINE_ANDROID__
                #else
            GL_CALL(glDrawBuffers(GLuint(arg[0]), reinterpret_cast<GLenum*>(arg[1])));
            cmd->status = err;
                #endif
        }
        break;

        case GLCommand::GET_QUERYOBJECT_UIV:
        {
#if defined(__DAVAENGINE_IPHONE__)
            EXEC_GL(glGetQueryObjectuivEXT(GLuint(arg[0]), GLenum(arg[1]), reinterpret_cast<GLuint*>(arg[2])));
#elif defined(__DAVAENGINE_ANDROID__)
            if (glGetQueryObjectuiv)
            {
                EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GLenum(arg[1]), reinterpret_cast<GLuint*>(arg[2])));
            }
#else
            EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GLenum(arg[1]), reinterpret_cast<GLuint*>(arg[2])));
#endif
            cmd->status = err;
        }
        break;

        case GLCommand::DELETE_QUERIES:
        {
#if defined(__DAVAENGINE_IPHONE__)
            EXEC_GL(glDeleteQueriesEXT(GLsizei(arg[0]), reinterpret_cast<const GLuint*>(arg[1])));
#elif defined(__DAVAENGINE_ANDROID__)
            if (glDeleteQueries)
            {
                EXEC_GL(glDeleteQueries(GLsizei(arg[0]), reinterpret_cast<const GLuint*>(arg[1])));
            }
#else
            EXEC_GL(glDeleteQueries(GLsizei(arg[0]), reinterpret_cast<const GLuint*>(arg[1])));
#endif
            cmd->status = err;
        }
        break;

        case GLCommand::GET_QUERY_RESULT_NO_WAIT:
        {
            GLuint result = 0;

#if defined(__DAVAENGINE_IPHONE__)
            EXEC_GL(glGetQueryObjectuivEXT(GLuint(arg[0]), GL_QUERY_RESULT_AVAILABLE, &result));
#elif defined(__DAVAENGINE_ANDROID__)
            if (glGetQueryObjectuiv)
            {
                EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GL_QUERY_RESULT_AVAILABLE, &result));
            }
#else
            EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GL_QUERY_RESULT_AVAILABLE, &result));
#endif
            cmd->status = err;

            if (err == GL_NO_ERROR && result)
            {
#if defined(__DAVAENGINE_IPHONE__)
                EXEC_GL(glGetQueryObjectuivEXT(GLuint(arg[0]), GL_QUERY_RESULT, reinterpret_cast<GLuint*>(arg[1])));
#elif defined(__DAVAENGINE_ANDROID__)
                if (glGetQueryObjectuiv)
                {
                    EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GL_QUERY_RESULT, reinterpret_cast<GLuint*>(arg[1])));
                }
#else
                EXEC_GL(glGetQueryObjectuiv(GLuint(arg[0]), GL_QUERY_RESULT, reinterpret_cast<GLuint*>(arg[1])));
#endif
            }
        }
        break;

        case GLCommand::SYNC_CPU_GPU:
        {
            if (_GLES2_TimeStampQuerySupported)
            {
                GLuint64 gpuTimestamp = 0, cpuTimestamp = 0;
                GLuint query = 0;

#if defined(__DAVAENGINE_IPHONE__)
#elif defined(__DAVAENGINE_MACOS__)
#elif defined(__DAVAENGINE_ANDROID__)

                if (glGenQueries)
                    GL_CALL(glGenQueries(1, &query));

                if (query)
                {
                    if (glQueryCounter)
                        GL_CALL(glQueryCounter(query, GL_TIMESTAMP));

                    GL_CALL(glFinish());

                    if (glGetQueryObjectui64v)
                        GL_CALL(glGetQueryObjectui64v(query, GL_QUERY_RESULT, &gpuTimestamp));

                    cpuTimestamp = DAVA::SystemTimer::GetUs();

                    if (glDeleteQueries)
                        GL_CALL(glDeleteQueries(1, &query));
                }

#else

                GL_CALL(glGenQueries(1, &query));

                if (query)
                {
                    GL_CALL(glQueryCounter(query, GL_TIMESTAMP));
                    GL_CALL(glGetQueryObjectui64v(query, GL_QUERY_RESULT, &gpuTimestamp));
                    cpuTimestamp = DAVA::SystemTimer::GetUs();
                    GL_CALL(glDeleteQueries(1, &query));
                }

#endif

                *reinterpret_cast<uint64*>(arg[0]) = cpuTimestamp;
                *reinterpret_cast<uint64*>(arg[1]) = gpuTimestamp / 1000; //mcs
            }
        }
        break;
        }
    }
    RELEASE_CONTEXT();
#undef EXEC_GL
}

//------------------------------------------------------------------------------

void ExecGL(GLCommand* command, uint32 cmdCount, bool forceExecute)
{
    CommonImpl::ImmediateCommand cmd;
    cmd.cmdData = command;
    cmd.cmdCount = cmdCount;
    cmd.forceExecute = forceExecute;
    RenderLoop::IssueImmediateCommand(&cmd);
}

namespace CommandBufferGLES2
{
void Init(uint32 maxCount)
{
    CommandBufferPoolGLES2::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_CommandBuffer_Begin = &gles2_CommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = &gles2_CommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = &gles2_CommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = &gles2_CommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = &gles2_CommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = &gles2_CommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = &gles2_CommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = &gles2_CommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = &gles2_CommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = &gles2_CommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = &gles2_CommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryBuffer = &gles2_CommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_SetQueryIndex = &gles2_CommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_IssueTimestampQuery = &gles2_CommandBuffer_IssueTimestampQuery;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = &gles2_CommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = &gles2_CommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = &gles2_CommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = &gles2_CommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = &gles2_CommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = &gles2_CommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = &gles2_CommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = &gles2_CommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = &gles2_CommandBuffer_SetMarker;

    dispatch->impl_SyncObject_Create = &gles2_SyncObject_Create;
    dispatch->impl_SyncObject_Delete = &gles2_SyncObject_Delete;
    dispatch->impl_SyncObject_IsSignaled = &gles2_SyncObject_IsSignaled;

    dispatch->impl_ProcessImmediateCommand = _GLES2_ExecImmediateCommand;
    dispatch->impl_ExecuteFrame = _GLES2_ExecuteQueuedCommands;
    dispatch->impl_RejectFrame = _GLES2_RejectFrame;
    dispatch->impl_PresentBuffer = _GLES2_PresentBuffer;
    dispatch->impl_ResetBlock = _GLES2_ResetBlock;
    dispatch->impl_FinishFrame = _GLES2_FinishFrame;
}
}

} // namespace rhi
