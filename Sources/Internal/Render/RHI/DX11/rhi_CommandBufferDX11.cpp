#include "rhi_DX11.h"
#include "Platform/DeviceInfo.h"
#include "../Common/dbg_StatSet.h"
#include "../Common/SoftwareCommandBuffer.h"
#include "../Common/RenderLoop.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Time/SystemTimer.h"
#include <wrl/client.h>

/*
 * Structure of this file:
 - CommandBufferDX11_t methods
 - Software command buffer implemetation
 - Hardware command buffer implemetation
 - RHI methods implementation
 - Render pass implementation
 - Sync object implementation
 */

namespace rhi
{
using namespace Microsoft::WRL;

#define RHI_DX11_CB_COMPILE_WITH_HARDWARE 1
#define RHI_DX11_CB_COMPILE_WITH_SOFTWARE 1

struct CommandBufferDX11_t : public SoftwareCommandBuffer
{
public:
    struct RasterizerParamDX11_t
    {
        uint32 cullMode : 3;
        uint32 scissorEnabled : 1;
        uint32 wireframe : 1;
        uint32 pad : 27;
        bool operator==(const RasterizerParamDX11_t& b) const;
    };
    struct RasterizerStateDX11_t
    {
        RasterizerParamDX11_t param;
        ID3D11RasterizerState* state = nullptr;
        RasterizerStateDX11_t(const RasterizerParamDX11_t& p, ID3D11RasterizerState* st);
        bool operator==(const RasterizerParamDX11_t& p) const;
    };

public:
    RenderPassConfig passCfg;
    ID3D11RasterizerState* cur_rs = nullptr;
    ID3D11RasterizerState* last_rs = nullptr;
    RasterizerParamDX11_t rs_param;
    D3D11_PRIMITIVE_TOPOLOGY cur_topo = D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D11_VIEWPORT def_viewport;
    Handle cur_pipelinestate = InvalidHandle;
    Handle cur_query_buf = InvalidHandle;
    Handle sync = InvalidHandle;
    Handle cur_vb[MAX_VERTEX_STREAM_COUNT];
    uint32 cur_vb_stride[MAX_VERTEX_STREAM_COUNT];
    uint32 cur_stream_count;

    bool isComplete = false;
    bool isFirstInPass = false;
    bool isLastInPass = false;

    void Begin(ID3D11DeviceContext* context);
    void Reset();
    void ApplyTopology(PrimitiveType primType, uint32 primCount, uint32* indexCount);
    ID3D11RasterizerState* GetRasterizerState(const RasterizerParamDX11_t& param);

#if (RHI_DX11_CB_COMPILE_WITH_SOFTWARE)
    void ExecuteSoftware();
#endif

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    ID3D11DeviceContext* context = nullptr;
    ID3DUserDefinedAnnotation* contextAnnotation = nullptr;
    ID3D11CommandList* commandList = nullptr;
    ID3D11Buffer* vertexConstBuffer[MAX_CONST_BUFFER_COUNT];
    ID3D11Buffer* fragmentConstBuffer[MAX_CONST_BUFFER_COUNT];
    DAVA::Vector<Handle> deferredPerfQueries;
    Handle last_ps = InvalidHandle;
    Handle last_vb[MAX_VERTEX_STREAM_COUNT];
    uint32 last_vb_stride[MAX_VERTEX_STREAM_COUNT];
    uint32 last_vdecl = 0;

