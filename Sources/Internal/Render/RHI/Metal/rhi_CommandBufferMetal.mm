#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "rhi_Metal.h"

#include "../rhi_Type.h"
#include "../rhi_Public.h"
#include "../Common/dbg_StatSet.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;
#include "Debug/ProfilerCPU.h"
#include "../Common/rhi_CommonImpl.h"
#include "../Common/SoftwareCommandBuffer.h"

#include "_metal.h"

#if 0
    #define MTL_TRACE(...) DAVA::Logger::Info(__VA_ARGS__)
    #define MTL_CP DAVA::Logger::Info("%s : %i", __FILE__, __LINE__);
#else
    #define MTL_TRACE(...)
    #define MTL_CP 
#endif


#if !(TARGET_IPHONE_SIMULATOR == 1)
namespace rhi
{
struct RenderPassMetal_t
{
    RenderPassConfig cfg;
    MTLRenderPassDescriptor* desc;
    id<MTLCommandBuffer> commandBuffer;
    id<MTLParallelRenderCommandEncoder> encoder;
    std::vector<Handle> cmdBuf;
    int priority;    
    

#if !RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    bool Initialize();
#endif
};

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
class CommandBufferMetal_t
#else
class CommandBufferMetal_t : public SoftwareCommandBuffer
#endif
{
public:
    id<MTLRenderCommandEncoder> encoder;
    id<MTLCommandBuffer> buf;

    id<MTLTexture> rt;
    MTLPixelFormat color_fmt[MAX_RENDER_TARGET_COUNT];
    uint32 color_count;
    Handle cur_ib;
    unsigned cur_vstream_count;
    Handle cur_vb[MAX_VERTEX_STREAM_COUNT];
    uint32 cur_stride;
    uint32 sampleCount;
    bool ds_used;

    void _ApplyVertexData(unsigned firstVertex = 0);
    
    #if !RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    void Execute();
    #endif
};

struct SyncObjectMetal_t
{
    uint32 is_signaled : 1;
};

typedef ResourcePool<CommandBufferMetal_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false> CommandBufferPoolMetal;
typedef ResourcePool<RenderPassMetal_t, RESOURCE_RENDER_PASS, RenderPassConfig, false> RenderPassPoolMetal;
typedef ResourcePool<SyncObjectMetal_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false> SyncObjectPoolMetal;

RHI_IMPL_POOL(CommandBufferMetal_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false);
RHI_IMPL_POOL(RenderPassMetal_t, RESOURCE_RENDER_PASS, RenderPassConfig, false);
RHI_IMPL_POOL(SyncObjectMetal_t, RESOURCE_SYNC_OBJECT, SyncObject::Descriptor, false);

static DAVA::Mutex _Metal_SyncObjectsSync;

static ResetParam _Metal_ResetParam;
static DAVA::Mutex _Metal_ResetSync;
static bool _Metal_ResetPending = false;

id<CAMetalDrawable> _Metal_currentDrawable = nil;

void CommandBufferMetal_t::_ApplyVertexData(unsigned firstVertex)
{
    for (unsigned s = 0; s != cur_vstream_count; ++s)
    {
        unsigned base = 0;
        id<MTLBuffer> vb = VertexBufferMetal::GetBuffer(cur_vb[s], &base);
        unsigned off = (s == 0) ? firstVertex * cur_stride : 0;

        [encoder setVertexBuffer:vb offset:base + off atIndex:s];
    }
}

#if !RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
void CommandBufferMetal_t::Execute()
{
    for (const uint8 *c = cmdData, *c_end = cmdData + curUsedSize; c != c_end;)
    {
        const SWCommand* cmd = reinterpret_cast<const SWCommand*>(c);

        switch (SoftwareCommandType(cmd->type))
        {
        case CMD_BEGIN:
        {
            cur_vstream_count = 0;
            for (unsigned s = 0; s != countof(cur_vb); ++s)
                cur_vb[s] = InvalidHandle;

            [encoder setDepthStencilState:_Metal_DefDepthState];
        }
        break;

        case CMD_END:
        {
            [encoder endEncoding];

            Handle syncObject = static_cast<const SWCommand_End*>(cmd)->syncObject;

            if (syncObject != InvalidHandle)
            {
                [buf addCompletedHandler:^(id<MTLCommandBuffer> cmdb) {
                  SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(syncObject);

                  sync->is_signaled = true;
                }];
            }
        }
        break;

        case CMD_SET_PIPELINE_STATE:
        {
            Handle ps = static_cast<const SWCommand_SetPipelineState*>(cmd)->ps;
            unsigned layoutUID = static_cast<const SWCommand_SetPipelineState*>(cmd)->vdecl;

            cur_stride = PipelineStateMetal::SetToRHI(ps, layoutUID, color_fmt, color_count, ds_used, encoder, sampleCount);
            cur_vstream_count = PipelineStateMetal::VertexStreamCount(ps);
            StatSet::IncStat(stat_SET_PS, 1);
        }
        break;

        case CMD_SET_CULL_MODE:
        {
            switch (CullMode(static_cast<const SWCommand_SetCullMode*>(cmd)->mode))
            {
            case CULL_NONE:
                [encoder setCullMode:MTLCullModeNone];
                break;

            case CULL_CCW:
                [encoder setFrontFacingWinding:MTLWindingClockwise];
                [encoder setCullMode:MTLCullModeBack];
                break;

            case CULL_CW:
                [encoder setFrontFacingWinding:MTLWindingClockwise];
                [encoder setCullMode:MTLCullModeFront];
                break;
            }
        }
        break;

        case CMD_SET_SCISSOR_RECT:
        {
            int x = static_cast<const SWCommand_SetScissorRect*>(cmd)->x;
            int y = static_cast<const SWCommand_SetScissorRect*>(cmd)->y;
            int w = static_cast<const SWCommand_SetScissorRect*>(cmd)->width;
            int h = static_cast<const SWCommand_SetScissorRect*>(cmd)->height;
            MTLScissorRect rc;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                unsigned max_x = (rt) ? unsigned(rt.width) : unsigned(_Metal_DefFrameBuf.width);
                unsigned max_y = (rt) ? unsigned(rt.height) : unsigned(_Metal_DefFrameBuf.height);

                rc.x = x;
                rc.y = y;
                rc.width = (x + w > max_x) ? (max_x - rc.x) : w;
                rc.height = (y + h > max_y) ? (max_y - rc.y) : h;

                if (rc.width == 0)
                {
                    rc.width = 1;
                    if (rc.x > 0)
                        --rc.x;
                }

                if (rc.height == 0)
                {
                    rc.height = 1;
                    if (rc.y > 0)
                        --rc.y;
                }
            }
            else
            {
                rc.x = 0;
                rc.y = 0;
                if (rt)
                {
                    rc.width = rt.width;
                    rc.height = rt.height;
                }
                else
                {
                    rc.width = _Metal_DefFrameBuf.width;
                    rc.height = _Metal_DefFrameBuf.height;
                }
            }

            [encoder setScissorRect:rc];
        }
        break;

        case CMD_SET_VIEWPORT:
        {
            MTLViewport vp;
            int x = static_cast<const SWCommand_SetViewport*>(cmd)->x;
            int y = static_cast<const SWCommand_SetViewport*>(cmd)->y;
            int w = static_cast<const SWCommand_SetViewport*>(cmd)->width;
            int h = static_cast<const SWCommand_SetViewport*>(cmd)->height;

            if (!(x == 0 && y == 0 && w == 0 && h == 0))
            {
                vp.originX = x;
                vp.originY = y;
                vp.width = w;
                vp.height = h;
                vp.znear = 0.0;
                vp.zfar = 1.0;
            }
            else
            {
                vp.originX = 0;
                vp.originY = 0;
                vp.width = rt.width;
                vp.height = rt.height;
                vp.znear = 0.0;
                vp.zfar = 1.0;
            }

            [encoder setViewport:vp];
        }
        break;

        case CMD_SET_FILLMODE:
        {
            [encoder setTriangleFillMode:(FillMode(static_cast<const SWCommand_SetFillMode*>(cmd)->mode) == FILLMODE_WIREFRAME) ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
        }
        break;

        case CMD_SET_VERTEX_DATA:
        {
            Handle vb = static_cast<const SWCommand_SetVertexData*>(cmd)->vb;
            unsigned streamIndex = static_cast<const SWCommand_SetVertexData*>(cmd)->streamIndex;

            cur_vb[streamIndex] = vb;
            StatSet::IncStat(stat_SET_VB, 1);
        }
        break;

        case CMD_SET_VERTEX_PROG_CONST_BUFFER:
        {
            Handle buffer = static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd)->buffer;
            unsigned index = static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd)->bufIndex;
            uintptr_t inst = reinterpret_cast<uintptr_t>(static_cast<const SWCommand_SetVertexProgConstBuffer*>(cmd)->inst);
            unsigned instOffset = static_cast<unsigned>(inst);

            ConstBufferMetal::SetToRHI(buffer, index, instOffset, encoder);
        }
        break;

        case CMD_SET_VERTEX_TEXTURE:
        {
            Handle tex = static_cast<const SWCommand_SetVertexTexture*>(cmd)->tex;
            unsigned unitIndex = static_cast<const SWCommand_SetVertexTexture*>(cmd)->unitIndex;

            TextureMetal::SetToRHIVertex(tex, unitIndex, encoder);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_SET_INDICES:
        {
            cur_ib = static_cast<const SWCommand_SetIndices*>(cmd)->ib;
            StatSet::IncStat(stat_SET_IB, 1);
        }
        break;

        case CMD_SET_QUERY_INDEX:
        {
            unsigned index = static_cast<const SWCommand_SetQueryIndex*>(cmd)->objectIndex;

            if (index != DAVA::InvalidIndex)
            {
                [encoder setVisibilityResultMode:MTLVisibilityResultModeBoolean offset:index * QueryBUfferElemeentAlign];
            }
            else
            {
                [encoder setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:0];
            }
        }
        break;

        case CMD_SET_QUERY_BUFFER:
            break; // do NOTHING

        case CMD_SET_FRAGMENT_PROG_CONST_BUFFER:
        {
            Handle buffer = static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd)->buffer;
            unsigned index = static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd)->bufIndex;
            uintptr_t inst = reinterpret_cast<uintptr_t>(static_cast<const SWCommand_SetFragmentProgConstBuffer*>(cmd)->inst);
            unsigned instOffset = static_cast<unsigned>(inst);
            ConstBufferMetal::SetToRHI(buffer, index, instOffset, encoder);
        }
        break;

