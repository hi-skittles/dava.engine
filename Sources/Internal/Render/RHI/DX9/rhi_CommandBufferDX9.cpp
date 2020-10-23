#include "../Common/rhi_Pool.h"
#include "rhi_DX9.h"
#include "../rhi_Type.h"
#include "../Common/rhi_RingBuffer.h"
#include "../Common/rhi_FormatConversion.h"
#include "../Common/dbg_StatSet.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Concurrency/Thread.h"
#include "Concurrency/Semaphore.h"
#include "Time/SystemTimer.h"

#include "../Common/SoftwareCommandBuffer.h"
#include "../Common/RenderLoop.h"
#include "../Common/rhi_CommonImpl.h"

#include "_dx9.h"
#include <vector>
#include <atomic>
#include <thread>

#if defined(DAVA_ACQUIRE_OGL_CONTEXT_EVERYTIME)
#define DAVA_DISABLE_CLEAR_ON_RESET 1
#endif

namespace rhi
{
//==============================================================================

static uint32 _DX9_FramesWithRestoreAttempt = 0;
const uint32 _DX9_MaxFramesWithRestoreAttempt = 15;

//------------------------------------------------------------------------------

static inline D3DPRIMITIVETYPE _DX9_PrimitiveType(PrimitiveType type)
{
    D3DPRIMITIVETYPE type9 = D3DPT_TRIANGLELIST;

    switch (type)
    {
    case PRIMITIVE_TRIANGLELIST:
        type9 = D3DPT_TRIANGLELIST;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        type9 = D3DPT_TRIANGLESTRIP;
        break;

    case PRIMITIVE_LINELIST:
        type9 = D3DPT_LINELIST;
        break;
    }

    return type9;
}

//------------------------------------------------------------------------------

static IDirect3DIndexBuffer9* _DX9_SequentialIB()
{
    static IDirect3DIndexBuffer9* ib = NULL;

    if (!ib)
    {
        HRESULT hr;

        hr = _D3D9_Device->CreateIndexBuffer(0xFFFF * sizeof(uint16), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, NULL);
        if (SUCCEEDED(hr))
        {
            void* idata;

            hr = ib->Lock(0, 0xFFFF * sizeof(uint16), &idata, 0);
            if (SUCCEEDED(hr))
            {
                uint16 idx = 0;

                for (uint16 *i = (uint16 *)idata, *i_end = ((uint16 *)idata) + 0xFFFF; i != i_end; ++i, ++idx)
                    *i = idx;

                ib->Unlock();
            }
        }
    }

    return ib;
}

class
RenderPassDX9_t
{
public:
    std::vector<Handle> cmdBuf;
    int priority;
    Handle perfQueryStart;
    Handle perfQueryEnd;
};

class CommandBufferDX9_t : public SoftwareCommandBuffer
{
public:
    CommandBufferDX9_t();

    void Execute();

    RenderPassConfig passCfg; //-V730_NOINIT
    Handle sync = InvalidHandle;
    RingBuffer* text = nullptr;
    uint32 isFirstInPass : 1;
    uint32 isLastInPass : 1;
};

struct SyncObjectDX9_t
{
    uint32 frame;
    uint32 is_signaled : 1;
    uint32 is_used : 1;
};

typedef ResourcePool<CommandBufferDX9_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false> CommandBufferPoolDX9;
typedef ResourcePool<RenderPassDX9_t, RESOURCE_RENDER_PASS, RenderPassConfig, false> RenderPassPoolDX9;
typedef ResourcePool<SyncObjectDX9_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false> SyncObjectPoolDX9;

RHI_IMPL_POOL(CommandBufferDX9_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false);
RHI_IMPL_POOL(RenderPassDX9_t, RESOURCE_RENDER_PASS, RenderPassConfig, false);
RHI_IMPL_POOL(SyncObjectDX9_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false);

static DAVA::Mutex _DX9_SyncObjectsSync;

static Handle dx9_RenderPass_Allocate(const RenderPassConfig& passDesc, uint32 cmdBufCount, Handle* cmdBuf)
{
    DVASSERT(cmdBufCount);
    DVASSERT(passDesc.IsValid());

    Handle handle = RenderPassPoolDX9::Alloc();
    RenderPassDX9_t* pass = RenderPassPoolDX9::Get(handle);

    pass->cmdBuf.resize(cmdBufCount);
    pass->priority = passDesc.priority;
    pass->perfQueryStart = passDesc.perfQueryStart;
    pass->perfQueryEnd = passDesc.perfQueryEnd;

    for (uint32 i = 0; i != cmdBufCount; ++i)
    {
        Handle h = CommandBufferPoolDX9::Alloc();
        CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(h);

        cb->passCfg = passDesc;
        cb->isFirstInPass = i == 0;
        cb->isLastInPass = i == cmdBufCount - 1;

        pass->cmdBuf[i] = h;
        cmdBuf[i] = h;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void dx9_RenderPass_Begin(Handle pass)
{
}

//------------------------------------------------------------------------------

static void dx9_RenderPass_End(Handle pass)
{
}

namespace RenderPassDX9
{
void Init(uint32 maxCount)
{
    RenderPassPoolDX9::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Renderpass_Allocate = &dx9_RenderPass_Allocate;
    dispatch->impl_Renderpass_Begin = &dx9_RenderPass_Begin;
    dispatch->impl_Renderpass_End = &dx9_RenderPass_End;
}
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_Begin(Handle cmdBuf)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    cb->curUsedSize = 0;
    cb->allocCmd<SWCommand_Begin>();
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_End(Handle cmdBuf, Handle syncObject)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_End* cmd = cb->allocCmd<SWCommand_End>();
    cmd->syncObject = syncObject;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetPipelineState(Handle cmdBuf, Handle ps, uint32 vdecl)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetPipelineState* cmd = cb->allocCmd<SWCommand_SetPipelineState>();
    cmd->vdecl = uint16(vdecl);
    cmd->ps = ps;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetCullMode(Handle cmdBuf, CullMode mode)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetCullMode* cmd = cb->allocCmd<SWCommand_SetCullMode>();
    cmd->mode = mode;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetScissorRect(Handle cmdBuf, ScissorRect rect)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetScissorRect* cmd = cb->allocCmd<SWCommand_SetScissorRect>();
    cmd->x = rect.x;
    cmd->y = rect.y;
    cmd->width = rect.width;
    cmd->height = rect.height;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetViewport(Handle cmdBuf, Viewport vp)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetViewport* cmd = cb->allocCmd<SWCommand_SetViewport>();
    cmd->x = uint16(vp.x);
    cmd->y = uint16(vp.y);
    cmd->width = uint16(vp.width);
    cmd->height = uint16(vp.height);
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetFillMode(Handle cmdBuf, FillMode mode)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetFillMode* cmd = cb->allocCmd<SWCommand_SetFillMode>();
    cmd->mode = mode;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetVertexData* cmd = cb->allocCmd<SWCommand_SetVertexData>();
    cmd->vb = vb;
    cmd->streamIndex = uint16(streamIndex);
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    //    L_ASSERT(buffer);
    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);
    if (buffer != DAVA::InvalidIndex)
    {
        CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
        SWCommand_SetVertexProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetVertexProgConstBuffer>();
        cmd->buffer = buffer;
        cmd->bufIndex = bufIndex;
        cmd->inst = ConstBufferDX9::Instance(buffer);
    }
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetVertexTexture* cmd = cb->allocCmd<SWCommand_SetVertexTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetIndices(Handle cmdBuf, Handle ib)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetIndices* cmd = cb->allocCmd<SWCommand_SetIndices>();
    cmd->ib = ib;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetQueryIndex(Handle cmdBuf, uint32 objectIndex)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetQueryIndex* cmd = cb->allocCmd<SWCommand_SetQueryIndex>();
    cmd->objectIndex = objectIndex;
}

static void dx9_CommandBuffer_IssueTimestampQuery(Handle cmdBuf, Handle query)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_IssueTimestamptQuery* cmd = cb->allocCmd<SWCommand_IssueTimestamptQuery>();
    cmd->perfQuery = query;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetQueryBuffer(Handle cmdBuf, Handle queryBuf)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetQueryBuffer* cmd = cb->allocCmd<SWCommand_SetQueryBuffer>();
    cmd->queryBuf = queryBuf;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);