    void InitHardware();
    void ReleaseHardware(bool isComplete);
    void ResetHardware();
    void ExecuteHardware();
    void ApplyVertexData();
    void ApplyRasterizerState();
    void ApplyConstBuffers();
#endif
};
using CommandBufferPoolDX11 = ResourcePool<CommandBufferDX11_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false>;
RHI_IMPL_POOL(CommandBufferDX11_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false);

struct RenderPassDX11_t
{
    DAVA::Vector<Handle> commandBuffers;
    Handle perfQueryStart = InvalidHandle;
    Handle perfQueryEnd = InvalidHandle;
    int priority = 0;
};
using RenderPassPoolDX11 = ResourcePool<RenderPassDX11_t, RESOURCE_RENDER_PASS, RenderPassConfig, false>;
RHI_IMPL_POOL(RenderPassDX11_t, RESOURCE_RENDER_PASS, RenderPassConfig, false);

struct SyncObjectDX11_t
{
    uint32 frame = 0;
    uint32 is_signaled : 1;
    uint32 is_used : 1;
    uint32 pad : 30;
};
using SyncObjectPoolDX11 = ResourcePool<SyncObjectDX11_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false>;
RHI_IMPL_POOL(SyncObjectDX11_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false);

static DAVA::Mutex _DX11_SyncObjectsSync;
static DAVA::Mutex _DX11_PendingSecondaryCmdListSync;
static DAVA::Vector<ComPtr<ID3D11CommandList>> _DX11_PendingSecondaryCmdLists;

/*
 * CommandBufferDX11_t methods
 */
void CommandBufferDX11_t::Reset()
{
    isComplete = false;
    cur_topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    cur_rs = nullptr;
    cur_pipelinestate = InvalidHandle;
    cur_query_buf = InvalidHandle;
    rs_param.cullMode = CULL_NONE;
    rs_param.scissorEnabled = false;
    rs_param.wireframe = false;
    last_rs = nullptr;
    for (uint32 i = 0; i != MAX_VERTEX_STREAM_COUNT; ++i)
    {
        cur_vb[i] = InvalidHandle;
        cur_vb_stride[i] = 0;
    }
#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    if (dx11.useHardwareCommandBuffers)
        ResetHardware();
#endif
}

void CommandBufferDX11_t::ApplyTopology(PrimitiveType primType, uint32 primCount, uint32* indexCount)
{
    D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    switch (primType)
    {
    case PRIMITIVE_TRIANGLELIST:
        *indexCount = primCount * 3;
        topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        *indexCount = 2 + primCount;
        topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;

    case PRIMITIVE_LINELIST:
        *indexCount = primCount * 2;
        topo = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        break;
    }

    if (topo != cur_topo)
    {
    #if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
        if (dx11.useHardwareCommandBuffers)
        {
            context->IASetPrimitiveTopology(topo);
        }
    #endif
        cur_topo = topo;
    }
}

ID3D11RasterizerState* CommandBufferDX11_t::GetRasterizerState(const RasterizerParamDX11_t& param)
{
    static std::vector<RasterizerStateDX11_t> cache;

    auto existing = std::find(cache.begin(), cache.end(), param);
    if (existing != cache.end())
        return existing->state;

    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = (param.wireframe) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = param.scissorEnabled;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    switch (CullMode(param.cullMode))
    {
    case CULL_NONE:
        desc.CullMode = D3D11_CULL_NONE;
        break;
    case CULL_CCW:
        desc.CullMode = D3D11_CULL_BACK;
        break;
    case CULL_CW:
        desc.CullMode = D3D11_CULL_FRONT;
        break;
    default:
        DVASSERT(0, "Invalid CullMode provided");
    }

    ID3D11RasterizerState* state = nullptr;
    bool commandExecuted = DX11DeviceCommand(DX11Command::CREATE_RASTERIZER_STATE, &desc, &state);
    if (commandExecuted && (state != nullptr))
        cache.emplace_back(param, state);

    return state;
}

void CommandBufferDX11_t::Begin(ID3D11DeviceContext* inContext)
{
    bool clear_color = isFirstInPass && passCfg.colorBuffer[0].loadAction == LOADACTION_CLEAR;
    bool clear_depth = isFirstInPass && passCfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR;

    sync = InvalidHandle;

    def_viewport.TopLeftX = 0;
    def_viewport.TopLeftY = 0;
    def_viewport.MinDepth = 0.0f;
    def_viewport.MaxDepth = 1.0f;

    ID3D11RenderTargetView* rt_view[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    unsigned rt_count = 0;
    ID3D11DepthStencilView* ds_view = NULL;

    if (passCfg.depthStencilBuffer.texture == rhi::DefaultDepthBuffer)
    {
        ds_view = dx11.depthStencilView.Get();
    }
    else if (passCfg.depthStencilBuffer.texture != rhi::InvalidHandle)
    {
        TextureDX11::SetDepthStencil(passCfg.depthStencilBuffer.texture, &ds_view);
    }

    if (passCfg.UsingMSAA())
    {
        DVASSERT(passCfg.depthStencilBuffer.multisampleTexture != InvalidHandle);
        TextureDX11::SetDepthStencil(passCfg.depthStencilBuffer.multisampleTexture, &ds_view);
    }

    for (unsigned i = 0; i != countof(passCfg.colorBuffer); ++i)
    {
        if (passCfg.colorBuffer[i].texture != rhi::InvalidHandle)
        {
            if (passCfg.UsingMSAA())
                TextureDX11::SetRenderTarget(passCfg.colorBuffer[i].multisampleTexture, passCfg.colorBuffer[i].textureLevel, passCfg.colorBuffer[i].textureFace, inContext, rt_view + i);
            else
                TextureDX11::SetRenderTarget(passCfg.colorBuffer[i].texture, passCfg.colorBuffer[i].textureLevel, passCfg.colorBuffer[i].textureFace, inContext, rt_view + i);

            ++rt_count;
        }
        else
        {
            if (i == 0)
            {
                if (passCfg.UsingMSAA())
                    TextureDX11::SetRenderTarget(passCfg.colorBuffer[i].multisampleTexture, passCfg.colorBuffer[i].textureLevel, passCfg.colorBuffer[i].textureFace, inContext, rt_view + i);
                else
                    rt_view[0] = dx11.renderTargetView.Get();

                rt_count = 1;
            }

            break;
        }
    }

    inContext->OMSetRenderTargets(rt_count, rt_view, ds_view);

    if (passCfg.colorBuffer[0].texture != rhi::InvalidHandle)
    {
        Size2i sz = (passCfg.UsingMSAA()) ? TextureDX11::Size(passCfg.colorBuffer[0].multisampleTexture) : TextureDX11::Size(passCfg.colorBuffer[0].texture);

        def_viewport.Width = static_cast<float>(sz.dx);
        def_viewport.Height = static_cast<float>(sz.dy);
    }

    inContext->OMGetRenderTargets(countof(rt_view), rt_view, &ds_view);

    for (unsigned i = 0; i != countof(rt_view); ++i)
    {
        if (rt_view[i])
        {
            if (i == 0)
            {
                if (passCfg.colorBuffer[0].texture == rhi::InvalidHandle)
                {
                    D3D11_TEXTURE2D_DESC desc;

                    dx11.renderTarget->GetDesc(&desc);

                    def_viewport.Width = float(desc.Width);
                    def_viewport.Height = float(desc.Height);
                }

                inContext->RSSetViewports(1, &(def_viewport));
            }

            if (clear_color)
                inContext->ClearRenderTargetView(rt_view[i], passCfg.colorBuffer[i].clearColor);

            rt_view[i]->Release();
        }
    }

    if (ds_view)
    {
        if (clear_depth)
            inContext->ClearDepthStencilView(ds_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, passCfg.depthStencilBuffer.clearDepth, passCfg.depthStencilBuffer.clearStencil);

        ds_view->Release();
    }

    inContext->IASetPrimitiveTopology(cur_topo);

    DVASSERT(!isFirstInPass || cur_query_buf == InvalidHandle || !QueryBufferDX11::QueryIsCompleted(cur_query_buf));
}

/*
 * Inner classes implementation
 */
bool CommandBufferDX11_t::RasterizerParamDX11_t::operator==(const CommandBufferDX11_t::RasterizerParamDX11_t& b) const
{
    return (cullMode == b.cullMode) && (scissorEnabled == b.scissorEnabled) && (wireframe == b.wireframe);
}

CommandBufferDX11_t::RasterizerStateDX11_t::RasterizerStateDX11_t(const CommandBufferDX11_t::RasterizerParamDX11_t& p, ID3D11RasterizerState* st)
    : param(p)
    , state(st)
{
}

bool CommandBufferDX11_t::RasterizerStateDX11_t::operator==(const RasterizerParamDX11_t& p) const
{
    return param == p;
}

/*================================================================================
 *
 * Software command buffer implemetation
 *
 *================================================================================*
 * Implementation for hardware command buffers could be found below
 */

#if (RHI_DX11_CB_COMPILE_WITH_SOFTWARE)
static void dx11_SWCommandBuffer_Begin(Handle cmdBuf)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->curUsedSize = 0;
    cb->allocCmd<SWCommand_Begin>();
}

static void dx11_SWCommandBuffer_End(Handle cmdBuf, Handle syncObject)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_End* cmd = cb->allocCmd<SWCommand_End>();
    cmd->syncObject = syncObject;
}

static void dx11_SWCommandBuffer_SetPipelineState(Handle cmdBuf, Handle ps, uint32 vdeclUID)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetPipelineState* cmd = cb->allocCmd<SWCommand_SetPipelineState>();
    cmd->ps = ps;
    cmd->vdecl = vdeclUID;
}

static void dx11_SWCommandBuffer_SetCullMode(Handle cmdBuf, CullMode mode)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetCullMode* cmd = cb->allocCmd<SWCommand_SetCullMode>();
    cmd->mode = mode;
}

void dx11_SWCommandBuffer_SetScissorRect(Handle cmdBuf, ScissorRect rect)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetScissorRect* cmd = cb->allocCmd<SWCommand_SetScissorRect>();
    cmd->x = rect.x;
    cmd->y = rect.y;
    cmd->width = rect.width;
    cmd->height = rect.height;
}

static void dx11_SWCommandBuffer_SetViewport(Handle cmdBuf, Viewport vp)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetViewport* cmd = cb->allocCmd<SWCommand_SetViewport>();
    cmd->x = vp.x;
    cmd->y = vp.y;
    cmd->width = vp.width;
    cmd->height = vp.height;
}

static void dx11_SWCommandBuffer_SetFillMode(Handle cmdBuf, FillMode mode)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetFillMode* cmd = cb->allocCmd<SWCommand_SetFillMode>();
    cmd->mode = mode;
}

static void dx11_SWCommandBuffer_SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetVertexData* cmd = cb->allocCmd<SWCommand_SetVertexData>();
    cmd->vb = vb;
    cmd->streamIndex = streamIndex;
}

static void dx11_SWCommandBuffer_SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetVertexProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetVertexProgConstBuffer>();
    cmd->bufIndex = bufIndex;
    cmd->buffer = buffer;
    cmd->inst = ConstBufferDX11::Instance(buffer);
}

static void dx11_SWCommandBuffer_SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetVertexTexture* cmd = cb->allocCmd<SWCommand_SetVertexTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
}

static void dx11_SWCommandBuffer_SetIndices(Handle cmdBuf, Handle ib)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetIndices* cmd = cb->allocCmd<SWCommand_SetIndices>();
    cmd->ib = ib;
}

static void dx11_SWCommandBuffer_SetQueryIndex(Handle cmdBuf, uint32 objectIndex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetQueryIndex* cmd = cb->allocCmd<SWCommand_SetQueryIndex>();
    cmd->objectIndex = objectIndex;
}

