#include "rhi_BufferDX11.h"
#include "../Common/rhi_Pool.h"

namespace rhi
{
using VertexBufferDX11Pool = ResourcePool<BufferDX11_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor, true>;
RHI_IMPL_POOL(BufferDX11_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor, true);

static Handle dx11_VertexBuffer_Create(const VertexBuffer::Descriptor& desc)
{
    Handle handle = VertexBufferDX11Pool::Alloc();
    BufferDX11_t* buffer = VertexBufferDX11Pool::Get(handle);
    if (!buffer->Create(desc.size, desc.usage, DXGI_FORMAT_UNKNOWN, D3D11_BIND_VERTEX_BUFFER, desc.initialData))
    {
        VertexBufferDX11Pool::Free(handle);
        handle = InvalidHandle;
    }
    return handle;
}

static void dx11_VertexBuffer_Delete(Handle handle, bool)
{
    if (handle != InvalidHandle)
    {
        BufferDX11_t* buffer = VertexBufferDX11Pool::Get(handle);
        buffer->Destroy();
        VertexBufferDX11Pool::Free(handle);
    }
}

static bool dx11_VertexBuffer_Update(Handle handle, const void* data, uint32 offset, uint32 size)
{
    BufferDX11_t* buffer = VertexBufferDX11Pool::Get(handle);
    return buffer->Update(data, offset, size);
}

static void* dx11_VertexBuffer_Map(Handle handle, uint32 offset, uint32 size)
{
    BufferDX11_t* buffer = VertexBufferDX11Pool::Get(handle);
    return buffer->Map(offset, size);
}

static void dx11_VertexBuffer_Unmap(Handle handle)
{
    BufferDX11_t* buffer = VertexBufferDX11Pool::Get(handle);
    return buffer->Unmap();
}

void VertexBufferDX11::Init(uint32 maxCount)
{
    VertexBufferDX11Pool::Reserve(maxCount);
}

void VertexBufferDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_VertexBuffer_Create = &dx11_VertexBuffer_Create;
    dispatch->impl_VertexBuffer_Delete = &dx11_VertexBuffer_Delete;
    dispatch->impl_VertexBuffer_Update = &dx11_VertexBuffer_Update;
    dispatch->impl_VertexBuffer_Map = &dx11_VertexBuffer_Map;
    dispatch->impl_VertexBuffer_Unmap = &dx11_VertexBuffer_Unmap;
}

void VertexBufferDX11::SetToRHI(Handle vbh, uint32 stream_i, uint32 offset, uint32 stride, ID3D11DeviceContext* context)
{
    BufferDX11_t* self = VertexBufferDX11Pool::Get(vbh);
    self->ResolvePendingUpdate(context);

    ID3D11Buffer* vb[1] = { self->buffer };
    UINT vb_offset[1] = { offset };
    UINT vb_stride[1] = { stride };
    context->IASetVertexBuffers(stream_i, 1, vb, vb_stride, vb_offset);
}
}