        case CMD_SET_FRAGMENT_TEXTURE:
        {
            Handle tex = static_cast<const SWCommand_SetFragmentTexture*>(cmd)->tex;
            unsigned unitIndex = static_cast<const SWCommand_SetFragmentTexture*>(cmd)->unitIndex;

            TextureMetal::SetToRHIFragment(tex, unitIndex, encoder);
            StatSet::IncStat(stat_SET_TEX, 1);
        }
        break;

        case CMD_SET_DEPTHSTENCIL_STATE:
        {
            DepthStencilStateMetal::SetToRHI(static_cast<const SWCommand_SetDepthStencilState*>(cmd)->depthStencilState, encoder);
        }
        break;

        case CMD_SET_SAMPLER_STATE:
        {
            SamplerStateMetal::SetToRHI(static_cast<const SWCommand_SetSamplerState*>(cmd)->samplerState, encoder);
            StatSet::IncStat(stat_SET_SS, 1);
        }
        break;

        case CMD_DRAW_PRIMITIVE:
        {
            MTLPrimitiveType ptype = MTLPrimitiveType(static_cast<const SWCommand_DrawPrimitive*>(cmd)->mode);
            unsigned vertexCount = static_cast<const SWCommand_DrawPrimitive*>(cmd)->vertexCount;

            _ApplyVertexData();
            [encoder drawPrimitives:ptype vertexStart:0 vertexCount:vertexCount];
        }
        break;

        case CMD_DRAW_INDEXED_PRIMITIVE:
        {
            MTLPrimitiveType ptype = MTLPrimitiveType(static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->mode);
            unsigned indexCount = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->indexCount;
            unsigned firstVertex = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->firstVertex;
            unsigned startIndex = static_cast<const SWCommand_DrawIndexedPrimitive*>(cmd)->startIndex;

            unsigned ib_base = 0;
            id<MTLBuffer> ib = IndexBufferMetal::GetBuffer(cur_ib, &ib_base);
            MTLIndexType i_type = IndexBufferMetal::GetType(cur_ib);
            unsigned i_off = (i_type == MTLIndexTypeUInt16) ? startIndex * sizeof(uint16) : startIndex * sizeof(uint32);

            _ApplyVertexData(firstVertex);
            [encoder drawIndexedPrimitives:ptype indexCount:indexCount indexType:i_type indexBuffer:ib indexBufferOffset:ib_base + i_off];
        }
        break;

        case CMD_DRAW_INSTANCED_PRIMITIVE:
        {
            MTLPrimitiveType ptype = MTLPrimitiveType(static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->mode);
            unsigned vertexCount = static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->vertexCount;
            unsigned instCount = static_cast<const SWCommand_DrawInstancedPrimitive*>(cmd)->instanceCount;

            _ApplyVertexData();
            [encoder drawPrimitives:ptype vertexStart:0 vertexCount:vertexCount instanceCount:instCount];
        }
        break;

        case CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE:
        {
            MTLPrimitiveType ptype = MTLPrimitiveType(static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->mode);
            unsigned firstVertex = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->firstVertex;
            unsigned indexCount = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->indexCount;
            unsigned startIndex = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->startIndex;
            unsigned instCount = static_cast<const SWCommand_DrawInstancedIndexedPrimitive*>(cmd)->instanceCount;

            unsigned ib_base = 0;
            id<MTLBuffer> ib = IndexBufferMetal::GetBuffer(cur_ib, &ib_base);
            MTLIndexType i_type = IndexBufferMetal::GetType(cur_ib);
            unsigned i_off = (i_type == MTLIndexTypeUInt16) ? startIndex * sizeof(uint16) : startIndex * sizeof(uint32);

            _ApplyVertexData(firstVertex);
            [encoder drawIndexedPrimitives:ptype indexCount:indexCount indexType:i_type indexBuffer:ib indexBufferOffset:ib_base + i_off instanceCount:instCount];
        }
        break;

