#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct IndexBufferNull_t : public ResourceImpl<IndexBufferNull_t, IndexBuffer::Descriptor>
{
    void* mappedData = nullptr;
};
RHI_IMPL_RESOURCE(IndexBufferNull_t, IndexBuffer::Descriptor)

using IndexBufferNullPool = ResourcePool<IndexBufferNull_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor>;
RHI_IMPL_POOL(IndexBufferNull_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
{
    Handle h = IndexBufferNullPool::Alloc();
    IndexBufferNull_t* self = IndexBufferNullPool::Get(h);
    self->UpdateCreationDesc(desc);

    return h;
}

void null_IndexBuffer_Delete(Handle h, bool)
{
    IndexBufferNull_t* self = IndexBufferNullPool::Get(h);
    DVASSERT(self->mappedData == nullptr);

    return IndexBufferNullPool::Free(h);
}

bool null_IndexBuffer_Update(Handle, const void*, uint32, uint32)
{
    return true;
}

void* null_IndexBuffer_Map(Handle h, uint32 offset, uint32 size)
{
    IndexBufferNull_t* self = IndexBufferNullPool::Get(h);
    const IndexBuffer::Descriptor& desc = self->CreationDesc();

    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);
    DVASSERT(offset + size <= desc.size);
    DVASSERT(self->mappedData == nullptr);

    self->mappedData = ::malloc(desc.size);

    return static_cast<uint8*>(self->mappedData) + offset;
}

void null_IndexBuffer_Unmap(Handle h)
{
    IndexBufferNull_t* self = IndexBufferNullPool::Get(h);
    DVASSERT(self->mappedData != nullptr);
    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);

    ::free(self->mappedData);
    self->mappedData = nullptr;
}

bool null_IndexBuffer_NeedRestore(Handle)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////

namespace IndexBufferNull
{
void Init(uint32 maxCount)
{
    IndexBufferNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = null_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = null_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = null_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = null_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = null_IndexBuffer_Unmap;
    dispatch->impl_IndexBuffer_NeedRestore = null_IndexBuffer_NeedRestore;
}
}
} //ns rhi