static void dx11_SWCommandBuffer_SetQueryBuffer(Handle cmdBuf, Handle queryBuf)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    DVASSERT(cb->cur_query_buf == InvalidHandle);
    SWCommand_SetQueryBuffer* cmd = cb->allocCmd<SWCommand_SetQueryBuffer>();
    cmd->queryBuf = queryBuf;
}

static void dx11_SWCommandBuffer_IssueTimestampQuery(Handle cmdBuf, Handle perfQuery)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_IssueTimestamptQuery* cmd = cb->allocCmd<SWCommand_IssueTimestamptQuery>();
    cmd->perfQuery = perfQuery;
}

static void dx11_SWCommandBuffer_SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetFragmentProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetFragmentProgConstBuffer>();
    cmd->bufIndex = bufIndex;
    cmd->buffer = buffer;
    cmd->inst = ConstBufferDX11::Instance(buffer);
}

static void dx11_SWCommandBuffer_SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetFragmentTexture* cmd = cb->allocCmd<SWCommand_SetFragmentTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
}

static void dx11_SWCommandBuffer_SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetDepthStencilState* cmd = cb->allocCmd<SWCommand_SetDepthStencilState>();
    cmd->depthStencilState = depthStencilState;
}

static void dx11_SWCommandBuffer_SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SWCommand_SetSamplerState* cmd = cb->allocCmd<SWCommand_SetSamplerState>();
    cmd->samplerState = samplerState;
}

static void dx11_SWCommandBuffer_DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    uint32 vertexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &vertexCount);

    SWCommand_DrawPrimitive* cmd = cb->allocCmd<SWCommand_DrawPrimitive>();
    cmd->mode = cb->cur_topo;
    cmd->vertexCount = vertexCount;
}

static void dx11_SWCommandBuffer_DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count,
                                                      uint32 vertexCount, uint32 firstVertex, uint32 startIndex)
{
    uint32 indexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &indexCount);

    SWCommand_DrawIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawIndexedPrimitive>();
    cmd->mode = cb->cur_topo;
    cmd->firstVertex = firstVertex;
    cmd->indexCount = indexCount;
    cmd->startIndex = startIndex;
}

static void dx11_SWCommandBuffer_DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count)
{
    INT baseVertex = 0;
    uint32 vertexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &vertexCount);

    SWCommand_DrawInstancedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedPrimitive>();
    cmd->mode = cb->cur_topo;
    cmd->instanceCount = instCount;
    cmd->vertexCount = vertexCount;
}

static void dx11_SWCommandBuffer_DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount,
                                                               uint32 count, uint32 vertexCount, uint32 firstVertex,
                                                               uint32 startIndex, uint32 baseInstance)
{
    uint32 indexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &indexCount);

    SWCommand_DrawInstancedIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedIndexedPrimitive>();
    cmd->mode = cb->cur_topo;
    cmd->indexCount = indexCount;
    cmd->instanceCount = instCount;
    cmd->firstVertex = firstVertex;
    cmd->instanceCount = instCount;
    cmd->baseInstance = baseInstance;
    cmd->startIndex = startIndex;
}

static void dx11_SWCommandBuffer_SetMarker(Handle cmdBuf, const char* text)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    if (cb->text == nullptr)
    {
        cb->text = new RingBuffer();
        cb->text->Initialize(64 * 1024);
    }

    size_t len = strlen(text);
    char* txt = reinterpret_cast<char*>(cb->text->Alloc(static_cast<uint32_t>(len / sizeof(float) + 1)));
    memcpy(txt, text, len);
    txt[len] = '\0';

    SWCommand_SetMarker* cmd = cb->allocCmd<SWCommand_SetMarker>();
    cmd->text = text;
}

