#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct PerfQueryNull_t : public ResourceImpl<PerfQueryNull_t, PerfQuery::Descriptor>
{
};
RHI_IMPL_RESOURCE(PerfQueryNull_t, PerfQuery::Descriptor)

using PerfQueryNullPool = ResourcePool<PerfQueryNull_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor>;
RHI_IMPL_POOL(PerfQueryNull_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_PerfQuery_Create()
{
    return PerfQueryNullPool::Alloc();
}

void null_PerfQuery_Delete(Handle h)
{
    PerfQueryNullPool::Free(h);
}

void null_PerfQuery_Reset(Handle)
{
}

bool null_PerfQuery_IsReady(Handle)
{
    return true;
}

uint64 null_PerfQuery_Value(Handle)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////

namespace PerfQueryNull
{
void Init(uint32 maxCount)
{
    PerfQueryNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PerfQuery_Create = null_PerfQuery_Create;
    dispatch->impl_PerfQuery_Delete = null_PerfQuery_Delete;
    dispatch->impl_PerfQuery_Reset = null_PerfQuery_Reset;
    dispatch->impl_PerfQuery_IsReady = null_PerfQuery_IsReady;
    dispatch->impl_PerfQuery_Value = null_PerfQuery_Value;
}
}
} //ns rhi