        default:
            Logger::Error("unsupported command: %d", cmd->type);
            DVASSERT(false, "unsupported command");
        }

        if (cmd->type == CMD_END)
            break;
        c += cmd->size;
    }
}
#endif

//------------------------------------------------------------------------------

void SetRenderPassAttachments(MTLRenderPassDescriptor* desc, const RenderPassConfig& cfg, bool& ds_used)
{
    bool usingMSAA = cfg.UsingMSAA();

    for (unsigned i = 0; i != countof(cfg.colorBuffer); ++i)
    {
        switch (cfg.colorBuffer[i].loadAction)
        {
        case LOADACTION_CLEAR:
            desc.colorAttachments[i].loadAction = MTLLoadActionClear;
            break;
        case LOADACTION_LOAD:
            desc.colorAttachments[i].loadAction = MTLLoadActionLoad;
            break;
        default:
            desc.colorAttachments[i].loadAction = MTLLoadActionDontCare;
        }

        desc.colorAttachments[i].storeAction = usingMSAA ? MTLStoreActionMultisampleResolve : MTLStoreActionStore;

        desc.colorAttachments[i].clearColor = MTLClearColorMake(cfg.colorBuffer[i].clearColor[0], cfg.colorBuffer[i].clearColor[1], cfg.colorBuffer[i].clearColor[2], cfg.colorBuffer[i].clearColor[3]);

        if (cfg.colorBuffer[i].texture != InvalidHandle)
        {
            if (usingMSAA)
            {
                TextureMetal::SetAsRenderTarget(cfg.colorBuffer[i].multisampleTexture, desc);
                TextureMetal::SetAsResolveRenderTarget(cfg.colorBuffer[i].texture, desc);
            }
            else
            {
                TextureMetal::SetAsRenderTarget(cfg.colorBuffer[i].texture, desc, i);
            }
        }
        else
        {
            if (usingMSAA)
            {
                if (i == 0)
                {
                    DVASSERT(cfg.colorBuffer[i].multisampleTexture != InvalidHandle);
                    TextureMetal::SetAsRenderTarget(cfg.colorBuffer[i].multisampleTexture, desc);
                    desc.colorAttachments[0].resolveTexture = _Metal_currentDrawable.texture;
                }
            }
            else
            {
                desc.colorAttachments[i].texture = _Metal_currentDrawable.texture;
            }
        }

        if (cfg.colorBuffer[i].texture == InvalidHandle)
            break;
    }

    if (cfg.depthStencilBuffer.texture == rhi::DefaultDepthBuffer)
    {
        if (usingMSAA)
        {
            TextureMetal::SetAsDepthStencil(cfg.depthStencilBuffer.multisampleTexture, desc);
            desc.depthAttachment.resolveTexture = _Metal_DefDepthBuf;
            desc.stencilAttachment.resolveTexture = _Metal_DefStencilBuf;
        }
        else
        {
            desc.depthAttachment.texture = _Metal_DefDepthBuf;
            desc.stencilAttachment.texture = _Metal_DefStencilBuf;
        }
        ds_used = true;
    }
    else if (cfg.depthStencilBuffer.texture != rhi::InvalidHandle)
    {
        if (usingMSAA)
        {
            TextureMetal::SetAsDepthStencil(cfg.depthStencilBuffer.multisampleTexture, desc);
            TextureMetal::SetAsResolveDepthStencil(cfg.depthStencilBuffer.texture, desc);
        }
        else
        {
            TextureMetal::SetAsDepthStencil(cfg.depthStencilBuffer.texture, desc);
        }
        ds_used = true;
    }

    if (ds_used)
    {
        desc.depthAttachment.loadAction = (cfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR) ? MTLLoadActionClear : MTLLoadActionDontCare;
        desc.stencilAttachment.loadAction = (cfg.depthStencilBuffer.loadAction == LOADACTION_CLEAR) ? MTLLoadActionClear : MTLLoadActionDontCare;
        desc.depthAttachment.clearDepth = cfg.depthStencilBuffer.clearDepth;
        desc.stencilAttachment.clearStencil = cfg.depthStencilBuffer.clearStencil;

        switch (cfg.depthStencilBuffer.storeAction)
        {
        case STOREACTION_STORE:
            DVASSERT(!usingMSAA);
            desc.depthAttachment.storeAction = MTLStoreActionStore;
            desc.stencilAttachment.storeAction = MTLStoreActionStore;
            break;

        case STOREACTION_NONE:
            desc.depthAttachment.storeAction = MTLStoreActionDontCare;
            desc.depthAttachment.resolveTexture = nil;
            desc.stencilAttachment.storeAction = MTLStoreActionDontCare;
            desc.stencilAttachment.resolveTexture = nil;
            break;

        case STOREACTION_RESOLVE:
            DVASSERT(usingMSAA);
            desc.depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
            desc.stencilAttachment.storeAction = MTLStoreActionMultisampleResolve;
            break;

        default:
            DVASSERT(0, "Invalid store action specified.");
            break;
        }
    }
}

void CheckDefaultBuffers()
{
    DAVA::LockGuard<DAVA::Mutex> guard(_Metal_ResetSync);

    if (_Metal_ResetPending)
    {
        if (_Metal_DefDepthBuf)
        {
            [_Metal_DefDepthBuf release];
            _Metal_DefDepthBuf = nil;
        }

        if (_Metal_DefStencilBuf)
        {
            [_Metal_DefStencilBuf release];
            _Metal_DefStencilBuf = nil;
        }

        NSUInteger width = _Metal_ResetParam.width;
        NSUInteger height = _Metal_ResetParam.height;

        MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:width height:height mipmapped:NO];
        MTLTextureDescriptor* stencilDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8 width:width height:height mipmapped:NO];

        _Metal_DefDepthBuf = [_Metal_Device newTextureWithDescriptor:depthDesc];
        _Metal_DefStencilBuf = [_Metal_Device newTextureWithDescriptor:stencilDesc];
        _Metal_Layer.drawableSize = CGSizeMake(width, height);

        _Metal_ResetPending = false;
    }
}
    
#if !RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

