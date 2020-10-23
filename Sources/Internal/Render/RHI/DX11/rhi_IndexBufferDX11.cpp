#include "rhi_BufferDX11.h"
#include "../Common/rhi_Pool.h"

namespace rhi
{
using IndexBufferDX11Pool = ResourcePool<BufferDX11_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true>;
RHI_IMPL_POOL(BufferDX11_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true);

static Handle dx11_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
{
    Handle handle = IndexBufferDX11Pool::Alloc();
    BufferDX11_t* buffer = IndexBufferDX11Pool::Get(handle);

    DXGI_FORMAT format = (desc.indexSize == INDEX_SIZE_32BIT) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    if (!buffer->Create(desc.size, desc.usage, format, D3D11_BIND_INDEX_BUFFER, desc.initialData))
    {
        IndexBufferDX11Pool::Free(handle);
        handle = InvalidHandle;
    }
    return handle;
}

static void dx11_IndexBuffer_Delete(Handle handle, bool)
{
    if (handle != InvalidHandle)
    {
        BufferDX11_t* buffer = IndexBufferDX11Pool::Get(handle);
        buffer->Destroy();
        IndexBufferDX11Pool::Free(handle);
    }
}

static bool dx11_IndexBuffer_Update(Handle handle, const void* data, uint32 offset, uint32 size)
{
    BufferDX11_t* buffer = IndexBufferDX11Pool::Get(handle);
    return buffer->Update(data, offset, size);
}

static void* dx11_IndexBuffer_Map(Handle handle, uint32 offset, uint32 size)
{
    BufferDX11_t* buffer = IndexBufferDX11Pool::Get(handle);
    return buffer->Map(offset, size);
}

static void dx11_IndexBuffer_Unmap(Handle handle)
{
    BufferDX11_t* buffer = IndexBufferDX11Pool::Get(handle);
    return buffer->Unmap();
}

void IndexBufferDX11::Init(uint32 maxCount)
{
    IndexBufferDX11Pool::Reserve(maxCount);
}

void IndexBufferDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = &dx11_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = &dx11_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = &dx11_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = &dx11_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = &dx11_IndexBuffer_Unmap;
}

void IndexBufferDX11::SetToRHI(Handle ibh, uint32 offset, ID3D11DeviceContext* context)
{
    BufferDX11_t* self = IndexBufferDX11Pool::Get(ibh);
    self->ResolvePendingUpdate(context);
    context->IASetIndexBuffer(self->buffer, self->format, offset);
}
}