void CommandBufferDX11_t::ExecuteSoftware()
{
    DVASSERT(dx11.useHardwareCommandBuffers == false);

    const uint32 immediateCommandCheckInterval = 10;
    uint32 commandsExecuted = 0;

    for (const uint8 *c = cmdData, *c_end = cmdData + curUsedSize; c != c_end;)
    {
        const SWCommand* cmd = reinterpret_cast<const SWCommand*>(c);

        switch (SoftwareCommandType(cmd->type))
        {
        case CMD_BEGIN:
        {
            Reset();
            Begin(dx11.ImmediateContext());
        }
        break;

        case CMD_END:
        {
            sync = static_cast<const SWCommand_End*>(cmd)->syncObject;
            isComplete = true;
        }
        break;

        case CMD_SET_VERTEX_DATA:
        {
            Handle vb = static_cast<const SWCommand_SetVertexData*>(cmd)->vb;
            uint32 stream_i = static_cast<const SWCommand_SetVertexData*>(cmd)->streamIndex;

            cur_vb[stream_i] = vb;
            if (cur_vb_stride[stream_i] == 0)
                cur_vb_stride[stream_i] = PipelineStateDX11::VertexLayoutStride(cur_pipelinestate, stream_i);
        }
        break;

        case CMD_SET_INDICES:
        {
            Handle ib = static_cast<const SWCommand_SetIndices*>(cmd)->ib;
            IndexBufferDX11::SetToRHI(ib, 0, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_QUERY_BUFFER:
        {
            cur_query_buf = static_cast<const SWCommand_SetQueryBuffer*>(cmd)->queryBuf;
        }
        break;

        case CMD_SET_QUERY_INDEX:
        {
            if (cur_query_buf != InvalidHandle)
            {
                QueryBufferDX11::SetQueryIndex(cur_query_buf, static_cast<const SWCommand_SetQueryIndex*>(cmd)->objectIndex, dx11.ImmediateContext());
            }
        }
        break;

        case CMD_ISSUE_TIMESTAMP_QUERY:
        {
            Handle perfQuery = ((SWCommand_IssueTimestamptQuery*)cmd)->perfQuery;
            PerfQueryDX11::IssueTimestampQuery(perfQuery, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_PIPELINE_STATE:
        {
            Handle ps = static_cast<const SWCommand_SetPipelineState*>(cmd)->ps;
            Handle vdeclUID = static_cast<const SWCommand_SetPipelineState*>(cmd)->vdecl;
            const VertexLayout* vdecl = (vdeclUID == VertexLayout::InvalidUID) ? nullptr : VertexLayout::Get(vdeclUID);

            cur_pipelinestate = ps;
            cur_stream_count = PipelineStateDX11::VertexLayoutStreamCount(ps);
            for (uint32 i = 0; i != cur_stream_count; ++i)
                cur_vb_stride[i] = (vdecl) ? vdecl->Stride(i) : 0;

            PipelineStateDX11::SetToRHI(ps, vdeclUID, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_CULL_MODE:
        {
            rs_param.cullMode = CullMode(static_cast<const SWCommand_SetCullMode*>(cmd)->mode);
            cur_rs = nullptr;
        }
        break;

        case CMD_SET_SCISSOR_RECT:
        {
            int x = static_cast<const SWCommand_SetScissorRect*>(cmd)->x;
            int y = static_cast<const SWCommand_SetScissorRect*>(cmd)->y;
            int w = static_cast<const SWCommand_SetScissorRect*>(cmd)->width;
            int h = static_cast<const SWCommand_SetScissorRect*>(cmd)->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                D3D11_RECT rect = { x, y, x + w - 1, y + h - 1 };

                rs_param.scissorEnabled = true;
                cur_rs = nullptr;

                dx11.context->RSSetScissorRects(1, &rect);
            }
            else
            {
                rs_param.scissorEnabled = false;
                cur_rs = nullptr;
            }
        }
        break;

        case CMD_SET_VIEWPORT:
        {
            int x = static_cast<const SWCommand_SetViewport*>(cmd)->x;
            int y = static_cast<const SWCommand_SetViewport*>(cmd)->y;
            int w = static_cast<const SWCommand_SetViewport*>(cmd)->width;
            int h = static_cast<const SWCommand_SetViewport*>(cmd)->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                D3D11_VIEWPORT vp;

                vp.TopLeftX = float(x);
                vp.TopLeftY = float(y);
                vp.Width = float(w);
                vp.Height = float(h);
                vp.MinDepth = 0.0f;
                vp.MaxDepth = 1.0f;

                dx11.context->RSSetViewports(1, &vp);
            }
            else
            {
                dx11.context->RSSetViewports(1, &def_viewport);
            }
        }
        break;

        case CMD_SET_FILLMODE:
        {
            rs_param.wireframe = FillMode(static_cast<const SWCommand_SetFillMode*>(cmd)->mode) == FILLMODE_WIREFRAME;
        }
        break;

        case CMD_SET_VERTEX_PROG_CONST_BUFFER:
        {
            Handle buffer = static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd)->buffer;
            const void* inst = static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd)->inst;

            ConstBufferDX11::SetToRHI(buffer, inst);
        }
        break;

        case CMD_SET_FRAGMENT_PROG_CONST_BUFFER:
        {
            Handle buffer = static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd)->buffer;
            const void* inst = static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd)->inst;

            ConstBufferDX11::SetToRHI(buffer, inst);
        }
        break;

        case CMD_SET_FRAGMENT_TEXTURE:
        {
            Handle tex = static_cast<const SWCommand_SetFragmentTexture*>(cmd)->tex;
            uint32 unitIndex = static_cast<const SWCommand_SetFragmentTexture*>(cmd)->unitIndex;
            TextureDX11::SetToRHIFragment(tex, unitIndex, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_VERTEX_TEXTURE:
        {
            Handle tex = static_cast<const SWCommand_SetVertexTexture*>(cmd)->tex;
            uint32 unitIndex = static_cast<const SWCommand_SetVertexTexture*>(cmd)->unitIndex;
            TextureDX11::SetToRHIVertex(tex, unitIndex, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_DEPTHSTENCIL_STATE:
        {
            DepthStencilStateDX11::SetToRHI(static_cast<const SWCommand_SetDepthStencilState*>(cmd)->depthStencilState, dx11.ImmediateContext());
        }
        break;

        case CMD_SET_SAMPLER_STATE:
        {
            SamplerStateDX11::SetToRHI(static_cast<const SWCommand_SetSamplerState*>(cmd)->samplerState, dx11.ImmediateContext());
        }
        break;

        case CMD_DRAW_PRIMITIVE:
        {
            D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY(static_cast<const SWCommand_DrawPrimitive*>(cmd)->mode);
            uint32 vertexCount = static_cast<const SWCommand_DrawPrimitive*>(cmd)->vertexCount;

            if (topo != cur_topo)
            {
                dx11.context->IASetPrimitiveTopology(topo);
                cur_topo = topo;
            }

            if (cur_rs == nullptr)
            {
                cur_rs = GetRasterizerState(rs_param);
                dx11.context->RSSetState(cur_rs);
            }

            for (uint32 s = 0; s != cur_stream_count; ++s)
                VertexBufferDX11::SetToRHI(cur_vb[s], s, 0, cur_vb_stride[s], dx11.ImmediateContext());

            dx11.context->Draw(vertexCount, 0);
        }
        break;

        case CMD_DRAW_INDEXED_PRIMITIVE:
        {
            D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY(static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->mode);
            uint32 baseVertex = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->firstVertex;
            uint32 indexCount = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->indexCount;
            uint32 startIndex = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->startIndex;

            if (topo != cur_topo)
            {
                dx11.context->IASetPrimitiveTopology(topo);
                cur_topo = topo;
            }

            if (cur_rs == nullptr)
            {
                cur_rs = GetRasterizerState(rs_param);
                dx11.context->RSSetState(cur_rs);
            }

            for (uint32 s = 0; s != cur_stream_count; ++s)
                VertexBufferDX11::SetToRHI(cur_vb[s], s, 0, cur_vb_stride[s], dx11.ImmediateContext());

            dx11.context->DrawIndexed(indexCount, startIndex, baseVertex);
        }
        break;

        case CMD_DRAW_INSTANCED_PRIMITIVE:
        {
            D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY(static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->mode);
            uint32 vertexCount = static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->vertexCount;
            uint32 instCount = static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->instanceCount;

            if (topo != cur_topo)
            {
                dx11.context->IASetPrimitiveTopology(topo);
                cur_topo = topo;
            }

            if (cur_rs == nullptr)
            {
                cur_rs = GetRasterizerState(rs_param);
                dx11.context->RSSetState(cur_rs);
            }

            for (uint32 s = 0; s != cur_stream_count; ++s)
                VertexBufferDX11::SetToRHI(cur_vb[s], s, 0, cur_vb_stride[s], dx11.ImmediateContext());

            dx11.context->DrawInstanced(vertexCount, instCount, 0, 0);
        }
        break;

        case CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE:
        {
            D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY(static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->mode);
            uint32 vertexCount = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->indexCount;
            uint32 indexCount = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->indexCount;
            uint32 startIndex = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->startIndex;
            uint32 instCount = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->instanceCount;
            uint32 baseInst = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->baseInstance;

            if (topo != cur_topo)
            {
                dx11.context->IASetPrimitiveTopology(topo);
                cur_topo = topo;
            }

            if (cur_rs == nullptr)
            {
                cur_rs = GetRasterizerState(rs_param);
                dx11.context->RSSetState(cur_rs);
            }

            for (uint32 s = 0; s != cur_stream_count; ++s)
                VertexBufferDX11::SetToRHI(cur_vb[s], s, 0, cur_vb_stride[s], dx11.ImmediateContext());

            dx11.context->DrawIndexedInstanced(indexCount, instCount, startIndex, 0, baseInst);
        }
        break;

        default:
            DAVA::Logger::Error("unsupported command: %d", cmd->type);
            DVASSERT(false, "unsupported command");
        }

        ++commandsExecuted;
        if (commandsExecuted >= immediateCommandCheckInterval)
        {
            RenderLoop::CheckImmediateCommand();
            commandsExecuted = 0;
        }

        if (cmd->type == CMD_END)
            break;

        c += cmd->size;
    }
}
#endif

void CommandBufferDX11::BindSoftwareCommandBufferDispatch(Dispatch* dispatch)
{
#if (RHI_DX11_CB_COMPILE_WITH_SOFTWARE)
    dispatch->impl_CommandBuffer_Begin = &dx11_SWCommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = &dx11_SWCommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = &dx11_SWCommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = &dx11_SWCommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = &dx11_SWCommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = &dx11_SWCommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = &dx11_SWCommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = &dx11_SWCommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = &dx11_SWCommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = &dx11_SWCommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = &dx11_SWCommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryBuffer = &dx11_SWCommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_SetQueryIndex = &dx11_SWCommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_IssueTimestampQuery = &dx11_SWCommandBuffer_IssueTimestampQuery;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = &dx11_SWCommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = &dx11_SWCommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = &dx11_SWCommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = &dx11_SWCommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = &dx11_SWCommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = &dx11_SWCommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = &dx11_SWCommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = &dx11_SWCommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = &dx11_SWCommandBuffer_SetMarker;
#endif
}

/*================================================================================
 *
 * Hardware command buffer implemetation
 *
 *================================================================================
 * Implementation for software command buffers could be found above
 */

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)

#define LUMIA_1020_DEPTHBUF_WORKAROUND 1

static void dx11_HWCommandBuffer_Begin(Handle cmdBuf)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->Reset();
    cb->Begin(cb->context);
}

static void dx11_HWCommandBuffer_End(Handle cmdBuf, Handle syncObject)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    if (cb->isLastInPass && cb->cur_query_buf != InvalidHandle)
        QueryBufferDX11::QueryComplete(cb->cur_query_buf, cb->context);

    DX11Check(cb->context->FinishCommandList(TRUE, &cb->commandList));
    cb->sync = syncObject;
    cb->isComplete = true;
}

static void dx11_HWCommandBuffer_SetPipelineState(Handle cmdBuf, Handle ps, uint32 vdeclUID)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    const VertexLayout* vdecl = (vdeclUID == VertexLayout::InvalidUID) ? nullptr : VertexLayout::Get(vdeclUID);

    cb->cur_pipelinestate = ps;
    cb->cur_stream_count = PipelineStateDX11::VertexLayoutStreamCount(ps);
    for (uint32 i = 0; i != cb->cur_stream_count; ++i)
        cb->cur_vb_stride[i] = (vdecl) ? vdecl->Stride(i) : 0;

    if (ps != cb->last_ps || vdeclUID != cb->last_vdecl)
    {
        PipelineStateDX11::SetToRHI(ps, vdeclUID, cb->context);
        cb->last_ps = ps;
        cb->last_vdecl = vdeclUID;
        StatSet::IncStat(stat_SET_PS, 1);
    }
}

