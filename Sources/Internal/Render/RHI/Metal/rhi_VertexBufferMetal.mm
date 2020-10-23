#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "rhi_Metal.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "mem_BufferAllocator.h"
#include "_metal.h"

#if !(TARGET_IPHONE_SIMULATOR == 1)

namespace rhi
{
struct VertexBufferMetal_t
{
    uint32 size = 0;
    void* data = nullptr;
    id<MTLBuffer> uid = nil;
    MetalBufferAllocator::Block block;
};

typedef ResourcePool<VertexBufferMetal_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor, false> VertexBufferMetalPool;
RHI_IMPL_POOL(VertexBufferMetal_t, RESOURCE_VERTEX_BUFFER, VertexBuffer::Descriptor, false);

static const uint32 VBPoolSize = 4 * 1024 * 1024;
static const uint32 VBPoolGranularity = 1024;
static MetalBufferAllocator _VB_Pool("VB", VBPoolSize, VBPoolGranularity);

static Handle metal_VertexBuffer_Create(const VertexBuffer::Descriptor& desc)
{
    Handle handle = InvalidHandle;

    if (desc.usage == USAGE_DYNAMICDRAW)
    {
        id<MTLBuffer> uid = (desc.initialData) ?
        [_Metal_Device newBufferWithBytes:desc.initialData
                                   length:desc.size
                                  options:MTLResourceOptionCPUCacheModeDefault] :
        [_Metal_Device newBufferWithLength:desc.size
                                   options:MTLResourceOptionCPUCacheModeDefault];

        if (uid)
        {
            handle = VertexBufferMetalPool::Alloc();
            VertexBufferMetal_t* vb = VertexBufferMetalPool::Get(handle);
            vb->data = [uid contents];
            vb->size = desc.size;
            vb->uid = uid;
            [vb->uid retain];
        }
    }
    else
    {
        MetalBufferAllocator::Block block;

        if (_VB_Pool.alloc(desc.size, &block))
        {
            handle = VertexBufferMetalPool::Alloc();
            VertexBufferMetal_t* vb = VertexBufferMetalPool::Get(handle);

            vb->block = block;
            vb->size = desc.size;
            vb->uid = nil;

            if (desc.initialData)
                block.Update(desc.initialData);
        }
    }

    return handle;
}

static void metal_VertexBuffer_Delete(Handle vb, bool)
{
    VertexBufferMetal_t* self = VertexBufferMetalPool::Get(vb);

    if (self)
    {
        if (self->uid)
        {
            [self->uid release];
            [self->uid release];
            self->uid = nil;
            self->data = nullptr;
        }
        else
        {
            _VB_Pool.free(self->block);
            self->data = nullptr;
            self->block.uid = nil;
            self->block.base = 0;
        }

        VertexBufferMetalPool::Free(vb);
    }
}

static bool metal_VertexBuffer_Update(Handle vb, const void* data, uint32 offset, uint32 size)
{
    bool success = false;
    VertexBufferMetal_t* self = VertexBufferMetalPool::Get(vb);

    if (offset + size <= self->size)
    {
        if (self->uid)
            memcpy(((uint8*)self->data) + offset, data, size);
        else
            self->block.Update(data);
        success = true;
    }

    return success;
}

static void* metal_VertexBuffer_Map(Handle vb, uint32 offset, uint32 size)
{
    VertexBufferMetal_t* self = VertexBufferMetalPool::Get(vb);
    DVASSERT(self->data);
    return (offset + size <= self->size) ? ((uint8*)self->data) + offset : 0;
}

static void metal_VertexBuffer_Unmap(Handle vb)
{
    VertexBufferMetal_t* self = VertexBufferMetalPool::Get(vb);
    DVASSERT(self->data);
}

static bool metal_VertexBuffer_NeedRestore(Handle vb)
{
    return false;
}

namespace VertexBufferMetal
{
void Init(uint32 maxCount)
{
    VertexBufferMetalPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_VertexBuffer_Create = &metal_VertexBuffer_Create;
    dispatch->impl_VertexBuffer_Delete = &metal_VertexBuffer_Delete;
    dispatch->impl_VertexBuffer_Update = &metal_VertexBuffer_Update;
    dispatch->impl_VertexBuffer_Map = &metal_VertexBuffer_Map;
    dispatch->impl_VertexBuffer_Unmap = &metal_VertexBuffer_Unmap;
    dispatch->impl_VertexBuffer_NeedRestore = &metal_VertexBuffer_NeedRestore;
}

id<MTLBuffer> GetBuffer(Handle ib, unsigned* base)
{
    VertexBufferMetal_t* self = VertexBufferMetalPool::Get(ib);
    id<MTLBuffer> uid;

    if (self->uid)
    {
        uid = self->uid;
        *base = 0;
    }
    else
    {
        uid = self->block.uid;
        *base = self->block.base;
    }

    return uid;
}
}

} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
