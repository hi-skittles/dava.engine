#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct ConstBufferNull_t : public ResourceImpl<ConstBufferNull_t, ConstBuffer::Descriptor>
{
};
RHI_IMPL_RESOURCE(ConstBufferNull_t, ConstBuffer::Descriptor)

struct PipelineStateNull_t : public ResourceImpl<PipelineStateNull_t, PipelineState::Descriptor>
{
};
RHI_IMPL_RESOURCE(PipelineStateNull_t, PipelineState::Descriptor)

using ConstBufferNullPool = ResourcePool<ConstBufferNull_t, RESOURCE_CONST_BUFFER, ConstBuffer::Descriptor>;
using PipelineStateNullPool = ResourcePool<PipelineStateNull_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor>;

RHI_IMPL_POOL(ConstBufferNull_t, RESOURCE_CONST_BUFFER, ConstBuffer::Descriptor, false);
RHI_IMPL_POOL(PipelineStateNull_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

bool null_ConstBuffer_SetConst(Handle, uint32, uint32, const float*)
{
    return true;
}

bool null_ConstBuffer_SetConst1fv(Handle, uint32, uint32, const float*, uint32)
{
    return true;
}

void null_ConstBuffer_Delete(Handle h)
{
    ConstBufferNullPool::Free(h);
}

//////////////////////////////////////////////////////////////////////////

Handle null_PipelineState_Create(const PipelineState::Descriptor&)
{
    return PipelineStateNullPool::Alloc();
}

void null_PipelineState_Delete(Handle h)
{
    PipelineStateNullPool::Free(h);
}

Handle null_PipelineState_CreateVertexConstBuffer(Handle, uint32)
{
    return ConstBufferNullPool::Alloc();
}

Handle null_PipelineState_CreateFragmentConstBuffer(Handle, uint32)
{
    return ConstBufferNullPool::Alloc();
}

//////////////////////////////////////////////////////////////////////////

namespace ConstBufferNull
{
void Init(uint32 maxCount)
{
    ConstBufferNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_ConstBuffer_SetConst = null_ConstBuffer_SetConst;
    dispatch->impl_ConstBuffer_SetConst1fv = null_ConstBuffer_SetConst1fv;
    dispatch->impl_ConstBuffer_Delete = null_ConstBuffer_Delete;
}
}

//////////////////////////////////////////////////////////////////////////

namespace PipelineStateNull
{
void Init(uint32 maxCount)
{
    PipelineStateNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PipelineState_Create = null_PipelineState_Create;
    dispatch->impl_PipelineState_Delete = null_PipelineState_Delete;
    dispatch->impl_PipelineState_CreateVertexConstBuffer = null_PipelineState_CreateVertexConstBuffer;
    dispatch->impl_PipelineState_CreateFragmentConstBuffer = null_PipelineState_CreateFragmentConstBuffer;
}
}
} //ns rhi