static void dx11_HWCommandBuffer_SetCullMode(Handle cmdBuf, CullMode mode)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->rs_param.cullMode = mode;
    cb->cur_rs = nullptr;
}

void dx11_HWCommandBuffer_SetScissorRect(Handle cmdBuf, ScissorRect inRect)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->cur_rs = nullptr;

    if ((inRect.x == 0) && (inRect.y == 0) && (inRect.width == 0) && (inRect.height == 0))
    {
        cb->rs_param.scissorEnabled = false;
    }
    else
    {
        cb->rs_param.scissorEnabled = true;
        D3D11_RECT rect = { inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height };
        cb->context->RSSetScissorRects(1, &rect);
    }
}

static void dx11_HWCommandBuffer_SetViewport(Handle cmdBuf, Viewport inVp)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);

    if ((inVp.x == 0) && (inVp.y == 0) && (inVp.width == 0) && (inVp.height == 0))
    {
        cb->context->RSSetViewports(1, &cb->def_viewport);
    }
    else
    {
        D3D11_VIEWPORT vp = {};
        vp.TopLeftX = float(inVp.x);
        vp.TopLeftY = float(inVp.y);
        vp.Width = float(inVp.width);
        vp.Height = float(inVp.height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        cb->context->RSSetViewports(1, &vp);
    }
}

static void dx11_HWCommandBuffer_SetFillMode(Handle cmdBuf, FillMode mode)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->rs_param.wireframe = (mode == FILLMODE_WIREFRAME);
}

static void dx11_HWCommandBuffer_SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->cur_vb[streamIndex] = vb;
    if (cb->cur_vb_stride[streamIndex] == 0)
        cb->cur_vb_stride[streamIndex] = PipelineStateDX11::VertexLayoutStride(cb->cur_pipelinestate, streamIndex);
}

static void dx11_HWCommandBuffer_SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    ConstBufferDX11::SetToRHI(buffer, cb->context, cb->vertexConstBuffer);
}

static void dx11_HWCommandBuffer_SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    TextureDX11::SetToRHIVertex(tex, unitIndex, cb->context);
    StatSet::IncStat(stat_SET_TEX, 1);
}

static void dx11_HWCommandBuffer_SetIndices(Handle cmdBuf, Handle ib)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    IndexBufferDX11::SetToRHI(ib, 0, cb->context);
}

static void dx11_HWCommandBuffer_SetQueryIndex(Handle cmdBuf, uint32 objectIndex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    if (cb->cur_query_buf != InvalidHandle)
        QueryBufferDX11::SetQueryIndex(cb->cur_query_buf, objectIndex, cb->context);
}

static void dx11_HWCommandBuffer_SetQueryBuffer(Handle cmdBuf, Handle queryBuf)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    DVASSERT(cb->cur_query_buf == InvalidHandle);
    cb->cur_query_buf = queryBuf;
}

static void dx11_HWCommandBuffer_IssueTimestampQuery(Handle cmdBuf, Handle perfQuery)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    PerfQueryDX11::IssueTimestampQueryDeferred(perfQuery, cb->context);
    cb->deferredPerfQueries.push_back(perfQuery);
}

static void dx11_HWCommandBuffer_SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    ConstBufferDX11::SetToRHI(buffer, cb->context, cb->fragmentConstBuffer);
}

static void dx11_HWCommandBuffer_SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    TextureDX11::SetToRHIFragment(tex, unitIndex, cb->context);
    StatSet::IncStat(stat_SET_TEX, 1);
}

static void dx11_HWCommandBuffer_SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    DepthStencilStateDX11::SetToRHI(depthStencilState, cb->context);
}

static void dx11_HWCommandBuffer_SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    SamplerStateDX11::SetToRHI(samplerState, cb->context);
    StatSet::IncStat(stat_SET_SS, 1);
}

static void dx11_HWCommandBuffer_DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    INT baseVertex = 0;
    uint32 vertexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &vertexCount);
    cb->ApplyVertexData();
    cb->ApplyRasterizerState();
    cb->ApplyConstBuffers();
    cb->context->Draw(vertexCount, baseVertex);
    StatSet::IncStat(stat_DIP, 1);
}

static void dx11_HWCommandBuffer_DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count, uint32 vertexCount, uint32 firstVertex, uint32 startIndex)
{
    uint32 indexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &indexCount);
    cb->ApplyVertexData();
    cb->ApplyRasterizerState();
    cb->ApplyConstBuffers();
    cb->context->DrawIndexed(indexCount, startIndex, firstVertex);
    StatSet::IncStat(stat_DIP, 1);
}

static void dx11_HWCommandBuffer_DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count)
{
    INT baseVertex = 0;
    uint32 vertexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &vertexCount);
    cb->ApplyVertexData();
    cb->ApplyRasterizerState();
    cb->ApplyConstBuffers();
    cb->context->DrawInstanced(vertexCount, instCount, baseVertex, 0);
    StatSet::IncStat(stat_DIP, 1);
}

static void dx11_HWCommandBuffer_DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 count, uint32 vertexCount, uint32 firstVertex, uint32 startIndex, uint32 baseInstance)
{
    uint32 indexCount = 0;

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    cb->ApplyTopology(type, count, &indexCount);
    cb->ApplyVertexData();
    cb->ApplyRasterizerState();
    cb->ApplyConstBuffers();
    cb->context->DrawIndexedInstanced(indexCount, instCount, startIndex, firstVertex, baseInstance);
    StatSet::IncStat(stat_DIP, 1);
}

static void dx11_HWCommandBuffer_SetMarker(Handle cmdBuf, const char* text)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(cmdBuf);
    if (cb->contextAnnotation != nullptr)
    {
        wchar_t txt[128] = {};
        MultiByteToWideChar(CP_ACP, 0, text, -1, txt, countof(txt));
        cb->contextAnnotation->SetMarker(txt);
    }
}

