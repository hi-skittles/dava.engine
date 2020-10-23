#include "../rhi_Public.h"
#include "rhi_Private.h"
#include "rhi_CommonImpl.h"
#include "rhi_Pool.h"
#include "rhi_Utils.h"

#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Concurrency/Thread.h"
#include "RenderLoop.h"
#include "FrameLoop.h"

namespace rhi
{
struct PacketList_t
{
    struct Desc
    {
    };

    Handle cmdBuf;
    Handle queryBuffer;
    Viewport viewport;

    Handle curPipelineState;
    uint32 curVertexLayout;
    Handle curTextureSet;
    Handle curSamplerState;
    Handle curDepthStencilState;
    CullMode curCullMode;

    Handle defDepthStencilState;
    Handle defSamplerState;
    ScissorRect defScissorRect;

    Handle curVertexStream[MAX_VERTEX_STREAM_COUNT];

    uint32 setDefaultViewport : 1;
    uint32 restoreDefScissorRect : 1;
    uint32 restoreSolidFill : 1;
    uint32 invertCulling : 1;

    // debug
    uint32 batchIndex;
};

typedef ResourcePool<PacketList_t, RESOURCE_PACKET_LIST, PacketList_t::Desc, false> PacketListPool;
RHI_IMPL_POOL(PacketList_t, RESOURCE_PACKET_LIST, PacketList_t::Desc, false);

void InitPacketListPool(uint32 maxCount)
{
    PacketListPool::Reserve(maxCount);
}

//------------------------------------------------------------------------------

void SetFramePerfQueries(HPerfQuery startQuery, HPerfQuery endQuery)
{
    FrameLoop::SetFramePerfQueries(startQuery, endQuery);
}

//------------------------------------------------------------------------------

HRenderPass AllocateRenderPass(const RenderPassConfig& passDesc, uint32 packetListCount, HPacketList* packetList)
{
    Handle cb[8];
    DVASSERT(packetListCount < countof(cb));

    Handle pass = RenderPass::Allocate(passDesc, packetListCount, cb);
    FrameLoop::AddPass(pass);

    for (unsigned i = 0; i != packetListCount; ++i)
    {
        Handle plh = PacketListPool::Alloc();
        PacketList_t* pl = PacketListPool::Get(plh);

        pl->cmdBuf = cb[i];
        pl->queryBuffer = passDesc.queryBuffer;
        pl->setDefaultViewport = i == 0;
        pl->viewport = passDesc.viewport;
        pl->invertCulling = passDesc.invertCulling;

        packetList[i] = HPacketList(plh);
    }

    return HRenderPass(pass);
}

//------------------------------------------------------------------------------

void BeginRenderPass(HRenderPass pass)
{
    RenderPass::Begin(pass);
}

//------------------------------------------------------------------------------

void EndRenderPass(HRenderPass pass)
{
    RenderPass::End(pass);
}

//------------------------------------------------------------------------------

void BeginPacketList(HPacketList packetList)
{
    PacketList_t* pl = PacketListPool::Get(packetList);
    static Handle def_ds = rhi::InvalidHandle;
    static Handle def_ss = rhi::InvalidHandle;

    if (def_ds == rhi::InvalidHandle)
    {
        rhi::DepthStencilState::Descriptor desc;

        def_ds = rhi::DepthStencilState::Create(desc);
    }

    if (def_ss == rhi::InvalidHandle)
    {
        rhi::SamplerState::Descriptor desc;

        desc.fragmentSamplerCount = rhi::MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT;
        desc.vertexSamplerCount = rhi::MAX_VERTEX_TEXTURE_SAMPLER_COUNT;
        def_ss = rhi::SamplerState::Create(desc);
    }

    pl->curPipelineState = InvalidHandle;
    pl->curVertexLayout = rhi::VertexLayout::InvalidUID;
    pl->curTextureSet = InvalidHandle;
    pl->defDepthStencilState = def_ds;
    pl->defSamplerState = def_ss;

    CommandBuffer::Begin(pl->cmdBuf);

    if (pl->setDefaultViewport)
        CommandBuffer::SetViewport(pl->cmdBuf, pl->viewport);

    CommandBuffer::SetScissorRect(pl->cmdBuf, ScissorRect()); // ensure default scissor-rect is used

    CommandBuffer::SetDepthStencilState(pl->cmdBuf, pl->defDepthStencilState);
    pl->curDepthStencilState = pl->defDepthStencilState;

    CommandBuffer::SetSamplerState(pl->cmdBuf, def_ss);
    pl->curSamplerState = pl->defSamplerState;

    CommandBuffer::SetCullMode(pl->cmdBuf, CULL_NONE);
    pl->curCullMode = CULL_NONE;

    for (unsigned i = 0; i != countof(pl->curVertexStream); ++i)
        pl->curVertexStream[i] = InvalidHandle;

    CommandBuffer::SetCullMode(pl->cmdBuf, CULL_NONE);
    rhi::CommandBuffer::SetFillMode(pl->cmdBuf, FILLMODE_SOLID);

    if (pl->queryBuffer != rhi::InvalidHandle)
        CommandBuffer::SetQueryBuffer(pl->cmdBuf, pl->queryBuffer);

    pl->restoreDefScissorRect = false;
    pl->restoreSolidFill = false;

    pl->batchIndex = 0;
}

//------------------------------------------------------------------------------

void EndPacketList(HPacketList packetList, HSyncObject syncObject)
{
    PacketList_t* pl = PacketListPool::Get(packetList);

    CommandBuffer::End(pl->cmdBuf, syncObject);
    PacketListPool::Free(packetList);
}

//------------------------------------------------------------------------------

void AddPackets(HPacketList packetList, const Packet* packet, uint32 packetCount)
{
    //PROFILER_TIMING("rhi::AddPackets");

    PacketList_t* pl = PacketListPool::Get(packetList);
    Handle cmdBuf = pl->cmdBuf;

    for (const Packet *p = packet, *p_end = packet + packetCount; p != p_end; ++p)
    {
        if (p->perfQueryStart.IsValid())
            rhi::CommandBuffer::IssueTimestampQuery(cmdBuf, p->perfQueryStart);

        Handle dsState = (p->depthStencilState != rhi::InvalidHandle) ? p->depthStencilState : pl->defDepthStencilState;
        Handle sState = (p->samplerState != rhi::InvalidHandle) ? p->samplerState : pl->defSamplerState;

        if (p->renderPipelineState != pl->curPipelineState || p->vertexLayoutUID != pl->curVertexLayout)
        {
            rhi::CommandBuffer::SetPipelineState(cmdBuf, p->renderPipelineState, p->vertexLayoutUID);
            pl->curPipelineState = p->renderPipelineState;
            pl->curVertexLayout = p->vertexLayoutUID;
        }

        if (dsState != pl->curDepthStencilState)
        {
            rhi::CommandBuffer::SetDepthStencilState(cmdBuf, dsState);
            pl->curDepthStencilState = p->depthStencilState;
        }
        if (sState != pl->curSamplerState)
        {
            rhi::CommandBuffer::SetSamplerState(cmdBuf, sState);
            pl->curSamplerState = p->samplerState;
        }
        if (p->cullMode != pl->curCullMode)
        {
            CullMode mode = p->cullMode;

            if (pl->invertCulling)
            {
                switch (mode)
                {
                case CULL_CW:
                    mode = CULL_CCW;
                    break;
                case CULL_CCW:
                    mode = CULL_CW;
                    break;
                default:
                    break;
                }
            }

            rhi::CommandBuffer::SetCullMode(cmdBuf, mode);
            pl->curCullMode = p->cullMode;
        }

        for (unsigned i = 0; i != p->vertexStreamCount; ++i)
        {
            //-            if( p->vertexStream[i] != pl->curVertexStream[i] )
            {
                rhi::CommandBuffer::SetVertexData(cmdBuf, p->vertexStream[i], i);
                pl->curVertexStream[i] = p->vertexStream[i];
            }
        }

        if (p->indexBuffer != InvalidHandle)
        {
            rhi::CommandBuffer::SetIndices(cmdBuf, p->indexBuffer);
        }

        for (unsigned i = 0; i != p->vertexConstCount; ++i)
        {
            rhi::CommandBuffer::SetVertexConstBuffer(cmdBuf, i, p->vertexConst[i]);
        }

        for (unsigned i = 0; i != p->fragmentConstCount; ++i)
        {
            rhi::CommandBuffer::SetFragmentConstBuffer(cmdBuf, i, p->fragmentConst[i]);
        }

        if (p->textureSet != pl->curTextureSet)
        {
            if (p->textureSet != InvalidHandle)
            {
                CommonImpl::TextureSet_t* ts = TextureSet::Get(p->textureSet);
                for (unsigned i = 0; i != ts->fragmentTextureCount; ++i)
                {
                    rhi::CommandBuffer::SetFragmentTexture(cmdBuf, i, ts->fragmentTexture[i]);
                }
                for (unsigned i = 0; i != ts->vertexTextureCount; ++i)
                {
                    rhi::CommandBuffer::SetVertexTexture(cmdBuf, i, ts->vertexTexture[i]);
                }
            }

            pl->curTextureSet = p->textureSet;
        }

        if (p->options & Packet::OPT_OVERRIDE_SCISSOR)
        {
            rhi::CommandBuffer::SetScissorRect(cmdBuf, p->scissorRect);
            pl->restoreDefScissorRect = true;
        }
        else
        {
            if (pl->restoreDefScissorRect)
            {
                rhi::CommandBuffer::SetScissorRect(cmdBuf, pl->defScissorRect);
                pl->restoreDefScissorRect = false;
            }
        }

        if (p->options & Packet::OPT_WIREFRAME)
        {
            rhi::CommandBuffer::SetFillMode(cmdBuf, FILLMODE_WIREFRAME);
            pl->restoreSolidFill = true;
        }
        else
        {
            if (pl->restoreSolidFill)
            {
                rhi::CommandBuffer::SetFillMode(cmdBuf, FILLMODE_SOLID);
                pl->restoreSolidFill = false;
            }
        }

        rhi::CommandBuffer::SetQueryIndex(cmdBuf, p->queryIndex);

        if (p->instanceCount)
        {
            if (p->indexBuffer != InvalidHandle)
            {
                DVASSERT(p->vertexCount); // vertexCount MUST BE SPECIFIED
                rhi::CommandBuffer::DrawInstancedIndexedPrimitive(cmdBuf, p->primitiveType, p->instanceCount, p->primitiveCount, p->vertexCount, p->baseVertex, p->startIndex, p->baseInstance);
            }
            else
            {
                rhi::CommandBuffer::DrawInstancedPrimitive(cmdBuf, p->primitiveType, p->instanceCount, p->primitiveCount);
            }
        }
        else
        {
            if (p->indexBuffer != InvalidHandle)
            {
                DVASSERT(p->vertexCount); // vertexCount MUST BE SPECIFIED
                rhi::CommandBuffer::DrawIndexedPrimitive(cmdBuf, p->primitiveType, p->primitiveCount, p->vertexCount, p->baseVertex, p->startIndex);
            }
            else
            {
                rhi::CommandBuffer::DrawPrimitive(cmdBuf, p->primitiveType, p->primitiveCount);
            }
        }

        if (p->perfQueryEnd.IsValid())
            rhi::CommandBuffer::IssueTimestampQuery(cmdBuf, p->perfQueryEnd);

        ++pl->batchIndex;
    }
}

//------------------------------------------------------------------------------

void AddPacket(HPacketList packetList, const Packet& packet)
{
    AddPackets(packetList, &packet, 1);
}

void Present()
{
    RenderLoop::Present();
}

HSyncObject GetCurrentFrameSyncObject()
{
    return RenderLoop::GetCurrentFrameSyncObject();
}

void SuspendRendering()
{
    RenderLoop::SuspendRender();
}

void SuspendRenderingAfterFrame()
{
    RenderLoop::SuspendRenderAfterFrame();
}

void ResumeRendering()
{
    RenderLoop::ResumeRender();
}

void Initialize(Api api, const InitParam& param)
{
    InitializeImplementation(api, param);

    //init common
    if (param.maxTextureSetCount)
        TextureSet::InitTextreSetPool(param.maxTextureSetCount);
    if (param.maxPacketListCount)
        InitPacketListPool(param.maxPacketListCount);

    //temporary here to support legacy - later read all this from config, and assert on unsupported values
    DAVA::Thread::eThreadPriority priority = DAVA::Thread::PRIORITY_NORMAL;
    int32 bindToProcessor = -1;
    uint32 renderTreadFrameCount = (param.threadedRenderEnabled) ? param.threadedRenderFrameCount : 0;
    if (api == RHI_DX11)
    {
        bindToProcessor = 1;
        priority = DAVA::Thread::PRIORITY_HIGH;
    }
    //end of temporary

    RenderLoop::InitializeRenderLoop(renderTreadFrameCount, priority, bindToProcessor);
}

void Uninitialize()
{
    UninitializeImplementation();
    RenderLoop::UninitializeRenderLoop();
}

void ReportError(const InitParam& params, RenderingError error)
{
    if (params.renderingErrorCallback)
    {
        params.renderingErrorCallback(error, params.renderingErrorCallbackContext);
    }
}

} //namespace rhi