    if (buffer != DAVA::InvalidIndex)
    {
        CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
        SWCommand_SetFragmentProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetFragmentProgConstBuffer>();
        cmd->bufIndex = bufIndex;
        cmd->buffer = buffer;
        cmd->inst = ConstBufferDX9::Instance(buffer);
    }
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetFragmentTexture* cmd = cb->allocCmd<SWCommand_SetFragmentTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetDepthStencilState* cmd = cb->allocCmd<SWCommand_SetDepthStencilState>();
    cmd->depthStencilState = depthStencilState;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_SetSamplerState* cmd = cb->allocCmd<SWCommand_SetSamplerState>();
    cmd->samplerState = samplerState;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_DrawPrimitive* cmd = cb->allocCmd<SWCommand_DrawPrimitive>();
    cmd->mode = _DX9_PrimitiveType(type);
    cmd->vertexCount = count;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count, uint32 vertexCount, uint32 firstVertex, uint32 startIndex)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_DrawIndexedPrimitiveRanged* cmd = cb->allocCmd<SWCommand_DrawIndexedPrimitiveRanged>();
    cmd->mode = _DX9_PrimitiveType(type);
    cmd->indexCount = count;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;
    cmd->vertexCount = vertexCount;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_DrawInstancedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedPrimitive>();
    cmd->mode = _DX9_PrimitiveType(type);
    cmd->vertexCount = count;
    cmd->instanceCount = instCount;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count, uint32 vertexCount, uint32 firstVertex, uint32 startIndex, uint32 baseInstance)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);
    SWCommand_DrawInstancedIndexedPrimitiveRanged* cmd = cb->allocCmd<SWCommand_DrawInstancedIndexedPrimitiveRanged>();
    cmd->mode = _DX9_PrimitiveType(type);
    cmd->indexCount = count;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;
    cmd->vertexCount = vertexCount;
    cmd->instanceCount = instCount;
    cmd->baseInstance = baseInstance;
}

//------------------------------------------------------------------------------

static void dx9_CommandBuffer_SetMarker(Handle cmdBuf, const char* text)
{
    CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cmdBuf);

    if (!cb->text)
    {
        cb->text = new RingBuffer();
        cb->text->Initialize(64 * 1024);
    }

    size_t len = strlen(text);
    char* txt = (char*)cb->text->Alloc(static_cast<unsigned>(len / sizeof(float) + 1));

    memcpy(txt, text, len);
    txt[len] = '\0';

    SWCommand_SetMarker* cmd = cb->allocCmd<SWCommand_SetMarker>();
    cmd->text = text;
}

//------------------------------------------------------------------------------

static Handle dx9_SyncObject_Create()
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX9_SyncObjectsSync);
    Handle handle = SyncObjectPoolDX9::Alloc();
    SyncObjectDX9_t* sync = SyncObjectPoolDX9::Get(handle);

    sync->is_signaled = false;
    sync->is_used = false;

    return handle;
}

//------------------------------------------------------------------------------

static void dx9_SyncObject_Delete(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX9_SyncObjectsSync);
    SyncObjectPoolDX9::Free(obj);
}

//------------------------------------------------------------------------------

static bool dx9_SyncObject_IsSignaled(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX9_SyncObjectsSync);
    if (!SyncObjectPoolDX9::IsAlive(obj))
        return true;

    bool signaled = false;
    SyncObjectDX9_t* sync = SyncObjectPoolDX9::Get(obj);

    if (sync)
        signaled = sync->is_signaled;

    return signaled;
}

CommandBufferDX9_t::CommandBufferDX9_t()
    : sync(InvalidHandle)
    , isFirstInPass(true)
    , isLastInPass(true)
{
}