void FlushContextIfRequired(ID3D11DeviceContext* context)
{
#if defined(__DAVAENGINE_WIN_UAP__) && LUMIA_1020_DEPTHBUF_WORKAROUND
    static int runningOnLumia1020 = -1;
    if (runningOnLumia1020 == -1)
        runningOnLumia1020 = (DAVA::DeviceInfo::GetModel().find("NOKIA RM-875") != DAVA::String::npos) ? 1 : 0;

    if (runningOnLumia1020)
        context->Flush();
#endif
}

void CommandBufferDX11_t::InitHardware()
{
    DVASSERT(dx11.useHardwareCommandBuffers);
    if (context == nullptr)
    {
        commandList = nullptr;
        if (DX11DeviceCommand(DX11Command::CREATE_DEFERRED_CONTEXT, 0, &context))
        {
            context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)(&contextAnnotation));
        }
    }
}

void CommandBufferDX11_t::ExecuteHardware()
{
    DVASSERT(dx11.useHardwareCommandBuffers);
    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_CMD_BUFFER_EXECUTE);
    dx11.context->ExecuteCommandList(commandList, FALSE);
    FlushContextIfRequired(dx11.ImmediateContext());
}

void CommandBufferDX11_t::ReleaseHardware(bool isComplete)
{
    DVASSERT(dx11.useHardwareCommandBuffers);

    if ((context != nullptr) && !isComplete)
    {
        context->ClearState();
        context->FinishCommandList(FALSE, &commandList);
    }
    DAVA::SafeRelease(contextAnnotation);
    DAVA::SafeRelease(commandList);
    DAVA::SafeRelease(context);
}

void CommandBufferDX11_t::ResetHardware()
{
    DVASSERT(dx11.useHardwareCommandBuffers);

    for (uint32 i = 0; i != MAX_VERTEX_STREAM_COUNT; ++i)
    {
        last_vb[i] = InvalidHandle;
        last_vb_stride[i] = 0;
    }
    last_ps = InvalidHandle;
    last_vdecl = VertexLayout::InvalidUID;
    memset(vertexConstBuffer, 0, sizeof(vertexConstBuffer));
    memset(fragmentConstBuffer, 0, sizeof(fragmentConstBuffer));
    context->IASetPrimitiveTopology(cur_topo);
    deferredPerfQueries.clear();
}

void CommandBufferDX11_t::ApplyVertexData()
{
    DVASSERT(dx11.useHardwareCommandBuffers);

    for (uint32 i = 0; i != cur_stream_count; ++i)
    {
        if (cur_vb[i] != last_vb[i] || cur_vb_stride[i] != last_vb_stride[i])
        {
            VertexBufferDX11::SetToRHI(cur_vb[i], i, 0, cur_vb_stride[i], context);
            last_vb[i] = cur_vb[i];
            last_vb_stride[i] = cur_vb_stride[i];
            StatSet::IncStat(stat_SET_VB, 1);
        }
    }
}

void CommandBufferDX11_t::ApplyRasterizerState()
{
    DVASSERT(dx11.useHardwareCommandBuffers);

    if (cur_rs == nullptr)
        cur_rs = GetRasterizerState(rs_param);

    if (cur_rs != last_rs)
    {
        context->RSSetState(cur_rs);
        last_rs = cur_rs;
    }
}

void CommandBufferDX11_t::ApplyConstBuffers()
{
    DVASSERT(dx11.useHardwareCommandBuffers);

    context->VSSetConstantBuffers(0, MAX_CONST_BUFFER_COUNT, vertexConstBuffer);
    context->PSSetConstantBuffers(0, MAX_CONST_BUFFER_COUNT, fragmentConstBuffer);
    StatSet::IncStat(stat_SET_CB, 2);
}

#endif

void CommandBufferDX11::BindHardwareCommandBufferDispatch(Dispatch* dispatch)
{
#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    dispatch->impl_CommandBuffer_Begin = &dx11_HWCommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = &dx11_HWCommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = &dx11_HWCommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = &dx11_HWCommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = &dx11_HWCommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = &dx11_HWCommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = &dx11_HWCommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = &dx11_HWCommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = &dx11_HWCommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = &dx11_HWCommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = &dx11_HWCommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryBuffer = &dx11_HWCommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_SetQueryIndex = &dx11_HWCommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_IssueTimestampQuery = &dx11_HWCommandBuffer_IssueTimestampQuery;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = &dx11_HWCommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = &dx11_HWCommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = &dx11_HWCommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = &dx11_HWCommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = &dx11_HWCommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = &dx11_HWCommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = &dx11_HWCommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = &dx11_HWCommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = &dx11_HWCommandBuffer_SetMarker;
#endif
}

void CommandBufferDX11::Init(uint32 maxCount)
{
    CommandBufferPoolDX11::Reserve(maxCount);
}

Handle CommandBufferDX11::Allocate(const RenderPassConfig& passDesc, bool isFirstInPass, bool isLastInPass)
{
    Handle handle = CommandBufferPoolDX11::Alloc();

    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(handle);
    cb->passCfg = passDesc;
    cb->isFirstInPass = isFirstInPass;
    cb->isLastInPass = isLastInPass;

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    if (dx11.useHardwareCommandBuffers)
        cb->InitHardware();
#endif

    return handle;
}

void CommandBufferDX11::ExecuteAndRelease(Handle handle, uint32 frameNumber)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(handle);

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    if (dx11.useHardwareCommandBuffers)
    {
        cb->ExecuteHardware();
        cb->ReleaseHardware(true);
    }
#endif

#if (RHI_DX11_CB_COMPILE_WITH_SOFTWARE)
    if (!dx11.useHardwareCommandBuffers)
        cb->ExecuteSoftware();
#endif

    if (cb->isLastInPass && cb->passCfg.UsingMSAA())
    {
        const rhi::RenderPassConfig::ColorBuffer& colorBuffer = cb->passCfg.colorBuffer[0];
        TextureDX11::ResolveMultisampling(colorBuffer.multisampleTexture, colorBuffer.texture, dx11.ImmediateContext());
    }

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    if (dx11.useHardwareCommandBuffers)
        PerfQueryDX11::DeferredPerfQueriesIssued(cb->deferredPerfQueries);
#endif

    if (cb->sync != InvalidHandle)
    {
        SyncObjectDX11_t* s = SyncObjectPoolDX11::Get(handle);
        s->is_signaled = false;
        s->frame = frameNumber;
    }

    CommandBufferPoolDX11::Free(handle);
}

void CommandBufferDX11::SignalAndRelease(Handle handle)
{
    CommandBufferDX11_t* cb = CommandBufferPoolDX11::Get(handle);
    if (cb->sync != InvalidHandle)
    {
        SyncObjectDX11_t* s = SyncObjectPoolDX11::Get(handle);
        s->is_signaled = true;
        s->is_used = true;
    }

#if (RHI_DX11_CB_COMPILE_WITH_HARDWARE)
    if (dx11.useHardwareCommandBuffers)
        cb->ReleaseHardware(cb->isComplete);
#endif

    CommandBufferPoolDX11::Free(handle);
}

/*
 *
 * RHI methods implementation
 *
 */