bool RenderPassMetal_t::Initialize()
{
    bool need_drawable = cfg.colorBuffer[0].texture == InvalidHandle;
    if (need_drawable && !_Metal_currentDrawable)
    {
        if (_Metal_DrawableDispatchSemaphore != nullptr)
            _Metal_DrawableDispatchSemaphore->Wait();

        @autoreleasepool
        {
            CheckDefaultBuffers();

            _Metal_currentDrawable = [[_Metal_Layer nextDrawable] retain];
            _Metal_DefFrameBuf = _Metal_currentDrawable.texture;
        }
    }

    if (need_drawable && !_Metal_currentDrawable)
    {
        for (unsigned i = 0; i != cmdBuf.size(); ++i)
        {
            CommandBufferPoolMetal::Free(cmdBuf[i]);
        }
        cmdBuf.clear();

        MTL_TRACE("-rp.init failed (no drawable)");
        return false;
    }

    bool ds_used = false;

    desc = [MTLRenderPassDescriptor renderPassDescriptor];

    SetRenderPassAttachments(desc, cfg, ds_used);

    if (cfg.queryBuffer != InvalidHandle)
    {
        desc.visibilityResultBuffer = QueryBufferMetal::GetBuffer(cfg.queryBuffer);
    }

    commandBuffer = [_Metal_DefCmdQueue commandBuffer];
    [commandBuffer retain];
    if (cmdBuf.size() == 1)
    {
        CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf[0]);

        encoder = nil;
        cb->encoder = [commandBuffer renderCommandEncoderWithDescriptor:desc];
        [cb->encoder retain];

        cb->rt = desc.colorAttachments[0].texture;
        cb->color_count = 0;
        for (unsigned t = 0; t != MAX_RENDER_TARGET_COUNT; ++t)
        {
            cb->color_fmt[t] = desc.colorAttachments[t].texture.pixelFormat;

            if (cfg.colorBuffer[t].texture == InvalidHandle)
            {
                if (t == 0)
                    cb->color_count = 1;
                break;
            }
            else
            {
                ++cb->color_count;
            }
        }

        cb->ds_used = ds_used;
        cb->cur_ib = InvalidHandle;
        cb->cur_vstream_count = 0;
        cb->sampleCount = rhi::TextureSampleCountForAAType(cfg.antialiasingType);
        for (unsigned s = 0; s != countof(cb->cur_vb); ++s)
            cb->cur_vb[s] = InvalidHandle;
    }
    else
    {
        encoder = [commandBuffer parallelRenderCommandEncoderWithDescriptor:desc];
        [encoder retain];

        for (unsigned i = 0; i != cmdBuf.size(); ++i)
        {
            CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf[i]);

            cb->encoder = [encoder renderCommandEncoder];
            [cb->encoder retain];
            cb->buf = commandBuffer;
            cb->rt = desc.colorAttachments[0].texture;
            cb->color_count = 0;
            for (unsigned t = 0; t != MAX_RENDER_TARGET_COUNT; ++t)
            {
                cb->color_fmt[t] = desc.colorAttachments[t].texture.pixelFormat;

                if (cfg.colorBuffer[t].texture == InvalidHandle)
                {
                    if (t == 0)
                        cb->color_count = 1;
                    break;
                }
                else
                {
                    ++cb->color_count;
                }
            }
            cb->ds_used = ds_used;
            cb->cur_ib = InvalidHandle;
            cb->cur_vstream_count = 0;
            cb->sampleCount = rhi::TextureSampleCountForAAType(cfg.antialiasingType);
            for (unsigned s = 0; s != countof(cb->cur_vb); ++s)
                cb->cur_vb[s] = InvalidHandle;
        }
    }

    return true;
}

#endif

//------------------------------------------------------------------------------

static Handle metal_RenderPass_Allocate(const RenderPassConfig& passConf, uint32 cmdBufCount, Handle* cmdBuf)
{
    DVASSERT(passConf.IsValid());
    DVASSERT(cmdBufCount);
    

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    bool need_drawable = passConf.colorBuffer[0].texture == InvalidHandle && !_Metal_currentDrawable;

    if (need_drawable)
    {
        @autoreleasepool
        {
            CheckDefaultBuffers();

            _Metal_currentDrawable = [[_Metal_Layer nextDrawable] retain];
            _Metal_DefFrameBuf = _Metal_currentDrawable.texture;

            MTL_TRACE(" next.drawable= %p %i %s", (void*)(_Metal_currentDrawable), [_Metal_currentDrawable retainCount], NSStringFromClass([_Metal_currentDrawable class]).UTF8String);
        }
    }

    if (need_drawable && !_Metal_currentDrawable)
    {
        for (unsigned i = 0; i != cmdBufCount; ++i)
            cmdBuf[i] = InvalidHandle;

        MTL_TRACE("-rp.alloc InvalidHande (no drawable)");
        return InvalidHandle;
    }

    Handle pass_h = RenderPassPoolMetal::Alloc();
    MTL_TRACE("-rp.alloc %u (%u)", RHI_HANDLE_INDEX(pass_h), cmdBufCount);
    RenderPassMetal_t* pass = RenderPassPoolMetal::Get(pass_h);
    bool ds_used = false;
    pass->desc = [MTLRenderPassDescriptor renderPassDescriptor];

    SetRenderPassAttachments(pass->desc, passConf, ds_used);

    if (passConf.queryBuffer != InvalidHandle)
    {
        pass->desc.visibilityResultBuffer = QueryBufferMetal::GetBuffer(passConf.queryBuffer);
    }

    pass->cmdBuf.resize(cmdBufCount);
    pass->priority = passConf.priority;
    pass->commandBuffer = [_Metal_DefCmdQueue commandBuffer];
    [pass->commandBuffer retain];

    if (cmdBufCount == 1)
    {
        Handle cb_h = CommandBufferPoolMetal::Alloc();
        CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cb_h);

        pass->encoder = nil;

        cb->encoder = [pass->commandBuffer renderCommandEncoderWithDescriptor:pass->desc];
        [cb->encoder retain];

        cb->buf = pass->commandBuffer;
        cb->rt = pass->desc.colorAttachments[0].texture;
        cb->color_fmt = pass->desc.colorAttachments[0].texture.pixelFormat;
        cb->ds_used = ds_used;
        cb->cur_ib = InvalidHandle;
        cb->cur_vstream_count = 0;
        cb->sampleCount = passConf.sampleCount;
        for (unsigned s = 0; s != countof(cb->cur_vb); ++s)
            cb->cur_vb[s] = InvalidHandle;

        pass->cmdBuf[0] = cb_h;
        cmdBuf[0] = cb_h;
    }
    else
    {
        pass->encoder = [pass->commandBuffer parallelRenderCommandEncoderWithDescriptor:pass->desc];
        [pass->encoder retain];

        for (unsigned i = 0; i != cmdBufCount; ++i)
        {
            Handle cb_h = CommandBufferPoolMetal::Alloc();
            CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cb_h);

            cb->encoder = [pass->encoder renderCommandEncoder];
            [cb->encoder retain];
            cb->buf = pass->commandBuffer;
            cb->rt = pass->desc.colorAttachments[0].texture;
            cb->color_fmt = pass->desc.colorAttachments[0].texture.pixelFormat;
            cb->ds_used = ds_used;
            cb->cur_ib = InvalidHandle;
            cb->cur_vstream_count = 0;
            cb->sampleCount = passConf.sampleCount;
            for (unsigned s = 0; s != countof(cb->cur_vb); ++s)
                cb->cur_vb[s] = InvalidHandle;

            pass->cmdBuf[i] = cb_h;
            cmdBuf[i] = cb_h;
        }
    }

    return pass_h;
    
