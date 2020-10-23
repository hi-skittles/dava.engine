#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "rhi_Metal.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "_metal.h"
#include "mem_BufferAllocator.h"

#if !(TARGET_IPHONE_SIMULATOR == 1)

namespace rhi
{
struct IndexBufferMetal_t
{
    unsigned size = 0;
    void* data = nil;
    id<MTLBuffer> uid = nil;
    MetalBufferAllocator::Block block;
    MTLIndexType type;
};

typedef ResourcePool<IndexBufferMetal_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, false> IndexBufferMetalPool;
RHI_IMPL_POOL(IndexBufferMetal_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, false);

static const uint32 IBPoolSize = 2 * 1024 * 1024;
static const uint32 IBPoolGranularity = 256;
static MetalBufferAllocator _IB_Pool("IB", IBPoolSize, IBPoolGranularity);

static Handle metal_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
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
            handle = IndexBufferMetalPool::Alloc();
            IndexBufferMetal_t* ib = IndexBufferMetalPool::Get(handle);
            ib->data = [uid contents];
            ib->size = desc.size;
            ib->uid = uid;
            ib->type = (desc.indexSize == INDEX_SIZE_32BIT) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
            [ib->uid retain];
        }
    }
    else
    {
        MetalBufferAllocator::Block block;

        if (_IB_Pool.alloc(desc.size, &block))
        {
            handle = IndexBufferMetalPool::Alloc();
            IndexBufferMetal_t* ib = IndexBufferMetalPool::Get(handle);

            ib->block = block;
            ib->size = desc.size;
            ib->type = (desc.indexSize == INDEX_SIZE_32BIT) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
            ib->uid = nil;

            if (desc.initialData)
                block.Update(desc.initialData);
        }
    }

    return handle;
}

static void metal_IndexBuffer_Delete(Handle ib, bool)
{
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);

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
            _IB_Pool.free(self->block);
            self->data = nullptr;
            self->block.uid = nil;
            self->block.base = 0;
        }

        IndexBufferMetalPool::Free(ib);
    }
}

static bool metal_IndexBuffer_Update(Handle ib, const void* data, unsigned offset, unsigned size)
{
    bool success = false;
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);

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

static void* metal_IndexBuffer_Map(Handle ib, unsigned offset, unsigned size)
{
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);
    DVASSERT(self->data);
    return (offset + size <= self->size) ? ((uint8*)self->data) + offset : 0;
}

static void metal_IndexBuffer_Unmap(Handle ib)
{
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);
    DVASSERT(self->data != nullptr);
}

static bool metal_IndexBuffer_NeedRestore(Handle ib)
{
    return false;
}

namespace IndexBufferMetal
{
void Init(uint32 maxCount)
{
    IndexBufferMetalPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = &metal_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = &metal_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = &metal_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = &metal_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = &metal_IndexBuffer_Unmap;
    dispatch->impl_IndexBuffer_NeedRestore = &metal_IndexBuffer_NeedRestore;
}

id<MTLBuffer> GetBuffer(Handle ib, unsigned* base)
{
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);
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

MTLIndexType GetType(Handle ib)
{
    IndexBufferMetal_t* self = IndexBufferMetalPool::Get(ib);
    return self->type;
}

} // namespace IndexBufferGLES

} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