static void dx11_ExecuteFrame(const CommonImpl::Frame& frame)
{
    DVASSERT(frame.readyToExecute);
    DVASSERT((frame.sync == InvalidHandle) || SyncObjectPoolDX11::IsAlive(frame.sync));

    StatSet::ResetAll();

    DAVA::Vector<RenderPassDX11_t*> pass;
    pass.reserve(frame.pass.size());
    for (Handle p : frame.pass)
    {
        RenderPassDX11_t* pp = RenderPassPoolDX11::Get(p);
        pass.emplace_back(pp);
    }

    // sort from highest to lowest priorities
    std::stable_sort(pass.begin(), pass.end(), [](RenderPassDX11_t* l, RenderPassDX11_t* r)
                     {
                         return l->priority > r->priority;
                     });

    if (frame.sync != InvalidHandle)
    {
        SyncObjectDX11_t* s = SyncObjectPoolDX11::Get(frame.sync);
        s->frame = frame.frameNumber;
        s->is_signaled = false;
        s->is_used = true;
    }

    PerfQueryDX11::ObtainPerfQueryMeasurment(dx11.ImmediateContext());
    PerfQueryDX11::BeginMeasurment(dx11.ImmediateContext());

    if (frame.perfQueryStart != InvalidHandle)
        PerfQueryDX11::IssueTimestampQuery(frame.perfQueryStart, dx11.ImmediateContext());

    if (dx11.useHardwareCommandBuffers)
    {
        DAVA::LockGuard<DAVA::Mutex> lock(_DX11_PendingSecondaryCmdListSync);
        for (ComPtr<ID3D11CommandList> cmdList : _DX11_PendingSecondaryCmdLists)
            dx11.context->ExecuteCommandList(cmdList.Get(), FALSE);
        _DX11_PendingSecondaryCmdLists.clear();
    }

    for (RenderPassDX11_t* pp : pass)
    {
        if (pp->perfQueryStart != InvalidHandle)
            PerfQueryDX11::IssueTimestampQuery(pp->perfQueryStart, dx11.ImmediateContext());

        for (Handle cb_h : pp->commandBuffers)
            CommandBufferDX11::ExecuteAndRelease(cb_h, frame.frameNumber);

        if (pp->perfQueryEnd != InvalidHandle)
            PerfQueryDX11::IssueTimestampQuery(pp->perfQueryEnd, dx11.ImmediateContext());
    }

    for (Handle p : frame.pass)
        RenderPassPoolDX11::Free(p);

    if (frame.perfQueryEnd != InvalidHandle)
        PerfQueryDX11::IssueTimestampQuery(frame.perfQueryEnd, dx11.ImmediateContext());

    DAVA::LockGuard<DAVA::Mutex> lock(_DX11_SyncObjectsSync);
    for (SyncObjectPoolDX11::Iterator s = SyncObjectPoolDX11::Begin(), s_end = SyncObjectPoolDX11::End(); s != s_end; ++s)
    {
        if (s->is_used && (frame.frameNumber - s->frame >= 2))
            s->is_signaled = true;
    }
}

static void dx11_RejectFrame(const CommonImpl::Frame& frame)
{
    if (frame.sync != InvalidHandle)
    {
        SyncObjectDX11_t* s = SyncObjectPoolDX11::Get(frame.sync);
        s->is_signaled = true;
        s->is_used = true;
    }

    for (Handle p : frame.pass)
        RenderPassDX11::RejectCommandBuffersAndRelease(p);
}

static void dx11_FinishFrame()
{
    if (dx11.useHardwareCommandBuffers)
    {
        ComPtr<ID3D11CommandList> cmdList;
        {
            DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);
            DX11Check(dx11.deferredContext->FinishCommandList(TRUE, cmdList.GetAddressOf()));
        }
        {
            DAVA::LockGuard<DAVA::Mutex> lock(_DX11_PendingSecondaryCmdListSync);
            _DX11_PendingSecondaryCmdLists.emplace_back(cmdList);
        }
    }

    ConstBufferDX11::InvalidateAllInstances();
}

