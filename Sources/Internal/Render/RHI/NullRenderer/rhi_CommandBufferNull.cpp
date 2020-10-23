#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct RenderPassNull_t : public ResourceImpl<RenderPassNull_t, RenderPassConfig>
{
    std::vector<Handle> cmdBuf;
};
RHI_IMPL_RESOURCE(RenderPassNull_t, RenderPassConfig)

struct CommandBufferNull_t : public ResourceImpl<CommandBufferNull_t, CommandBuffer::Descriptor>
{
};
RHI_IMPL_RESOURCE(CommandBufferNull_t, CommandBuffer::Descriptor)

using RenderPassNullPool = ResourcePool<RenderPassNull_t, RESOURCE_RENDER_PASS, RenderPassConfig>;
using CommandBufferNullPool = ResourcePool<CommandBufferNull_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor>;

RHI_IMPL_POOL(RenderPassNull_t, RESOURCE_RENDER_PASS, RenderPassConfig, false);
RHI_IMPL_POOL(CommandBufferNull_t, RESOURCE_COMMAND_BUFFER, CommandBuffer::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_Renderpass_Allocate(const RenderPassConfig&, uint32 cmdBufCount, Handle* cmdBuf)
{
    Handle h = RenderPassNullPool::Alloc();
    RenderPassNull_t* self = RenderPassNullPool::Get(h);

    self->cmdBuf.resize(cmdBufCount);
    for (uint32 i = 0; i < cmdBufCount; ++i)
    {
        cmdBuf[i] = CommandBufferNullPool::Alloc();
        self->cmdBuf[i] = cmdBuf[i];
    }

    return h;
}

void null_Renderpass_Begin(Handle)
{
}

void null_Renderpass_End(Handle h)
{
    RenderPassNull_t* self = RenderPassNullPool::Get(h);
    for (Handle cbh : self->cmdBuf)
        CommandBufferNullPool::Free(cbh);
    self->cmdBuf.clear();

    RenderPassNullPool::Free(h);
}

//////////////////////////////////////////////////////////////////////////

void null_CommandBuffer_Begin(Handle)
{
}

void null_CommandBuffer_End(Handle, Handle)
{
}

void null_CommandBuffer_SetPipelineState(Handle, Handle, uint32 vdecl)
{
}

void null_CommandBuffer_SetCullMode(Handle, CullMode)
{
}

void null_CommandBuffer_SetScissorRect(Handle, ScissorRect)
{
}

void null_CommandBuffer_SetViewport(Handle, Viewport)
{
}

void null_CommandBuffer_SetFillMode(Handle, FillMode)
{
}

void null_CommandBuffer_SetVertexData(Handle, Handle, uint32)
{
}

void null_CommandBuffer_SetVertexConstBuffer(Handle, uint32, Handle)
{
}

void null_CommandBuffer_SetVertexTexture(Handle, uint32, Handle)
{
}

void null_CommandBuffer_SetIndices(Handle, Handle)
{
}

void null_CommandBuffer_SetQueryIndex(Handle, uint32)
{
}

void null_CommandBuffer_SetQueryBuffer(Handle, Handle)
{
}

void null_CommandBuffer_IssueTimestampQuery(Handle, Handle)
{
}

void null_CommandBuffer_SetFragmentConstBuffer(Handle, uint32, Handle)
{
}

void null_CommandBuffer_SetFragmentTexture(Handle, uint32, Handle)
{
}

void null_CommandBuffer_SetDepthStencilState(Handle, Handle)
{
}

void null_CommandBuffer_SetSamplerState(Handle, const Handle)
{
}

void null_CommandBuffer_DrawPrimitive(Handle, PrimitiveType, uint32)
{
}

void null_CommandBuffer_DrawIndexedPrimitive(Handle, PrimitiveType, uint32, uint32, uint32, uint32)
{
}

void null_CommandBuffer_DrawInstancedPrimitive(Handle, PrimitiveType, uint32, uint32)
{
}

void null_CommandBuffer_DrawInstancedIndexedPrimitive(Handle, PrimitiveType, uint32, uint32, uint32, uint32, uint32, uint32)
{
}

void null_CommandBuffer_SetMarker(Handle, const char*)
{
}

//////////////////////////////////////////////////////////////////////////

namespace RenderPassNull
{
void Init(uint32 maxCount)
{
    RenderPassNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Renderpass_Allocate = null_Renderpass_Allocate;
    dispatch->impl_Renderpass_Begin = null_Renderpass_Begin;
    dispatch->impl_Renderpass_End = null_Renderpass_End;
}
}

//////////////////////////////////////////////////////////////////////////

namespace CommandBufferNull
{
void Init(uint32 maxCount)
{
    CommandBufferNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_CommandBuffer_Begin = null_CommandBuffer_Begin;
    dispatch->impl_CommandBuffer_End = null_CommandBuffer_End;
    dispatch->impl_CommandBuffer_SetPipelineState = null_CommandBuffer_SetPipelineState;
    dispatch->impl_CommandBuffer_SetCullMode = null_CommandBuffer_SetCullMode;
    dispatch->impl_CommandBuffer_SetScissorRect = null_CommandBuffer_SetScissorRect;
    dispatch->impl_CommandBuffer_SetViewport = null_CommandBuffer_SetViewport;
    dispatch->impl_CommandBuffer_SetFillMode = null_CommandBuffer_SetFillMode;
    dispatch->impl_CommandBuffer_SetVertexData = null_CommandBuffer_SetVertexData;
    dispatch->impl_CommandBuffer_SetVertexConstBuffer = null_CommandBuffer_SetVertexConstBuffer;
    dispatch->impl_CommandBuffer_SetVertexTexture = null_CommandBuffer_SetVertexTexture;
    dispatch->impl_CommandBuffer_SetIndices = null_CommandBuffer_SetIndices;
    dispatch->impl_CommandBuffer_SetQueryIndex = null_CommandBuffer_SetQueryIndex;
    dispatch->impl_CommandBuffer_SetQueryBuffer = null_CommandBuffer_SetQueryBuffer;
    dispatch->impl_CommandBuffer_IssueTimestampQuery = null_CommandBuffer_IssueTimestampQuery;
    dispatch->impl_CommandBuffer_SetFragmentConstBuffer = null_CommandBuffer_SetFragmentConstBuffer;
    dispatch->impl_CommandBuffer_SetFragmentTexture = null_CommandBuffer_SetFragmentTexture;
    dispatch->impl_CommandBuffer_SetDepthStencilState = null_CommandBuffer_SetDepthStencilState;
    dispatch->impl_CommandBuffer_SetSamplerState = null_CommandBuffer_SetSamplerState;
    dispatch->impl_CommandBuffer_DrawPrimitive = null_CommandBuffer_DrawPrimitive;
    dispatch->impl_CommandBuffer_DrawIndexedPrimitive = null_CommandBuffer_DrawIndexedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedPrimitive = null_CommandBuffer_DrawInstancedPrimitive;
    dispatch->impl_CommandBuffer_DrawInstancedIndexedPrimitive = null_CommandBuffer_DrawInstancedIndexedPrimitive;
    dispatch->impl_CommandBuffer_SetMarker = null_CommandBuffer_SetMarker;
}
}
} //ns rhi