#else

    Handle pass_h = RenderPassPoolMetal::Alloc();
    MTL_TRACE("-rp.alloc %u (%u)", RHI_HANDLE_INDEX(pass_h), cmdBufCount);
    RenderPassMetal_t* pass = RenderPassPoolMetal::Get(pass_h);

    pass->cfg = passConf;
    pass->priority = passConf.priority;

    pass->cmdBuf.resize(cmdBufCount);
    for (unsigned i = 0; i != cmdBufCount; ++i)
    {
        Handle cb_h = CommandBufferPoolMetal::Alloc();
        CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cb_h);

        cb->ds_used = passConf.depthStencilBuffer.texture != rhi::InvalidHandle;

        pass->cmdBuf[i] = cb_h;
        cmdBuf[i] = cb_h;
    }

    return pass_h;

#endif
}

static void metal_RenderPass_Begin(Handle pass_h)
{
    MTL_TRACE(" -rp.begin %u", RHI_HANDLE_INDEX(pass_h));
    RenderPassMetal_t* pass = RenderPassPoolMetal::Get(pass_h);
}

static void metal_RenderPass_End(Handle pass_h)
{
    MTL_TRACE(" -rp.end %u", RHI_HANDLE_INDEX(pass_h));
    RenderPassMetal_t* pass = RenderPassPoolMetal::Get(pass_h);    

    #if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    if (pass->cmdBuf.size() > 1)
    {
        [pass->encoder endEncoding];
    }
    #endif
    MTL_TRACE("  drawable %p %i", (void*)(_Metal_currentDrawable), [_Metal_currentDrawable retainCount]);
}

namespace RenderPassMetal
{
void Init(uint32 maxCount)
{
    RenderPassPoolMetal::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Renderpass_Allocate = &metal_RenderPass_Allocate;
    dispatch->impl_Renderpass_Begin = &metal_RenderPass_Begin;
    dispatch->impl_Renderpass_End = &metal_RenderPass_End;
}
}

//------------------------------------------------------------------------------

static void metal_CommandBuffer_Begin(Handle cmdBuf)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

    cb->cur_vstream_count = 0;
    for (unsigned s = 0; s != countof(cb->cur_vb); ++s)
        cb->cur_vb[s] = InvalidHandle;

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    [cb->encoder setDepthStencilState:_Metal_DefDepthState];
#else
    cb->curUsedSize = 0;
    cb->allocCmd<SWCommand_Begin>();
#endif
}

//------------------------------------------------------------------------------

static void metal_CommandBuffer_End(Handle cmdBuf, Handle syncObject)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    [cb->encoder endEncoding];

    if (syncObject != InvalidHandle)
    {
        [cb->buf addCompletedHandler:^(id<MTLCommandBuffer> cmdb) {
          SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(syncObject);

          sync->is_signaled = true;
        }];
    }
    
#else
    SWCommand_End* cmd = cb->allocCmd<SWCommand_End>();
    cmd->syncObject = syncObject;
#endif
}

//------------------------------------------------------------------------------

static void metal_CommandBuffer_SetPipelineState(Handle cmdBuf, Handle ps, uint32 layoutUID)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    cb->cur_stride = PipelineStateMetal::SetToRHI(ps, layoutUID, cb->color_fmt, cb->ds_used, cb->encoder, cb->sampleCount);
    cb->cur_vstream_count = PipelineStateMetal::VertexStreamCount(ps);
    StatSet::IncStat(stat_SET_PS, 1);
#else
    SWCommand_SetPipelineState* cmd = cb->allocCmd<SWCommand_SetPipelineState>();
    cmd->ps = ps;
    cmd->vdecl = layoutUID;
#endif
}

//------------------------------------------------------------------------------

static void metal_CommandBuffer_SetCullMode(Handle cmdBuf, CullMode mode)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    switch (mode)
    {
    case CULL_NONE:
        [cb->encoder setCullMode:MTLCullModeNone];
        break;

    case CULL_CCW:
        [cb->encoder setFrontFacingWinding:MTLWindingClockwise];
        [cb->encoder setCullMode:MTLCullModeBack];
        break;

    case CULL_CW:
        [cb->encoder setFrontFacingWinding:MTLWindingClockwise];
        [cb->encoder setCullMode:MTLCullModeFront];
        break;
    }
#else
    SWCommand_SetCullMode* cmd = cb->allocCmd<SWCommand_SetCullMode>();
    cmd->mode = mode;
#endif
}

//------------------------------------------------------------------------------

static void metal_CommandBuffer_SetScissorRect(Handle cmdBuf, ScissorRect rect)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    MTLScissorRect rc;
    if (!(rect.x == 0 && rect.y == 0 && rect.width == 0 && rect.height == 0))
    {
        unsigned max_x = (cb->rt) ? unsigned(cb->rt.width) : unsigned(_Metal_DefFrameBuf.width);
        unsigned max_y = (cb->rt) ? unsigned(cb->rt.height) : unsigned(_Metal_DefFrameBuf.height);

        rc.x = rect.x;
        rc.y = rect.y;
        rc.width = (rect.x + rect.width > max_x) ? (max_x - rc.x) : rect.width;
        rc.height = (rect.y + rect.height > max_y) ? (max_y - rc.y) : rect.height;

        if (rc.width == 0)
        {
            rc.width = 1;
            if (rc.x > 0)
                --rc.x;
        }

        if (rc.height == 0)
        {
            rc.height = 1;
            if (rc.y > 0)
                --rc.y;
        }
    }
    else
    {
        rc.x = 0;
        rc.y = 0;
        if (cb->rt)
        {
            rc.width = cb->rt.width;
            rc.height = cb->rt.height;
        }
        else
        {
            rc.width = _Metal_DefFrameBuf.width;
            rc.height = _Metal_DefFrameBuf.height;
        }
    }

    [cb->encoder setScissorRect:rc];

#else

    int x = rect.x;
    int y = rect.y;
    int w = rect.width;
    int h = rect.height;
    SWCommand_SetScissorRect* cmd = cb->allocCmd<SWCommand_SetScissorRect>();
    cmd->x = x;
    cmd->y = y;
    cmd->width = w;
    cmd->height = h;
    
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetViewport(Handle cmdBuf, Viewport viewport)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    MTLViewport vp;
    if (!(viewport.x == 0 && viewport.y == 0 && viewport.width == 0 && viewport.height == 0))
    {
        vp.originX = viewport.x;
        vp.originY = viewport.y;
        vp.width = viewport.width;
        vp.height = viewport.height;
        vp.znear = 0.0;
        vp.zfar = 1.0;
    }
    else
    {
        vp.originX = 0;
        vp.originY = 0;
        vp.width = cb->rt.width;
        vp.height = cb->rt.height;
        vp.znear = 0.0;
        vp.zfar = 1.0;
    }

    [cb->encoder setViewport:vp];