void CommandBufferDX9_t::Execute()
{
    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_CMD_BUFFER_EXECUTE);

    Handle cur_pipelinestate = InvalidHandle;
    uint32 cur_vd_uid = VertexLayout::InvalidUID;
    uint32 cur_stride[MAX_VERTEX_STREAM_COUNT];
    Handle cur_query_buf = InvalidHandle;
    D3DVIEWPORT9 def_viewport;

    memset(cur_stride, 0, sizeof(cur_stride));

    _D3D9_Device->GetViewport(&def_viewport);

    sync = InvalidHandle;

    int immediate_cmd_ttw = 10;

    for (const uint8 *c = cmdData, *c_end = cmdData + curUsedSize; c != c_end;)
    {
        const SWCommand* cmd = reinterpret_cast<const SWCommand*>(c);

        switch (SoftwareCommandType(cmd->type))
        {
        case CMD_BEGIN:
        {
            if (isFirstInPass)
            {
                _D3D9_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
                _D3D9_TargetCount = 0;
                for (unsigned i = 0; i != countof(passCfg.colorBuffer); ++i)
                {
                    if (passCfg.colorBuffer[i].texture != rhi::InvalidHandle || passCfg.UsingMSAA())
                    {
                        if (i == 0)
                        {
                            DVASSERT(_D3D9_BackBuf == nullptr);
                            _D3D9_Device->GetRenderTarget(0, &_D3D9_BackBuf);

                            if (passCfg.UsingMSAA())
                                TextureDX9::SetAsRenderTarget(passCfg.colorBuffer[i].multisampleTexture, i, passCfg.colorBuffer[i].textureFace);
                        }

                        if (!passCfg.UsingMSAA())
                            TextureDX9::SetAsRenderTarget(passCfg.colorBuffer[i].texture, i, passCfg.colorBuffer[i].textureFace);
                        ++_D3D9_TargetCount;
                    }

                    if (passCfg.colorBuffer[i].texture == rhi::InvalidHandle && i == 0)
                    {
                        _D3D9_TargetCount = 1;
                        break;
                    }
                }

                _D3D9_Device->GetDepthStencilSurface(&_D3D9_DepthBuf);

                bool hasDepthBuf = true;
                if (passCfg.UsingMSAA() && passCfg.depthStencilBuffer.multisampleTexture != rhi::InvalidHandle)
                {
                    TextureDX9::SetAsDepthStencil(passCfg.depthStencilBuffer.multisampleTexture);
                }
                else if (passCfg.depthStencilBuffer.texture != rhi::DefaultDepthBuffer)
                {
                    if (passCfg.depthStencilBuffer.texture != rhi::InvalidHandle)
                    {
                        TextureDX9::SetAsDepthStencil(passCfg.depthStencilBuffer.texture);
                    }
                    else
                    {
                        DX9_CALL(_D3D9_Device->SetDepthStencilSurface(NULL), "SetDepthStencilSurface");
                        hasDepthBuf = false;
                    }
                }

                IDirect3DSurface9* rt = nullptr;
                _D3D9_Device->GetRenderTarget(0, &rt);
                if (rt != nullptr)
                {
                    D3DSURFACE_DESC desc = {};
                    if (SUCCEEDED(rt->GetDesc(&desc)))
                    {
                        def_viewport.X = 0;
                        def_viewport.Y = 0;
                        def_viewport.Width = desc.Width;
                        def_viewport.Height = desc.Height;
                        def_viewport.MinZ = 0.0f;
                        def_viewport.MaxZ = 1.0f;
                    }
                    rt->Release();
                }

                bool clear_color = passCfg.colorBuffer[0].loadAction == LOADACTION_CLEAR;
                bool clear_depth = hasDepthBuf && (passCfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR);

                DX9_CALL(_D3D9_Device->BeginScene(), "BeginScene");

                if (clear_color || clear_depth)
                {
                    _D3D9_Device->SetViewport(&def_viewport);
                    _D3D9_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

                    DWORD flags = 0;
                    int r = int(passCfg.colorBuffer[0].clearColor[0] * 255.0f);
                    int g = int(passCfg.colorBuffer[0].clearColor[1] * 255.0f);
                    int b = int(passCfg.colorBuffer[0].clearColor[2] * 255.0f);
                    int a = int(passCfg.colorBuffer[0].clearColor[3] * 255.0f);

                    if (clear_color)
                        flags |= D3DCLEAR_TARGET;
                    if (clear_depth)
                        flags |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;

                    DX9_CALL(_D3D9_Device->Clear(0, NULL, flags, D3DCOLOR_RGBA(r, g, b, a), passCfg.depthStencilBuffer.clearDepth, 0), "Clear");
                }

                DVASSERT(cur_query_buf == InvalidHandle || !QueryBufferDX9::QueryIsCompleted(cur_query_buf));
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
                    QueryBufferDX9::QueryComplete(cur_query_buf);
                }

                DX9_CALL(_D3D9_Device->EndScene(), "EndScene");

                for (unsigned t = 0; t != MAX_RENDER_TARGET_COUNT; ++t)
                {
                    if (passCfg.colorBuffer[t].storeAction == rhi::STOREACTION_RESOLVE)
                        TextureDX9::ResolveMultisampling(passCfg.colorBuffer[t].multisampleTexture, passCfg.colorBuffer[t].texture);
                }

                if (_D3D9_BackBuf)
                {
                    DX9_CALL(_D3D9_Device->SetRenderTarget(0, _D3D9_BackBuf), "SetRenderTarget");
                    _D3D9_BackBuf->Release();
                    _D3D9_BackBuf = nullptr;
                }
                if (_D3D9_DepthBuf)
                {
                    DX9_CALL(_D3D9_Device->SetDepthStencilSurface(_D3D9_DepthBuf), "SetDepthStencilSurface");
                    _D3D9_DepthBuf->Release();
                    _D3D9_DepthBuf = nullptr;
                }

                for (unsigned i = 1; i != _D3D9_TargetCount; ++i)
                    _D3D9_Device->SetRenderTarget(i, NULL);
                _D3D9_TargetCount = 1;
            }
        }
        break;

        case CMD_SET_VERTEX_DATA:
        {
            DVASSERT(cur_pipelinestate != InvalidHandle);
            Handle vb = (static_cast<const SWCommand_SetVertexData*>(cmd))->vb;
            unsigned stream = (static_cast<const SWCommand_SetVertexData*>(cmd))->streamIndex;
            unsigned stride = (cur_stride[stream]) ? cur_stride[stream] : PipelineStateDX9::VertexLayoutStride(cur_pipelinestate, stream);
            VertexBufferDX9::SetToRHI(vb, stream, 0, stride);

            StatSet::IncStat(stat_SET_VB, 1);
        }
        break;

        case CMD_SET_INDICES:
        {
            Handle ib = (static_cast<const SWCommand_SetIndices*>(cmd))->ib;
            IndexBufferDX9::SetToRHI(ib);

            StatSet::IncStat(stat_SET_IB, 1);
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
                QueryBufferDX9::SetQueryIndex(cur_query_buf, (static_cast<const SWCommand_SetQueryIndex*>(cmd))->objectIndex);
        }
        break;

        case CMD_ISSUE_TIMESTAMP_QUERY:
        {
            PerfQueryDX9::IssueTimestampQuery(static_cast<const SWCommand_IssueTimestamptQuery*>(cmd)->perfQuery);
        }
        break;

        case CMD_SET_PIPELINE_STATE:
        {
            Handle ps = (static_cast<const SWCommand_SetPipelineState*>(cmd))->ps;
            uint32 vd_uid = (static_cast<const SWCommand_SetPipelineState*>(cmd))->vdecl;

            const VertexLayout* vdecl = (vd_uid == VertexLayout::InvalidUID) ? nullptr : VertexLayout::Get(vd_uid);
            cur_pipelinestate = ps; //todo: cache optimization
            cur_vd_uid = vd_uid;

            if (vdecl)
            {
                for (unsigned s = 0; s != vdecl->StreamCount(); ++s)
                    cur_stride[s] = vdecl->Stride(s);
            }
            else
            {
                memset(cur_stride, 0, sizeof(cur_stride));
            }

            PipelineStateDX9::SetToRHI(cur_pipelinestate, vd_uid);

            StatSet::IncStat(stat_SET_PS, 1);
        }
        break;

        case CMD_SET_CULL_MODE:
        {
            DWORD mode = D3DCULL_CCW;
            switch (CullMode((static_cast<const SWCommand_SetCullMode*>(cmd))->mode))
            {
            case CULL_NONE:
                mode = D3DCULL_NONE;
                break;
            case CULL_CW:
                mode = D3DCULL_CW;
                break;
            case CULL_CCW:
                mode = D3DCULL_CCW;
                break;
            }

            _D3D9_Device->SetRenderState(D3DRS_CULLMODE, mode);
        }
        break;

        case CMD_SET_SCISSOR_RECT:
        {
            int32 x = (static_cast<const SWCommand_SetScissorRect*>(cmd))->x;
            int32 y = (static_cast<const SWCommand_SetScissorRect*>(cmd))->y;
            int32 w = (static_cast<const SWCommand_SetScissorRect*>(cmd))->width;
            int32 h = (static_cast<const SWCommand_SetScissorRect*>(cmd))->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                RECT rect = { x, y, x + w, y + h };

                _D3D9_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
                _D3D9_Device->SetScissorRect(&rect);
            }
            else
            {
                _D3D9_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            }
        }
        break;

        case CMD_SET_VIEWPORT:
        {
            int32 x = (static_cast<const SWCommand_SetViewport*>(cmd))->x;
            int32 y = (static_cast<const SWCommand_SetViewport*>(cmd))->y;
            int32 w = (static_cast<const SWCommand_SetViewport*>(cmd))->width;
            int32 h = (static_cast<const SWCommand_SetViewport*>(cmd))->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                D3DVIEWPORT9 vp;

                vp.X = x;
                vp.Y = y;
                vp.Width = w;
                vp.Height = h;
                vp.MinZ = 0.0f;
                vp.MaxZ = 1.0f;

                _D3D9_Device->SetViewport(&vp);
            }
            else
            {
                _D3D9_Device->SetViewport(&def_viewport);
            }
        }
        break;

        case CMD_SET_FILLMODE:
        {
            DWORD mode = D3DFILL_SOLID;
            switch (FillMode((static_cast<const SWCommand_SetFillMode*>(cmd))->mode))
            {
            case FILLMODE_SOLID:
                mode = D3DFILL_SOLID;
                break;
            case FILLMODE_WIREFRAME:
                mode = D3DFILL_WIREFRAME;
                break;
            }

            _D3D9_Device->SetRenderState(D3DRS_FILLMODE, mode);
        }
        break;

        case CMD_SET_DEPTHSTENCIL_STATE:
        {
            Handle state = (static_cast<const SWCommand_SetDepthStencilState*>(cmd))->depthStencilState;
            DepthStencilStateDX9::SetToRHI(state);
        }
        break;

        case CMD_SET_SAMPLER_STATE:
        {
            Handle state = (static_cast<const SWCommand_SetSamplerState*>(cmd))->samplerState;
            SamplerStateDX9::SetToRHI(state);
            StatSet::IncStat(stat_SET_SS, 1);
        }
        break;

        case CMD_SET_VERTEX_PROG_CONST_BUFFER:
        {
            Handle buffer = (static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd))->buffer;
            const void* inst = (static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd))->inst;
            ConstBufferDX9::SetToRHI(buffer, inst);
            StatSet::IncStat(stat_SET_CB, 1);
        }
        break;

        case CMD_SET_FRAGMENT_PROG_CONST_BUFFER:
        {
            Handle buffer = (static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd))->buffer;
            const void* inst = (static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd))->inst;
            ConstBufferDX9::SetToRHI(buffer, inst);
            StatSet::IncStat(stat_SET_CB, 1);
        }
        break;

        case CMD_SET_VERTEX_TEXTURE:
        {
            Handle tex = (static_cast<const SWCommand_SetVertexTexture*>(cmd))->tex;
            uint32 unit_i = (static_cast<const SWCommand_SetVertexTexture*>(cmd))->unitIndex;
            unit_i += D3DDMAPSAMPLER + 1;
            TextureDX9::SetToRHI(tex, unit_i);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_SET_FRAGMENT_TEXTURE:
        {
            Handle tex = (static_cast<const SWCommand_SetFragmentTexture*>(cmd))->tex;
            unsigned unit_i = (static_cast<const SWCommand_SetFragmentTexture*>(cmd))->unitIndex;
            TextureDX9::SetToRHI(tex, unit_i);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_DRAW_PRIMITIVE:
        {
            uint32 v_cnt = (static_cast<const SWCommand_DrawPrimitive*>(cmd))->vertexCount;
            uint8 mode = (static_cast<const SWCommand_DrawPrimitive*>(cmd))->mode;
            DX9_CALL(_D3D9_Device->DrawPrimitive((D3DPRIMITIVETYPE)(mode), /*base_vertex*/ 0, UINT(v_cnt)), "DrawPrimitive");

            StatSet::IncStat(stat_DP, 1);
            switch (mode)
            {
            case D3DPT_TRIANGLELIST:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case D3DPT_TRIANGLESTRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case D3DPT_LINELIST:
                StatSet::IncStat(stat_DLL, 1);
                break;

            default:
                break;
            }
        }
        break;

        case CMD_DRAW_INDEXED_PRIMITIVE_RANGED:
        {
            D3DPRIMITIVETYPE type = (D3DPRIMITIVETYPE)((static_cast<const SWCommand_DrawIndexedPrimitiveRanged*>(cmd))->mode);
            uint32 primCount = uint32((static_cast<const SWCommand_DrawIndexedPrimitiveRanged*>(cmd))->indexCount);
            uint32 vertexCount = uint32((static_cast<const SWCommand_DrawIndexedPrimitiveRanged*>(cmd))->vertexCount);
            uint32 firstVertex = uint32((static_cast<const SWCommand_DrawIndexedPrimitiveRanged*>(cmd))->firstVertex);
            uint32 startIndex = uint32((static_cast<const SWCommand_DrawIndexedPrimitiveRanged*>(cmd))->startIndex);

            DX9_CALL(_D3D9_Device->DrawIndexedPrimitive(type, firstVertex, 0, vertexCount, startIndex, primCount), "DrawIndexedPrimitive");

            StatSet::IncStat(stat_DIP, 1);
            switch (type)
            {
            case D3DPT_TRIANGLELIST:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case D3DPT_TRIANGLESTRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case D3DPT_LINELIST:
                StatSet::IncStat(stat_DLL, 1);
                break;

            default:
                break;
            }
        }
        break;

        case CMD_DRAW_INSTANCED_PRIMITIVE:
        {
            D3DPRIMITIVETYPE primType = D3DPRIMITIVETYPE((static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->mode);
            uint32 instCount = uint32((static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->instanceCount);
            uint32 primCount = uint32((static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd))->vertexCount);

            DVASSERT(primType == D3DPT_TRIANGLELIST);
            DVASSERT(primCount / 3 < 0xFFFF);
            PipelineStateDX9::SetupVertexStreams(cur_pipelinestate, cur_vd_uid, instCount);
            DX9_CALL(_D3D9_Device->SetIndices(_DX9_SequentialIB()), "SetIndices");
            DX9_CALL(_D3D9_Device->DrawIndexedPrimitive(primType, 0, 0, primCount * 3, 0, primCount), "DrawInstancedIndexedPrimitive");
            StatSet::IncStat(stat_DIP, 1);
            switch (primType)
            {
            case D3DPT_TRIANGLELIST:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case D3DPT_TRIANGLESTRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case D3DPT_LINELIST:
                StatSet::IncStat(stat_DLL, 1);
                break;

            default:
                break;
            }
        }
        break;

        case CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE_RANGED:
        {
            uint32 primCount = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->indexCount;
            D3DPRIMITIVETYPE type = (D3DPRIMITIVETYPE)((static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->mode);
            uint32 instCount = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->instanceCount;
            uint32 firstVertex = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->firstVertex;
            uint32 vertexCount = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->vertexCount;
            uint32 startIndex = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->startIndex;
            uint32 baseInst = (static_cast<const SWCommand_DrawInstancedIndexedPrimitiveRanged*>(cmd))->baseInstance;

            PipelineStateDX9::SetupVertexStreams(cur_pipelinestate, cur_vd_uid, instCount);
            DX9_CALL(_D3D9_Device->DrawIndexedPrimitive(type, firstVertex, 0, vertexCount, startIndex, primCount), "DrawIndexedPrimitive");

            StatSet::IncStat(stat_DIP, 1);
            switch (type)
            {
            case D3DPT_TRIANGLELIST:
                StatSet::IncStat(stat_DTL, 1);
                break;

            case D3DPT_TRIANGLESTRIP:
                StatSet::IncStat(stat_DTS, 1);
                break;

            case D3DPT_LINELIST:
                StatSet::IncStat(stat_DLL, 1);
                break;

            default:
                break;
            }
        }
        break;

        case CMD_SET_MARKER:
        {
            const char* text = (static_cast<const SWCommand_SetMarker*>(cmd))->text;
            wchar_t txt[128];

            ::MultiByteToWideChar(CP_ACP, 0, (const char*)(text), -1, txt, countof(txt));
            ::D3DPERF_SetMarker(D3DCOLOR_ARGB(0xFF, 0x40, 0x40, 0x80), txt);
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
}

static void _DX9_InvalidateFrameCache()
{
    ConstBufferDX9::InvalidateAllConstBufferInstances();
}

static void _DX9_RejectFrame(const CommonImpl::Frame& frame)
{
    if (frame.sync != InvalidHandle)
    {
        DAVA::LockGuard<DAVA::Mutex> guard(_DX9_SyncObjectsSync);
        SyncObjectDX9_t* s = SyncObjectPoolDX9::Get(frame.sync);
        s->is_signaled = true;
        s->is_used = true;
    }
    for (Handle p : frame.pass)
    {
        RenderPassDX9_t* pp = RenderPassPoolDX9::Get(p);

        for (Handle c : pp->cmdBuf)
        {
            CommandBufferDX9_t* cc = CommandBufferPoolDX9::Get(c);
            if (cc->sync != InvalidHandle)
            {
                DAVA::LockGuard<DAVA::Mutex> guard(_DX9_SyncObjectsSync);
                SyncObjectDX9_t* s = SyncObjectPoolDX9::Get(cc->sync);
                s->is_signaled = true;
                s->is_used = true;
            }
            CommandBufferPoolDX9::Free(c);
        }

        RenderPassPoolDX9::Free(p);
    }

#if defined(ENABLE_ASSERT_MESSAGE) || defined(ENABLE_ASSERT_LOGGING)
    if (NeedRestoreResources())
        ++_DX9_FramesWithRestoreAttempt;
    if (_DX9_FramesWithRestoreAttempt > _DX9_MaxFramesWithRestoreAttempt)
    {
        TextureDX9::LogUnrestoredBacktraces();
        VertexBufferDX9::LogUnrestoredBacktraces();
        IndexBufferDX9::LogUnrestoredBacktraces();
        DVASSERT(0, "Failed to restore all resources in time.");
    }
#endif
}

static void _DX9_ExecuteQueuedCommands(const CommonImpl::Frame& frame)
{
    DVASSERT((frame.sync == InvalidHandle) || SyncObjectPoolDX9::IsAlive(frame.sync));

    StatSet::ResetAll();

    _DX9_FramesWithRestoreAttempt = 0;

    PerfQueryDX9::ObtainPerfQueryMeasurment();

    std::vector<RenderPassDX9_t*> pass;

    for (Handle p : frame.pass)
    {
        RenderPassDX9_t* pp = RenderPassPoolDX9::Get(p);

        bool do_add = true;
        for (unsigned i = 0; i != pass.size(); ++i)
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
    }

    uint32 frame_n = frame.frameNumber;

    if (frame.sync != InvalidHandle)
    {
        SyncObjectDX9_t* sync = SyncObjectPoolDX9::Get(frame.sync);
        sync->frame = frame_n;
        sync->is_signaled = false;
        sync->is_used = true;
    }

    PerfQueryDX9::BeginMeasurment();
    if (frame.perfQueryStart != InvalidHandle)
        PerfQueryDX9::IssueTimestampQuery(frame.perfQueryStart);

    for (RenderPassDX9_t* pp : pass)
    {
        if (pp->perfQueryStart)
            PerfQueryDX9::IssueTimestampQuery(pp->perfQueryStart);

        for (unsigned b = 0; b != pp->cmdBuf.size(); ++b)
        {
            Handle cb_h = pp->cmdBuf[b];

            CommandBufferDX9_t* cb = CommandBufferPoolDX9::Get(cb_h);
            cb->Execute();

            if (cb->sync != InvalidHandle)
            {
                SyncObjectDX9_t* sync = SyncObjectPoolDX9::Get(cb->sync);
                sync->frame = frame_n;
                sync->is_signaled = false;
                sync->is_used = true;
            }

            CommandBufferPoolDX9::Free(cb_h);
        }

        if (pp->perfQueryEnd)
            PerfQueryDX9::IssueTimestampQuery(pp->perfQueryEnd);
    }

    for (Handle p : frame.pass)
        RenderPassPoolDX9::Free(p);

    if (frame.perfQueryEnd != InvalidHandle)
        PerfQueryDX9::IssueTimestampQuery(frame.perfQueryEnd);

    PerfQueryDX9::EndMeasurment();

    // update sync-objects
    _DX9_SyncObjectsSync.Lock();
    for (SyncObjectPoolDX9::Iterator s = SyncObjectPoolDX9::Begin(), s_end = SyncObjectPoolDX9::End(); s != s_end; ++s)
    {
        if (s->is_used && (frame_n - s->frame >= 2))
            s->is_signaled = true;
    }
    _DX9_SyncObjectsSync.Unlock();
}

bool _DX9_PresentBuffer()
{
    bool result = true;
    HRESULT hr = _D3D9_Device->Present(_DX9_PresentRectPtr, _DX9_PresentRectPtr, NULL, NULL);
    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICELOST)
        {
            result = false;
        }

        else if (hr == 0x88760872)
        {
            // ignore undocumented error
        }
        else
        {
            DAVA::Logger::Error("Present failed (%08X) : %s", hr, D3D9ErrorText(hr));
        }
    }

    return result;
}

void _DX9_ResetBlock()
{
    TextureDX9::ReleaseAll();
    VertexBufferDX9::ReleaseAll();
    IndexBufferDX9::ReleaseAll();
    PerfQueryDX9::ReleaseAll();
    QueryBufferDX9::ReleaseAll();

    bool resetNotified = false;
    for (;;)
    {
        HRESULT hr = _D3D9_Device->TestCooperativeLevel();
        if ((hr == D3DERR_DEVICENOTRESET) || SUCCEEDED(hr))
        {
            Logger::Info("[DX9 RESET] actually reseting device...");
            _DX9_ResetParamsMutex.Lock();
            D3DPRESENT_PARAMETERS param = _DX9_PresentParam;
            param.BackBufferFormat = (_DX9_PresentParam.Windowed) ? D3DFMT_UNKNOWN : D3DFMT_A8B8G8R8;
            hr = _D3D9_Device->Reset(&param);
            _DX9_ResetParamsMutex.Unlock();
            if (SUCCEEDED(hr))
            {
                break;
            }
            resetNotified = false;
            Logger::Error("[DX9 RESET] Failed to reset device (%08X) : %s", hr, D3D9ErrorText(hr));
        }
        else if (!resetNotified)
        {
            Logger::Error("[DX9 RESET] Can't reset now (%08X) : %s", hr, D3D9ErrorText(hr));
            resetNotified = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::Info("[DX9 RESET] reset succeeded ...");
#if !defined(DAVA_DISABLE_CLEAR_ON_RESET)
    //clear buffer
    DX9_CALL(_D3D9_Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 1), 1.0, 0), "Clear");
    _D3D9_Device->Present(_DX9_PresentRectPtr, _DX9_PresentRectPtr, NULL, NULL);
#endif

    _DX9_FramesWithRestoreAttempt = 0;

    TextureDX9::ReCreateAll();
    VertexBufferDX9::ReCreateAll();
    IndexBufferDX9::ReCreateAll();

    // update sync-objects, as pre-reset state is not actual anymore, also resolves constant reset causing already executed frame being never synced
    _DX9_SyncObjectsSync.Lock();
    for (SyncObjectPoolDX9::Iterator s = SyncObjectPoolDX9::Begin(), s_end = SyncObjectPoolDX9::End(); s != s_end; ++s)
    {
        if (s->is_used)
            s->is_signaled = true;
    }
    _DX9_SyncObjectsSync.Unlock();
}

static void _DX9_ExecImmediateCommand(CommonImpl::ImmediateCommand* command)
{
    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_EXECUTE_IMMEDIATE_CMDS);

#if 1
    #define CHECK_HR(hr) \
    if (FAILED(hr)) \
        Logger::Error("%s", D3D9ErrorText(hr));
#else
    CHECK_HR(hr)
#endif

    DX9Command* commandData = reinterpret_cast<DX9Command*>(command->cmdData);
    for (DX9Command *cmd = commandData, *cmdEnd = commandData + command->cmdCount; cmd != cmdEnd; ++cmd)
    {
        const uint64* arg = cmd->arg;

        Trace("exec %i\n", int(cmd->func));
        switch (cmd->func)
        {
        case DX9Command::NOP:
            break;

        case DX9Command::CREATE_VERTEX_BUFFER:
        {
            DVASSERT(*(IDirect3DVertexBuffer9**)(arg[4]) == nullptr);
            cmd->retval = _D3D9_Device->CreateVertexBuffer(UINT(arg[0]), DWORD(arg[1]), DWORD(arg[2]), D3DPOOL(arg[3]), (IDirect3DVertexBuffer9**)(arg[4]), (HANDLE*)(arg[5]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::LOCK_VERTEX_BUFFER:
        {
            cmd->retval = (*((IDirect3DVertexBuffer9**)arg[0]))->Lock(UINT(arg[1]), UINT(arg[2]), (VOID**)(arg[3]), DWORD(arg[4]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UNLOCK_VERTEX_BUFFER:
        {
            cmd->retval = (*((IDirect3DVertexBuffer9**)arg[0]))->Unlock();
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UPDATE_VERTEX_BUFFER:
        {
            IDirect3DVertexBuffer9* vb = *((IDirect3DVertexBuffer9**)arg[0]);

            if (vb)
            {
                unsigned sz = unsigned(arg[2]);
                void* dst = nullptr;
                void* src = (void*)(arg[1]);

                if (SUCCEEDED(vb->Lock(0, sz, &dst, 0)))
                {
                    memcpy(dst, src, sz);
                    cmd->retval = vb->Unlock();
                }

                CHECK_HR(cmd->retval);
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::CREATE_INDEX_BUFFER:
        {
            DVASSERT(*(IDirect3DIndexBuffer9**)(arg[4]) == nullptr);
            cmd->retval = _D3D9_Device->CreateIndexBuffer(UINT(arg[0]), DWORD(arg[1]), D3DFORMAT(arg[2]), D3DPOOL(arg[3]), (IDirect3DIndexBuffer9**)(arg[4]), (HANDLE*)(arg[5]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::LOCK_INDEX_BUFFER:
        {
            cmd->retval = (*((IDirect3DIndexBuffer9**)arg[0]))->Lock(UINT(arg[1]), UINT(arg[2]), (VOID**)(arg[3]), DWORD(arg[4]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UNLOCK_INDEX_BUFFER:
        {
            cmd->retval = (*((IDirect3DIndexBuffer9**)arg[0]))->Unlock();
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UPDATE_INDEX_BUFFER:
        {
            IDirect3DIndexBuffer9* ib = *((IDirect3DIndexBuffer9**)arg[0]);

            if (ib)
            {
                unsigned sz = unsigned(arg[2]);
                void* dst = nullptr;
                void* src = (void*)(arg[1]);

                if (SUCCEEDED(ib->Lock(0, sz, &dst, 0)))
                {
                    memcpy(dst, src, sz);
                    cmd->retval = ib->Unlock();
                }

                CHECK_HR(cmd->retval);
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::CREATE_TEXTURE:
        {
            DVASSERT(*(IDirect3DTexture9**)(arg[6]) == nullptr);
            cmd->retval = _D3D9_Device->CreateTexture(UINT(arg[0]), UINT(arg[1]), UINT(arg[2]), DWORD(arg[3]), D3DFORMAT(arg[4]), D3DPOOL(arg[5]), (IDirect3DTexture9**)(arg[6]), (HANDLE*)(arg[7]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::CREATE_CUBE_TEXTURE:
        {
            DVASSERT(*(IDirect3DCubeTexture9**)(arg[5]) == nullptr);
            cmd->retval = _D3D9_Device->CreateCubeTexture(UINT(arg[0]), UINT(arg[1]), DWORD(arg[2]), D3DFORMAT(arg[3]), D3DPOOL(arg[4]), (IDirect3DCubeTexture9**)(arg[5]), (HANDLE*)(arg[6]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::SET_TEXTURE_AUTOGEN_FILTER_TYPE:
        {
            cmd->retval = ((IDirect3DBaseTexture9*)(arg[0]))->SetAutoGenFilterType(D3DTEXTUREFILTERTYPE(arg[1]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::GET_TEXTURE_SURFACE_LEVEL:
        {
            IDirect3DTexture9* tex = *((IDirect3DTexture9**)(arg[0]));
            DVASSERT(*(IDirect3DSurface9**)(arg[2]) == nullptr);
            cmd->retval = tex->GetSurfaceLevel(UINT(arg[1]), (IDirect3DSurface9**)(arg[2]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::GET_CUBE_SURFACE_LEVEL:
        {
            IDirect3DCubeTexture9* tex = *((IDirect3DCubeTexture9**)(arg[0]));
            DVASSERT(*(IDirect3DSurface9**)(arg[3]) == nullptr);
            cmd->retval = tex->GetCubeMapSurface(D3DCUBEMAP_FACES(arg[1]), UINT(arg[2]), (IDirect3DSurface9**)(arg[3]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::LOCK_TEXTURE_RECT:
        {
            IDirect3DTexture9* tex = *((IDirect3DTexture9**)(arg[0]));

            cmd->retval = tex->LockRect(UINT(arg[1]), (D3DLOCKED_RECT*)(arg[2]), (const RECT*)(arg[3]), DWORD(arg[4]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UNLOCK_TEXTURE_RECT:
        {
            IDirect3DTexture9* tex = *((IDirect3DTexture9**)(arg[0]));

            cmd->retval = tex->UnlockRect(UINT(arg[1]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UPDATE_TEXTURE_LEVEL:
        {
            IDirect3DTexture9* tex = *((IDirect3DTexture9**)(arg[0]));

            if (tex)
            {
                UINT lev = UINT(arg[1]);
                void* src = (void*)(arg[2]);
                unsigned sz = unsigned(arg[3]);
                rhi::TextureFormat format = static_cast<rhi::TextureFormat>(arg[4]);
                D3DLOCKED_RECT rc = {};
                HRESULT hr = tex->LockRect(lev, &rc, NULL, 0);

                if (SUCCEEDED(hr))
                {
                    if (format == TEXTURE_FORMAT_R8G8B8A8)
                        _SwapRB8(src, rc.pBits, sz);
                    else if (format == TEXTURE_FORMAT_R4G4B4A4)
                        _SwapRB4(src, rc.pBits, sz);
                    else if (format == TEXTURE_FORMAT_R5G5B5A1)
                        _SwapRB5551(src, rc.pBits, sz);
                    else
                        memcpy(rc.pBits, src, sz);

                    cmd->retval = tex->UnlockRect(lev);
                }
                else
                {
                    CHECK_HR(hr);
                    cmd->retval = hr;
                }
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::READ_TEXTURE_LEVEL:
        {
            IDirect3DTexture9* tex = *((IDirect3DTexture9**)(arg[0]));

            if (tex)
            {
                UINT lev = UINT(arg[1]);
                unsigned sz = unsigned(arg[2]);
                rhi::TextureFormat format = static_cast<rhi::TextureFormat>(arg[3]);
                void* dst = (void*)(arg[4]);
                D3DLOCKED_RECT rc = {};
                HRESULT hr = tex->LockRect(lev, &rc, NULL, 0);

                if (SUCCEEDED(hr))
                {
                    if (format == TEXTURE_FORMAT_R8G8B8A8)
                        _SwapRB8(rc.pBits, dst, sz);
                    else if (format == TEXTURE_FORMAT_R4G4B4A4)
                        _SwapRB4(rc.pBits, dst, sz);
                    else if (format == TEXTURE_FORMAT_R5G5B5A1)
                        _SwapRB5551(rc.pBits, dst, sz);
                    else
                        memcpy(dst, rc.pBits, sz);

                    cmd->retval = tex->UnlockRect(lev);
                }
                else
                {
                    CHECK_HR(hr);
                    cmd->retval = hr;
                }
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::LOCK_CUBETEXTURE_RECT:
        {
            IDirect3DCubeTexture9* tex = *((IDirect3DCubeTexture9**)(arg[0]));

            cmd->retval = tex->LockRect(D3DCUBEMAP_FACES(arg[1]), UINT(arg[2]), (D3DLOCKED_RECT*)(arg[3]), (const RECT*)(arg[4]), DWORD(arg[5]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UNLOCK_CUBETEXTURE_RECT:
        {
            IDirect3DCubeTexture9* tex = *((IDirect3DCubeTexture9**)(arg[0]));

            cmd->retval = tex->UnlockRect(D3DCUBEMAP_FACES(arg[1]), UINT(arg[2]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::UPDATE_CUBETEXTURE_LEVEL:
        {
            IDirect3DCubeTexture9* tex = *((IDirect3DCubeTexture9**)(arg[0]));

            if (tex)
            {
                UINT lev = UINT(arg[1]);
                D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)(arg[2]);
                void* src = (void*)(arg[3]);
                unsigned sz = unsigned(arg[4]);
                rhi::TextureFormat format = static_cast<rhi::TextureFormat>(arg[5]);
                D3DLOCKED_RECT rc = { 0 };
                HRESULT hr = tex->LockRect(face, lev, &rc, NULL, 0);

                if (SUCCEEDED(hr))
                {
                    if (format == TEXTURE_FORMAT_R8G8B8A8)
                        _SwapRB8(src, rc.pBits, sz);
                    else if (format == TEXTURE_FORMAT_R4G4B4A4)
                        _SwapRB4(src, rc.pBits, sz);
                    else if (format == TEXTURE_FORMAT_R5G5B5A1)
                        _SwapRB5551(src, rc.pBits, sz);
                    else
                        memcpy(rc.pBits, src, sz);

                    cmd->retval = tex->UnlockRect(face, lev);
                }
                else
                {
                    CHECK_HR(hr);
                    cmd->retval = hr;
                }
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::READ_CUBETEXTURE_LEVEL:
        {
            IDirect3DCubeTexture9* tex = *((IDirect3DCubeTexture9**)(arg[0]));

            if (tex)
            {
                UINT lev = UINT(arg[1]);
                D3DCUBEMAP_FACES face = (D3DCUBEMAP_FACES)(arg[2]);
                unsigned sz = unsigned(arg[3]);
                rhi::TextureFormat format = static_cast<rhi::TextureFormat>(arg[4]);
                void* dst = (void*)(arg[5]);
                D3DLOCKED_RECT rc = {};
                HRESULT hr = tex->LockRect(face, lev, &rc, NULL, 0);

                if (SUCCEEDED(hr))
                {
                    if (format == TEXTURE_FORMAT_R8G8B8A8)
                        _SwapRB8(rc.pBits, dst, sz);
                    else if (format == TEXTURE_FORMAT_R4G4B4A4)
                        _SwapRB4(rc.pBits, dst, sz);
                    else if (format == TEXTURE_FORMAT_R5G5B5A1)
                        _SwapRB5551(rc.pBits, dst, sz);
                    else
                        memcpy(dst, rc.pBits, sz);

                    cmd->retval = tex->UnlockRect(face, lev);
                }
                else
                {
                    CHECK_HR(hr);
                    cmd->retval = hr;
                }
            }
            else
            {
                cmd->retval = E_FAIL;
            }
        }
        break;

        case DX9Command::GET_RENDERTARGET_DATA:
        {
            cmd->retval = _D3D9_Device->GetRenderTargetData(*(IDirect3DSurface9**)arg[0], *(IDirect3DSurface9**)arg[1]);
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::CREATE_VERTEX_SHADER:
        {
            cmd->retval = _D3D9_Device->CreateVertexShader((const DWORD*)(arg[0]), (IDirect3DVertexShader9**)(arg[1]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::CREATE_VERTEX_DECLARATION:
        {
            cmd->retval = _D3D9_Device->CreateVertexDeclaration((const D3DVERTEXELEMENT9*)(arg[0]), (IDirect3DVertexDeclaration9**)(arg[1]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::CREATE_PIXEL_SHADER:
        {
            cmd->retval = _D3D9_Device->CreatePixelShader((const DWORD*)(arg[0]), (IDirect3DPixelShader9**)(arg[1]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::GET_QUERY_DATA:
        {
            cmd->retval = ((IDirect3DQuery9*)(arg[0]))->GetData((void*)(arg[1]), DWORD(arg[2]), DWORD(arg[3]));
            CHECK_HR(cmd->retval);
        }
        break;

        case DX9Command::QUERY_INTERFACE:
        {
            IUnknown* ptr = *(IUnknown**)(arg[0]);
            cmd->retval = ptr->QueryInterface(*((const GUID*)(arg[1])), (void**)(arg[2]));
        }
        break;

        case DX9Command::RELEASE:
        {
            IUnknown* ptr = *(reinterpret_cast<IUnknown**>(arg[0]));
            cmd->retval = ptr->Release();
        }
        break;

        case DX9Command::CREATE_RENDER_TARGET:
        {
            DX9_CALL(_D3D9_Device->CreateRenderTarget((UINT)arg[0], (UINT)arg[1], static_cast<D3DFORMAT>(arg[2]),
                                                      static_cast<D3DMULTISAMPLE_TYPE>(arg[3]), (DWORD)arg[4], (BOOL)(arg[5] != 0),
                                                      (IDirect3DSurface9**)(arg[6]), (HANDLE*)(arg[7])),
                     "CreateRenderTarget");
        }
        break;

        case DX9Command::CREARE_DEPTHSTENCIL_SURFACE:
        {
            DX9_CALL(_D3D9_Device->CreateDepthStencilSurface((UINT)arg[0], (UINT)arg[1], static_cast<D3DFORMAT>(arg[2]),
                                                             static_cast<D3DMULTISAMPLE_TYPE>(arg[3]), (DWORD)arg[4], BOOL(arg[5] != 0),
                                                             (IDirect3DSurface9**)(arg[6]), (HANDLE*)(arg[7])),
                     "CreateDepthStencilSurface");
        }
        break;

        case DX9Command::SYNC_CPU_GPU:
        {
            if (DeviceCaps().isPerfQuerySupported)
            {
                IDirect3DQuery9 *disjointQuery = nullptr, *freqQuery = nullptr, *tsQuery = nullptr;

                _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &disjointQuery);
                _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &freqQuery);
                _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &tsQuery);

                if (disjointQuery && freqQuery && tsQuery)
                {
                    disjointQuery->Issue(D3DISSUE_BEGIN);
                    freqQuery->Issue(D3DISSUE_END);
                    tsQuery->Issue(D3DISSUE_END);
                    disjointQuery->Issue(D3DISSUE_END);

                    bool disjoint = true;
                    uint64 frequency = 0, timestamp = 0;

                    while (S_FALSE == tsQuery->GetData(&timestamp, sizeof(uint64), D3DGETDATA_FLUSH))
                    {
                    };
                    if (timestamp)
                    {
                        *reinterpret_cast<uint64*>(arg[0]) = DAVA::SystemTimer::GetUs();

                        while (S_FALSE == disjointQuery->GetData(&disjoint, sizeof(bool), D3DGETDATA_FLUSH))
                        {
                        };
                        while (S_FALSE == freqQuery->GetData(&frequency, sizeof(uint64), D3DGETDATA_FLUSH))
                        {
                        };

                        if (!disjoint && frequency)
                        {
                            *reinterpret_cast<uint64*>(arg[1]) = timestamp / (frequency / 1000000); //mcs
                        }
                    }
                }

                if (disjointQuery)
                    disjointQuery->Release();

                if (freqQuery)
                    freqQuery->Release();

                if (tsQuery)
                    tsQuery->Release();
            }
        }
        break;

        default:
            DVASSERT(!"unknown DX-cmd");
        }
    }

    #undef CHECK_HR
}

//------------------------------------------------------------------------------

void ExecDX9(DX9Command* command, uint32 cmdCount, bool forceExecute)
{
    CommonImpl::ImmediateCommand cmd;
    cmd.cmdData = command;
    cmd.cmdCount = cmdCount;
    cmd.forceExecute = forceExecute;
    RenderLoop::IssueImmediateCommand(&cmd);
}

namespace CommandBufferDX9
{
void Init(uint32 maxCount)
{
    CommandBufferPoolDX9::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_CommandBuffer_Begin = &dx9_CommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = &dx9_CommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = &dx9_CommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = &dx9_CommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = &dx9_CommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = &dx9_CommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = &dx9_CommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = &dx9_CommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = &dx9_CommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = &dx9_CommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = &dx9_CommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryBuffer = &dx9_CommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_SetQueryIndex = &dx9_CommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_IssueTimestampQuery = &dx9_CommandBuffer_IssueTimestampQuery;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = &dx9_CommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = &dx9_CommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = &dx9_CommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = &dx9_CommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = &dx9_CommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = &dx9_CommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = &dx9_CommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = &dx9_CommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = &dx9_CommandBuffer_SetMarker;

    dispatch->impl_SyncObject_Create = &dx9_SyncObject_Create;
    dispatch->impl_SyncObject_Delete = &dx9_SyncObject_Delete;
    dispatch->impl_SyncObject_IsSignaled = &dx9_SyncObject_IsSignaled;

    dispatch->impl_ProcessImmediateCommand = _DX9_ExecImmediateCommand;
    dispatch->impl_ExecuteFrame = _DX9_ExecuteQueuedCommands;
    dispatch->impl_RejectFrame = _DX9_RejectFrame;
    dispatch->impl_PresentBuffer = _DX9_PresentBuffer;
    dispatch->impl_ResetBlock = _DX9_ResetBlock;
    dispatch->impl_FinishFrame = _DX9_InvalidateFrameCache;
}
}

//==============================================================================
} // namespace rhi
