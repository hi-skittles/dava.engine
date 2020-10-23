#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct VertexBufferNull_t : public ResourceImpl<VertexBufferNull_t, VertexBuffer::Descriptor>
{
    void* mappedData = nullptr;
};
RHI_IMPL_RESOURCE(VertexBufferNull_t, VertexBuffer::Descriptor)

using VertexBufferNullPool = ResourcePool<VertexBufferNull_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor>;
RHI_IMPL_POOL(VertexBufferNull_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_VertexBuffer_Create(const VertexBuffer::Descriptor& desc)
{
    Handle h = VertexBufferNullPool::Alloc();
    VertexBufferNull_t* self = VertexBufferNullPool::Get(h);
    self->UpdateCreationDesc(desc);
    return h;
}

void null_VertexBuffer_Delete(Handle h, bool)
{
    VertexBufferNull_t* self = VertexBufferNullPool::Get(h);
    DVASSERT(self->mappedData == nullptr);

    VertexBufferNullPool::Free(h);
}

bool null_VertexBuffer_Update(Handle, const void*, uint32, uint32)
{
    return true;
}

void* null_VertexBuffer_Map(Handle h, uint32 offset, uint32 size)
{
    VertexBufferNull_t* self = VertexBufferNullPool::Get(h);
    const VertexBuffer::Descriptor& desc = self->CreationDesc();

    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);
    DVASSERT(offset + size <= desc.size);
    DVASSERT(self->mappedData == nullptr);

    self->mappedData = ::malloc(desc.size);

    return static_cast<uint8*>(self->mappedData) + offset;
}

void null_VertexBuffer_Unmap(Handle h)
{
    VertexBufferNull_t* self = VertexBufferNullPool::Get(h);
    DVASSERT(self->mappedData != nullptr);
    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);

    ::free(self->mappedData);
    self->mappedData = nullptr;
}

bool null_VertexBuffer_NeedRestore(Handle)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////

namespace VertexBufferNull
{
void Init(uint32 maxCount)
{
    VertexBufferNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_VertexBuffer_Create = null_VertexBuffer_Create;
    dispatch->impl_VertexBuffer_Delete = null_VertexBuffer_Delete;
    dispatch->impl_VertexBuffer_Update = null_VertexBuffer_Update;
    dispatch->impl_VertexBuffer_Map = null_VertexBuffer_Map;
    dispatch->impl_VertexBuffer_Unmap = null_VertexBuffer_Unmap;
    dispatch->impl_VertexBuffer_NeedRestore = null_VertexBuffer_NeedRestore;
}
}
} //ns rhi