#else

    int x = viewport.x;
    int y = viewport.y;
    int w = viewport.width;
    int h = viewport.height;
    SWCommand_SetViewport* cmd = cb->allocCmd<SWCommand_SetViewport>();
    cmd->x = x;
    cmd->y = y;
    cmd->width = w;
    cmd->height = h;
    

#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetFillMode(Handle cmdBuf, FillMode mode)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    [cb->encoder setTriangleFillMode:(mode == FILLMODE_WIREFRAME) ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
#else
    SWCommand_SetFillMode* cmd = cb->allocCmd<SWCommand_SetFillMode>();
    cmd->mode = mode;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetVertexData(Handle cmdBuf, Handle vb, uint32 streamIndex)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    cb->cur_vb[streamIndex] = vb;
    StatSet::IncStat(stat_SET_VB, 1);
#else
    SWCommand_SetVertexData* cmd = cb->allocCmd<SWCommand_SetVertexData>();
    cmd->vb = vb;
    cmd->streamIndex = streamIndex;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetVertexConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    ConstBufferMetal::SetToRHI(buffer, bufIndex, cb->encoder);
    StatSet::IncStat(stat_SET_CB, 1);
#else
    SWCommand_SetVertexProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetVertexProgConstBuffer>();
    cmd->bufIndex = bufIndex;
    cmd->buffer = buffer;
    uintptr_t instOffset = static_cast<uintptr_t>(ConstBufferMetal::Instance(buffer));
    cmd->inst = reinterpret_cast<void*>(instOffset);
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetVertexTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    TextureMetal::SetToRHIVertex(tex, unitIndex, cb->encoder);
    StatSet::IncStat(stat_SET_TEX, 1);
#else
    SWCommand_SetVertexTexture* cmd = cb->allocCmd<SWCommand_SetVertexTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetIndices(Handle cmdBuf, Handle ib)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    cb->cur_ib = ib;
    StatSet::IncStat(stat_SET_IB, 1);
#else
    SWCommand_SetIndices* cmd = cb->allocCmd<SWCommand_SetIndices>();
    cmd->ib = ib;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetQueryIndex(Handle cmdBuf, uint32 objectIndex)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    if (objectIndex != DAVA::InvalidIndex)
    {
        [cb->encoder setVisibilityResultMode:MTLVisibilityResultModeBoolean offset:objectIndex * QueryBUfferElemeentAlign];
    }
    else
    {
        [cb->encoder setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:0];
    }