static void dx11_ExecImmediateCommand(CommonImpl::ImmediateCommand* command)
{
    DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_EXECUTE_IMMEDIATE_CMDS);

    DX11Command* commandData = reinterpret_cast<DX11Command*>(command->cmdData);
    for (DX11Command *cmd = commandData, *cmdEnd = commandData + command->cmdCount; cmd != cmdEnd; ++cmd)
    {
        uint64* arg = cmd->arguments.arg;

        Trace("exec %i\n", int(cmd->func));
        switch (cmd->func)
        {
        case DX11Command::NOP:
            break;

        case DX11Command::MAP:
            cmd->retval = dx11.context->Map((ID3D11Resource*)(arg[0]), UINT(arg[1]), D3D11_MAP(arg[2]), UINT(arg[3]), (D3D11_MAPPED_SUBRESOURCE*)(arg[4]));
            break;

        case DX11Command::UNMAP:
            dx11.context->Unmap((ID3D11Resource*)(arg[0]), UINT(arg[1]));
            break;

        case DX11Command::UPDATE_SUBRESOURCE:
            dx11.context->UpdateSubresource((ID3D11Resource*)(arg[0]), UINT(arg[1]), (const D3D11_BOX*)(arg[2]), (const void*)(arg[3]), UINT(arg[4]), UINT(arg[5]));
            break;

        case DX11Command::COPY_RESOURCE:
            dx11.context->CopyResource((ID3D11Resource*)(arg[0]), (ID3D11Resource*)(arg[1]));
            break;

        case DX11Command::SYNC_CPU_GPU:
        {
            if (DeviceCaps().isPerfQuerySupported)
            {
                ID3D11Query* tsQuery = nullptr;
                ID3D11Query* fqQuery = nullptr;

                D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP };
                DX11DeviceCommand(DX11Command::CREATE_QUERY, &desc, &tsQuery);

                desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
                DX11DeviceCommand(DX11Command::CREATE_QUERY, &desc, &fqQuery);

                if (tsQuery && fqQuery)
                {
                    dx11.context->Begin(fqQuery);
                    dx11.context->End(tsQuery);
                    dx11.context->End(fqQuery);

                    uint64 timestamp = 0;
                    while (S_FALSE == dx11.context->GetData(tsQuery, &timestamp, sizeof(uint64), 0))
                    {
                    };

                    if (timestamp)
                    {
                        *reinterpret_cast<uint64*>(arg[0]) = DAVA::SystemTimer::GetUs();

                        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
                        while (S_FALSE == dx11.context->GetData(fqQuery, &data, sizeof(data), 0))
                        {
                        };

                        if (!data.Disjoint && data.Frequency)
                        {
                            *reinterpret_cast<uint64*>(arg[1]) = timestamp / (data.Frequency / 1000000); //mcs
                        }
                    }
                }
                DAVA::SafeRelease(tsQuery);
                DAVA::SafeRelease(fqQuery);
            }
        }
        break;
        case DX11Command::QUERY_INTERFACE:
        {
            ValidateDX11Device("QueryInterface");
            cmd->retval = dx11.device.Get()->QueryInterface(*(const GUID*)(arg[0]), (void**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_DEFERRED_CONTEXT:
        {
            ValidateDX11Device("CreateDeferredContext");
            cmd->retval = dx11.device->CreateDeferredContext((UINT)arg[0], (ID3D11DeviceContext**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_BLEND_STATE:
        {
            ValidateDX11Device("CreateBlendState");
            cmd->retval = dx11.device->CreateBlendState((const D3D11_BLEND_DESC*)(arg[0]), (ID3D11BlendState**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_SAMPLER_STATE:
        {
            ValidateDX11Device("CreateSamplerState");
            cmd->retval = dx11.device->CreateSamplerState((const D3D11_SAMPLER_DESC*)(arg[0]), (ID3D11SamplerState**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_RASTERIZER_STATE:
        {
            ValidateDX11Device("CreateRasterizerState");
            cmd->retval = dx11.device->CreateRasterizerState((const D3D11_RASTERIZER_DESC*)(arg[0]), (ID3D11RasterizerState**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_DEPTH_STENCIL_STATE:
        {
            ValidateDX11Device("CreateDepthStencilState");
            cmd->retval = dx11.device->CreateDepthStencilState((const D3D11_DEPTH_STENCIL_DESC*)(arg[0]), (ID3D11DepthStencilState**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_VERTEX_SHADER:
        {
            ValidateDX11Device("CreateVertexShader");
            cmd->retval = dx11.device->CreateVertexShader((const void*)(arg[0]), (SIZE_T)(arg[1]), (ID3D11ClassLinkage*)(arg[2]), (ID3D11VertexShader**)(arg[3]));
            break;
        }
        case DX11Command::CREATE_PIXEL_SHADER:
        {
            ValidateDX11Device("CreatePixelShader");
            cmd->retval = dx11.device->CreatePixelShader((const void*)(arg[0]), (SIZE_T)(arg[1]), (ID3D11ClassLinkage*)(arg[2]), (ID3D11PixelShader**)(arg[3]));
            break;
        }
        case DX11Command::CREATE_INPUT_LAYOUT:
        {
            ValidateDX11Device("CreateInputLayout");
            cmd->retval = dx11.device->CreateInputLayout((const D3D11_INPUT_ELEMENT_DESC*)(arg[0]), (UINT)(arg[1]), (const void*)(arg[2]), (SIZE_T)(arg[3]), (ID3D11InputLayout**)(arg[4]));
            break;
        }
        case DX11Command::CREATE_QUERY:
        {
            ValidateDX11Device("CreateQuery");
            cmd->retval = dx11.device->CreateQuery((const D3D11_QUERY_DESC*)(arg[0]), (ID3D11Query**)(arg[1]));
            break;
        }
        case DX11Command::CREATE_BUFFER:
        {
            ValidateDX11Device("CreateBuffer");
            cmd->retval = dx11.device->CreateBuffer((const D3D11_BUFFER_DESC*)(arg[0]), (const D3D11_SUBRESOURCE_DATA*)(arg[1]), (ID3D11Buffer**)(arg[2]));
            break;
        }
        case DX11Command::CREATE_TEXTURE_2D:
        {
            ValidateDX11Device("CreateTexture2D");
            cmd->retval = dx11.device->CreateTexture2D((const D3D11_TEXTURE2D_DESC*)(arg[0]), (const D3D11_SUBRESOURCE_DATA*)(arg[1]), (ID3D11Texture2D**)(arg[2]));
            break;
        }
        case DX11Command::CREATE_RENDER_TARGET_VIEW:
        {
            ValidateDX11Device("CreateRenderTargetView");
            cmd->retval = dx11.device->CreateRenderTargetView((ID3D11Resource*)(arg[0]), (const D3D11_RENDER_TARGET_VIEW_DESC*)(arg[1]), (ID3D11RenderTargetView**)(arg[2]));
            break;
        }
        case DX11Command::CREATE_DEPTH_STENCIL_VIEW:
        {
            ValidateDX11Device("CreateDepthStencilView");
            cmd->retval = dx11.device->CreateDepthStencilView((ID3D11Resource*)(arg[0]), (const D3D11_DEPTH_STENCIL_VIEW_DESC*)(arg[1]), (ID3D11DepthStencilView**)(arg[2]));
            break;
        }
        case DX11Command::CREATE_SHADER_RESOURCE_VIEW:
        {
            ValidateDX11Device("CreateShaderResourceView");
            cmd->retval = dx11.device->CreateShaderResourceView((ID3D11Resource*)(arg[0]), (const D3D11_SHADER_RESOURCE_VIEW_DESC*)(arg[1]), (ID3D11ShaderResourceView**)(arg[2]));
            break;
        }
        case DX11Command::CHECK_FORMAT_SUPPORT:
        {
            ValidateDX11Device("CheckFormatSupport");
            cmd->retval = dx11.device->CheckFormatSupport((DXGI_FORMAT)(arg[0]), (UINT*)(arg[1]));
            if (cmd->retval == E_FAIL)
            {
                cmd->retval = S_OK;
                *reinterpret_cast<UINT*>(arg[1]) = 0;
            }
            break;
        }
        default:
        {
            DAVA::String message = DAVA::Format("Invalid or unsupported DX11 command: %u", cmd->func);
            DVASSERT(0, message.c_str());
        }
        }
    }
}

/*
 *
 * Render pass implementation
 *
 */
static Handle dx11_RenderPass_Allocate(const RenderPassConfig& passDesc, uint32 cmdBufCount, Handle* cmdBuf)
{
    DVASSERT(cmdBufCount > 0);
    DVASSERT(passDesc.IsValid());

    Handle handle = RenderPassPoolDX11::Alloc();
    RenderPassDX11_t* pass = RenderPassPoolDX11::Get(handle);
    pass->commandBuffers.resize(cmdBufCount);
    pass->priority = passDesc.priority;
    pass->perfQueryStart = passDesc.perfQueryStart;
    pass->perfQueryEnd = passDesc.perfQueryEnd;
    for (uint32 i = 0; i != cmdBufCount; ++i)
    {
        Handle cbHandle = CommandBufferDX11::Allocate(passDesc, (i == 0), (i + 1 == cmdBufCount));
        pass->commandBuffers[i] = cbHandle;
        cmdBuf[i] = cbHandle;
    }
    return handle;
}

static void dx11_RenderPass_Begin(Handle pass)
{
}

static void dx11_RenderPass_End(Handle pass)
{
}

void RenderPassDX11::Init(uint32 maxCount)
{
    RenderPassPoolDX11::Reserve(maxCount);
}

void RenderPassDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Renderpass_Allocate = &dx11_RenderPass_Allocate;
    dispatch->impl_Renderpass_Begin = &dx11_RenderPass_Begin;
    dispatch->impl_Renderpass_End = &dx11_RenderPass_End;
    dispatch->impl_ProcessImmediateCommand = &dx11_ExecImmediateCommand;
    dispatch->impl_ExecuteFrame = &dx11_ExecuteFrame;
    dispatch->impl_RejectFrame = &dx11_RejectFrame;
    dispatch->impl_FinishFrame = &dx11_FinishFrame;
}

void RenderPassDX11::RejectCommandBuffersAndRelease(Handle handle)
{
    RenderPassDX11_t* pp = RenderPassPoolDX11::Get(handle);

    for (Handle cmdBuffer : pp->commandBuffers)
        CommandBufferDX11::SignalAndRelease(cmdBuffer);
    pp->commandBuffers.clear();

    RenderPassPoolDX11::Free(handle);
}

/*
 *
 * Sync object implementation
 *
 */
static Handle dx11_SyncObject_Create()
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX11_SyncObjectsSync);

    Handle handle = SyncObjectPoolDX11::Alloc();
    SyncObjectDX11_t* sync = SyncObjectPoolDX11::Get(handle);
    sync->is_signaled = false;
    sync->is_used = false;
    return handle;
}

static void dx11_SyncObject_Delete(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX11_SyncObjectsSync);
    SyncObjectPoolDX11::Free(obj);
}

static bool dx11_SyncObject_IsSignaled(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_DX11_SyncObjectsSync);
    if (!SyncObjectPoolDX11::IsAlive(obj))
        return true;

    bool signaled = false;
    SyncObjectDX11_t* sync = SyncObjectPoolDX11::Get(obj);

    if (sync)
        signaled = sync->is_signaled;

    return signaled;
}

void SyncObjectDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SyncObject_Create = &dx11_SyncObject_Create;
    dispatch->impl_SyncObject_Delete = &dx11_SyncObject_Delete;
    dispatch->impl_SyncObject_IsSignaled = &dx11_SyncObject_IsSignaled;
}
}
