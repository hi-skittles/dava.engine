#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct SamplerStateNull_t : public ResourceImpl<SamplerStateNull_t, SamplerState::Descriptor>
{
};
RHI_IMPL_RESOURCE(SamplerStateNull_t, SamplerState::Descriptor)

using SamplerStateNullPool = ResourcePool<SamplerStateNull_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor>;
RHI_IMPL_POOL(SamplerStateNull_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_SamplerState_Create(const SamplerState::Descriptor&)
{
    return SamplerStateNullPool::Alloc();
}

void null_SamplerState_Delete(Handle h)
{
    SamplerStateNullPool::Free(h);
}

//////////////////////////////////////////////////////////////////////////

namespace SamplerStateNull
{
void Init(uint32 maxCount)
{
    SamplerStateNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SamplerState_Create = null_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = null_SamplerState_Delete;
}
}
} //ns rhi