#else
    SWCommand_SetQueryIndex* cmd = cb->allocCmd<SWCommand_SetQueryIndex>();
    cmd->objectIndex = objectIndex;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetQueryBuffer(Handle /*cmdBuf*/, Handle /*queryBuf*/)
{
    // do NOTHING, since query-buffer specified in render-pass
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetFragmentConstBuffer(Handle cmdBuf, uint32 bufIndex, Handle buffer)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

    DVASSERT(bufIndex < MAX_CONST_BUFFER_COUNT);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    ConstBufferMetal::SetToRHI(buffer, bufIndex, cb->encoder);
    StatSet::IncStat(stat_SET_CB, 1);
#else
    SWCommand_SetFragmentProgConstBuffer* cmd = cb->allocCmd<SWCommand_SetFragmentProgConstBuffer>();
    cmd->bufIndex = bufIndex;
    cmd->buffer = buffer;
    uintptr_t instOffset = static_cast<uintptr_t>(ConstBufferMetal::Instance(buffer));
    cmd->inst = reinterpret_cast<void*>(instOffset);
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetFragmentTexture(Handle cmdBuf, uint32 unitIndex, Handle tex)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    TextureMetal::SetToRHIFragment(tex, unitIndex, cb->encoder);
    StatSet::IncStat(stat_SET_TEX, 1);
#else
    SWCommand_SetFragmentTexture* cmd = cb->allocCmd<SWCommand_SetFragmentTexture>();
    cmd->unitIndex = unitIndex;
    cmd->tex = tex;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetDepthStencilState(Handle cmdBuf, Handle depthStencilState)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    DepthStencilStateMetal::SetToRHI(depthStencilState, cb->encoder);
#else
    SWCommand_SetDepthStencilState* cmd = cb->allocCmd<SWCommand_SetDepthStencilState>();
    cmd->depthStencilState = depthStencilState;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetSamplerState(Handle cmdBuf, const Handle samplerState)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    SamplerStateMetal::SetToRHI(samplerState, cb->encoder);
    StatSet::IncStat(stat_SET_SS, 1);
#else
    SWCommand_SetSamplerState* cmd = cb->allocCmd<SWCommand_SetSamplerState>();
    cmd->samplerState = samplerState;
#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_DrawPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);
    MTLPrimitiveType ptype = MTLPrimitiveTypeTriangle;
    unsigned v_cnt = 0;

    switch (type)
    {
    case PRIMITIVE_TRIANGLELIST:
        ptype = MTLPrimitiveTypeTriangle;
        v_cnt = count * 3;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        ptype = MTLPrimitiveTypeTriangleStrip;
        v_cnt = 2 + count;
        break;

    case PRIMITIVE_LINELIST:
        ptype = MTLPrimitiveTypeLine;
        v_cnt = count * 2;
        break;
    }

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    cb->_ApplyVertexData();
    [cb->encoder drawPrimitives:ptype vertexStart:0 vertexCount:v_cnt];

    StatSet::IncStat(stat_DP, 1);
    switch (ptype)
    {
    case MTLPrimitiveTypeTriangle:
        StatSet::IncStat(stat_DTL, 1);
        break;
    case MTLPrimitiveTypeTriangleStrip:
        StatSet::IncStat(stat_DTS, 1);
        break;
    case MTLPrimitiveTypeLine:
        StatSet::IncStat(stat_DLL, 1);
        break;
    default:
        break;
    }

#else

    SWCommand_DrawPrimitive* cmd = cb->allocCmd<SWCommand_DrawPrimitive>();

    cmd->mode = ptype;
    cmd->vertexCount = v_cnt;

#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_DrawIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 count, uint32 /*vertexCount*/, uint32 firstVertex, uint32 startIndex)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);
    MTLPrimitiveType ptype = MTLPrimitiveTypeTriangle;
    unsigned i_cnt = 0;

    switch (type)
    {
    case PRIMITIVE_TRIANGLELIST:
        ptype = MTLPrimitiveTypeTriangle;
        i_cnt = count * 3;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        ptype = MTLPrimitiveTypeTriangleStrip;
        i_cnt = 2 + count;
        break;

    case PRIMITIVE_LINELIST:
        ptype = MTLPrimitiveTypeLine;
        i_cnt = count * 2;
        break;
    }

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    unsigned ib_base = 0;
    id<MTLBuffer> ib = IndexBufferMetal::GetBuffer(cb->cur_ib, &ib_base);
    MTLIndexType i_type = IndexBufferMetal::GetType(cb->cur_ib);
    unsigned i_off = (i_type == MTLIndexTypeUInt16) ? startIndex * sizeof(uint16) : startIndex * sizeof(uint32);

    cb->_ApplyVertexData(firstVertex);
    [cb->encoder drawIndexedPrimitives:ptype indexCount:i_cnt indexType:i_type indexBuffer:ib indexBufferOffset:ib_base + i_off];

    StatSet::IncStat(stat_DIP, 1);
    switch (ptype)
    {
    case MTLPrimitiveTypeTriangle:
        StatSet::IncStat(stat_DTL, 1);
        break;
    case MTLPrimitiveTypeTriangleStrip:
        StatSet::IncStat(stat_DTS, 1);
        break;
    case MTLPrimitiveTypeLine:
        StatSet::IncStat(stat_DLL, 1);
        break;
    default:
        break;
    }

#else

    SWCommand_DrawIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawIndexedPrimitive>();

    cmd->mode = ptype;
    cmd->indexCount = i_cnt;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;

#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_DrawInstancedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 inst_count, uint32 prim_count)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);
    MTLPrimitiveType ptype = MTLPrimitiveTypeTriangle;
    unsigned v_cnt = 0;

    switch (type)
    {
    case PRIMITIVE_TRIANGLELIST:
        ptype = MTLPrimitiveTypeTriangle;
        v_cnt = prim_count * 3;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        ptype = MTLPrimitiveTypeTriangleStrip;
        v_cnt = 2 + prim_count;
        break;

    case PRIMITIVE_LINELIST:
        ptype = MTLPrimitiveTypeLine;
        v_cnt = prim_count * 2;
        break;
    }

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    cb->_ApplyVertexData();
    [cb->encoder drawPrimitives:ptype vertexStart:0 vertexCount:v_cnt instanceCount:inst_count];

    StatSet::IncStat(stat_DP, 1);
    switch (ptype)
    {
    case MTLPrimitiveTypeTriangle:
        StatSet::IncStat(stat_DTL, 1);
        break;
    case MTLPrimitiveTypeTriangleStrip:
        StatSet::IncStat(stat_DTS, 1);
        break;
    case MTLPrimitiveTypeLine:
        StatSet::IncStat(stat_DLL, 1);
        break;
    default:
        break;
    }

#else

    SWCommand_DrawInstancedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedPrimitive>();

    cmd->mode = ptype;
    cmd->instanceCount = inst_count;
    cmd->vertexCount = v_cnt;

#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_DrawInstancedIndexedPrimitive(Handle cmdBuf, PrimitiveType type, uint32 instCount, uint32 prim_count, uint32 /*vertexCount*/, uint32 firstVertex, uint32 startIndex, uint32 baseInst)
{
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);
    MTLPrimitiveType ptype = MTLPrimitiveTypeTriangle;
    unsigned i_cnt = 0;

    switch (type)
    {
    case PRIMITIVE_TRIANGLELIST:
        ptype = MTLPrimitiveTypeTriangle;
        i_cnt = prim_count * 3;
        break;

    case PRIMITIVE_TRIANGLESTRIP:
        ptype = MTLPrimitiveTypeTriangleStrip;
        i_cnt = 2 + prim_count;
        break;

    case PRIMITIVE_LINELIST:
        ptype = MTLPrimitiveTypeLine;
        i_cnt = prim_count * 2;
        break;
    }

#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    unsigned ib_base = 0;
    id<MTLBuffer> ib = IndexBufferMetal::GetBuffer(cb->cur_ib, &ib_base);
    MTLIndexType i_type = IndexBufferMetal::GetType(cb->cur_ib);
    unsigned i_off = (i_type == MTLIndexTypeUInt16) ? startIndex * sizeof(uint16) : startIndex * sizeof(uint32);

    cb->_ApplyVertexData(firstVertex);
    [cb->encoder drawIndexedPrimitives:ptype indexCount:i_cnt indexType:i_type indexBuffer:ib indexBufferOffset:ib_base + i_off instanceCount:instCount];

    StatSet::IncStat(stat_DIP, 1);
    switch (ptype)
    {
    case MTLPrimitiveTypeTriangle:
        StatSet::IncStat(stat_DTL, 1);
        break;
    case MTLPrimitiveTypeTriangleStrip:
        StatSet::IncStat(stat_DTS, 1);
        break;
    case MTLPrimitiveTypeLine:
        StatSet::IncStat(stat_DLL, 1);
        break;
    default:
        break;
    }
#else
    SWCommand_DrawInstancedIndexedPrimitive* cmd = cb->allocCmd<SWCommand_DrawInstancedIndexedPrimitive>();

    cmd->mode = ptype;
    cmd->indexCount = i_cnt;
    cmd->firstVertex = firstVertex;
    cmd->startIndex = startIndex;
    cmd->instanceCount = instCount;
    cmd->baseInstance = baseInst;

#endif
}

//------------------------------------------------------------------------------

static void
metal_CommandBuffer_SetMarker(Handle cmdBuf, const char* text)
{
#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
    NSString* txt = [[NSString alloc] initWithUTF8String:text];
    CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cmdBuf);
    [cb->encoder insertDebugSignpost:txt];
    [txt release];
#endif
}

//------------------------------------------------------------------------------

static Handle
metal_SyncObject_Create()
{
    DAVA::LockGuard<DAVA::Mutex> guard(_Metal_SyncObjectsSync);
    Handle handle = SyncObjectPoolMetal::Alloc();
    SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(handle);

    sync->is_signaled = false;

    return handle;
}

//------------------------------------------------------------------------------

static void
metal_SyncObject_Delete(Handle obj)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_Metal_SyncObjectsSync);
    SyncObjectPoolMetal::Free(obj);
}

//------------------------------------------------------------------------------

static bool
metal_SyncObject_IsSignaled(Handle obj)
{
    bool signaled = false;
    DAVA::LockGuard<DAVA::Mutex> guard(_Metal_SyncObjectsSync);
    if (SyncObjectPoolMetal::IsAlive(obj))
    {
        SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(obj);

        if (sync)
            signaled = sync->is_signaled;
    }
    else
    {
        signaled = true;
    }

    return signaled;
}

static void Metal_RejectFrame(const CommonImpl::Frame& frame)
{
#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS

    MTL_TRACE("  discard-frame %u", ++frame_n);

    for (unsigned i = 0; i != frame.pass.size(); ++i)
    {
        if (frame.pass[i] == InvalidHandle)
            continue;
        RenderPassMetal_t* rp = RenderPassPoolMetal::Get(frame.pass[i]);

        for (unsigned b = 0; b != rp->cmdBuf.size(); ++b)
        {
            Handle cbh = rp->cmdBuf[b];
            CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cbh);

            cb->buf = nil;
            [cb->encoder release];
            cb->encoder = nil;
            cb->rt = nil;
        }

        rp->desc = nullptr;

        [rp->commandBuffer release];
        rp->commandBuffer = nil;
        [rp->encoder release];
        rp->encoder = nil;
    }
#endif

    @autoreleasepool
    {
        for (unsigned i = 0; i != frame.pass.size(); ++i)
        {
            if (frame.pass[i] == InvalidHandle)
                continue;
            RenderPassMetal_t* rp = RenderPassPoolMetal::Get(frame.pass[i]);

            for (unsigned b = 0; b != rp->cmdBuf.size(); ++b)
            {
                Handle cbh = rp->cmdBuf[b];
                CommandBufferPoolMetal::Free(cbh);
            }
            rp->cmdBuf.clear();
            RenderPassPoolMetal::Free(frame.pass[i]);
        }

        if (_Metal_currentDrawable != nil)
        {
            [_Metal_currentDrawable release];
            _Metal_currentDrawable = nil;
            _Metal_DefFrameBuf = nil;
        }

        if (frame.sync != InvalidHandle)
        {
            DAVA::LockGuard<DAVA::Mutex> guard(_Metal_SyncObjectsSync);
            SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(frame.sync);
            sync->is_signaled = true;
        }
    }
}

//------------------------------------------------------------------------------

static void Metal_ExecuteQueuedCommands(const CommonImpl::Frame& frame)
{
    @autoreleasepool
    {
        MTL_TRACE("--present %u", frame.frameNumber);

//for some reason when device is locked receive nil in drawable even before getting notification - check this case and do nothing to prevent crashes
#if RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
        if (_Metal_currentDrawable == nil)
        {
            Metal_RejectFrame(frame);
            return;
        }
#endif

        // sort cmd-lists by priority - command buffer are to be committed in pass-priority order
        static std::vector<RenderPassMetal_t*> pass;
        Handle syncObject = frame.sync;
        pass.clear();
        for (unsigned i = 0; i != frame.pass.size(); ++i)
        {
            RenderPassMetal_t* rp = RenderPassPoolMetal::Get(frame.pass[i]);
            bool do_add = true;

            for (std::vector<RenderPassMetal_t *>::iterator p = pass.begin(), p_end = pass.end(); p != p_end; ++p)
            {
                if (rp->priority > (*p)->priority)
                {
                    pass.insert(p, 1, rp);
                    do_add = false;
                    break;
                }
            }

            if (do_add)
                pass.push_back(rp);
        }

        // commit everything here - software command buffer are executed priorly
        // also add completion handlers here as befor rp->Initialize we dont have command buffers / frame drawable, and after committing adding handlers is prohibited
        bool initOk = true;
        for (int32 i = 0, sz = pass.size(); i < sz; ++i)
        {
            RenderPassMetal_t* rp = pass[i];

#if !RHI_METAL__USE_NATIVE_COMMAND_BUFFERS
            //execute software command buffers
            initOk &= rp->Initialize();
            if (!initOk)
                break;
            for (unsigned b = 0; b != rp->cmdBuf.size(); ++b)
            {
                Handle cbh = rp->cmdBuf[b];
                CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cbh);
                cb->Execute();
            }
            if (rp->encoder != nil)
                [rp->encoder endEncoding];
#endif

            if (i == (sz - 1))
            {
                //present drawable adds completion handler that calls actual present
                [rp->commandBuffer presentDrawable:_Metal_currentDrawable];
                unsigned frame_n = frame.frameNumber;
                if (syncObject != InvalidHandle)
                {
                    [rp->commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb)
                                                           {
                                                             MTL_TRACE("  .frame %u complete", frame_n);
                                                             DAVA::LockGuard<DAVA::Mutex> guard(_Metal_SyncObjectsSync);
                                                             SyncObjectMetal_t* sync = SyncObjectPoolMetal::Get(syncObject);
                                                             sync->is_signaled = true;
                                                           }];
                }
                if (_Metal_DrawableDispatchSemaphore != nullptr)
                {
                    [rp->commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb)
                                                           {
                                                             _Metal_DrawableDispatchSemaphore->Post();
                                                           }];
                }
            }
            [rp->commandBuffer commit];
        }

        if (!initOk)
        {
            //for some reason when device is locked receive nil in drawable even before getting notification - check this case and do nothing to prevent crashes
            Metal_RejectFrame(frame);
            return;
        }

        //clear passes
        for (RenderPassMetal_t* rp : pass)
        {
            for (unsigned b = 0; b != rp->cmdBuf.size(); ++b)
            {
                Handle cbh = rp->cmdBuf[b];
                CommandBufferMetal_t* cb = CommandBufferPoolMetal::Get(cbh);

                cb->buf = nil;
                [cb->encoder release];
                cb->encoder = nil;
                cb->rt = nil;

                CommandBufferPoolMetal::Free(cbh);
            }

            rp->desc = nullptr;

            [rp->commandBuffer release];
            rp->commandBuffer = nil;
            if (rp->encoder != nil)
            {
                [rp->encoder release];
                rp->encoder = nil;
            }

            rp->cmdBuf.clear();
        }

        for (Handle p : frame.pass)
            RenderPassPoolMetal::Free(p);

        //release frame stuff
        [_Metal_currentDrawable release];
        _Metal_currentDrawable = nil;
        _Metal_DefFrameBuf = nil;
    }
}

static bool Metal_PresentBuffer()
{
    return true;
}

static void Metal_InvalidateFrameCache()
{
    ConstBufferMetal::InvalidateAllInstances();
    ConstBufferMetal::ResetRingBuffer();
}

static void Metal_Suspend()
{
    Logger::Debug(" ***** Metal_Suspend");
}

static void metal_Reset(const ResetParam& param)
{
    DAVA::LockGuard<DAVA::Mutex> guard(_Metal_ResetSync);

    _Metal_ResetParam = param;
    _Metal_ResetPending = true;
}

namespace CommandBufferMetal
{
void Init(uint32 maxCount)
{
    CommandBufferPoolMetal::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_CommandBuffer_Begin = &metal_CommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = &metal_CommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = &metal_CommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = &metal_CommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = &metal_CommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = &metal_CommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = &metal_CommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = &metal_CommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = &metal_CommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = &metal_CommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = &metal_CommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryBuffer = &metal_CommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_SetQueryIndex = &metal_CommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = &metal_CommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = &metal_CommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = &metal_CommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = &metal_CommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = &metal_CommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = &metal_CommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = &metal_CommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = &metal_CommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = &metal_CommandBuffer_SetMarker;

    dispatch->impl_SyncObject_Create = &metal_SyncObject_Create;
    dispatch->impl_SyncObject_Delete = &metal_SyncObject_Delete;
    dispatch->impl_SyncObject_IsSignaled = &metal_SyncObject_IsSignaled;

    dispatch->impl_ExecuteFrame = &Metal_ExecuteQueuedCommands;
    dispatch->impl_RejectFrame = &Metal_RejectFrame;
    dispatch->impl_PresentBuffer = &Metal_PresentBuffer;
    dispatch->impl_FinishFrame = &Metal_InvalidateFrameCache;
    dispatch->impl_FinishRendering = &Metal_Suspend;
    dispatch->impl_Reset = &metal_Reset;
}
}

} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
