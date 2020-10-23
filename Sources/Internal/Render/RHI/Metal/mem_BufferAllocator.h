#pragma once

#if !(TARGET_IPHONE_SIMULATOR == 1)

#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "../Common/rhi_RemoteHeap.h"
#include "_metal.h"

/*
struct BufferTraits
{
    BufUID; // expected to be typedef'ed to proper render-API type (id<MTLBuffer>, IDirect3DVertexBuffer*, GLuint  etc.)
    static BufUID Create( uint32 size );
    static void Delete( BufUID uid );
    static void Update( BufUID uid, uint32 offset, const void* data, uint32 data_size );
};
// */

namespace rhi
{
struct MetalBufferTraits
{
    typedef id<MTLBuffer> BufUID;

    static BufUID Create(uint32 size)
    {
        id<MTLBuffer> buf = [_Metal_Device newBufferWithLength:size options:MTLResourceOptionCPUCacheModeDefault];

        [buf retain];
        return buf;
    }

    static void Delete(BufUID uid)
    {
        [uid release];
        [uid release];
        uid = nil;
    }

    static void Update(BufUID uid, uint32 offset, const void* data, uint32 data_size)
    {
        uint8* buf = (uint8*)([uid contents]);
        memcpy(buf + offset, data, data_size);
    }
};

template <class T>
class BufferAllocator
{
public:
    struct Block
    {
        typename T::BufUID uid;

        uint32 base = 0; // offset from start in buffer
        uint32 size = 0;

        void Update(const void* data)
        {
            T::Update(uid, base, data, size);
        }
    };

    BufferAllocator(const char* name, uint32 page_size = 2 * 1024 * 1024, uint32 granularity = 1024);
    ~BufferAllocator() = default;

    bool alloc(uint32 size, Block* block);
    bool free(const Block& block);

    void dump_stats() const;

    // disable copying and moving
    BufferAllocator(BufferAllocator&&) = delete;
    BufferAllocator(const BufferAllocator&) = delete;
    BufferAllocator& operator=(const BufferAllocator&) = delete;

private:
    struct page_t
    {
        uint32 size;
        typename T::BufUID bufferUid;
        SimpleRemoteHeap* heap;
    };

    DAVA::Mutex pageLock;
    std::vector<page_t> _page;
    std::string _name;
    const uint32 _page_sz;
    const uint32 _granularity;
};

static uint8* MemBase = reinterpret_cast<uint8*>(0x000000f0llu);

typedef BufferAllocator<MetalBufferTraits> MetalBufferAllocator;

template <class T>
inline BufferAllocator<T>::BufferAllocator(const char* name, uint32 page_size, uint32 granularity)
    : _name(name)
    , _page_sz(page_size)
    , _granularity(granularity)
{
    _page.reserve(8);
}

template <class T>
inline bool BufferAllocator<T>::alloc(uint32 size, BufferAllocator<T>::Block* block)
{
    bool success = false;

    void* mem = nullptr;
    uint32 alignedSize = L_ALIGNED_SIZE(size, _granularity);

    DAVA::LockGuard<DAVA::Mutex> lock(pageLock);

    for (page_t& page : _page)
    {
        mem = page.heap->alloc(alignedSize);

        if (mem)
        {
            block->uid = page.bufferUid;
            block->base = static_cast<uint32>(reinterpret_cast<uint8*>(mem) - MemBase);
            block->size = size;
            success = true;
            break;
        }
    }

    if (mem == nullptr)
    {
        uint32 requiredSize = _page_sz;
        while (requiredSize < alignedSize)
            requiredSize *= 2;

        page_t page;
        page.size = requiredSize;
        page.heap = new SimpleRemoteHeap(8 * 1024);
        page.heap->initialize(MemBase, page.size);
        page.bufferUid = T::Create(requiredSize);
        _page.push_back(page);

        mem = page.heap->alloc(alignedSize);

        if (mem)
        {
            block->uid = page.bufferUid;
            block->base = static_cast<uint32>(reinterpret_cast<uint8*>(mem) - MemBase);
            block->size = size;
            success = true;
        }
    }

    return success;
}

template <class T>
inline bool BufferAllocator<T>::free(const BufferAllocator<T>::Block& block)
{
    bool success = false;

    DAVA::LockGuard<DAVA::Mutex> lock(pageLock);

    for (auto p = _page.begin(), p_end = _page.end(); p != p_end; ++p)
    {
        if (block.uid == p->bufferUid)
        {
            p->heap->free(MemBase + block.base);

            if (p->heap->Empty())
            {
                delete p->heap;
                T::Delete(p->bufferUid);
                _page.erase(p);
            }

            success = true;
            break;
        }
    }

    return success;
}

template <class T>
inline void BufferAllocator<T>::dump_stats() const
{
    DAVA::LockGuard<DAVA::Mutex> lock(pageLock);

    DAVA::Logger::Info("\"%s\" pages (%u)", _name.c_str(), _page.size());
    for (uint32 p = 0; p != _page.size(); ++p)
    {
        SimpleRemoteHeap::Stat s;
        _page[p].heap->get_stats(&s);
        DAVA::Logger::Info("  [%u] heap usage : %.1f%% (%u blocks)  comitted %.1f%%", p, 100.0f * (float(s.alloced_size) / float(s.reserved)), s.alloced_block_count, 100.0f * (float(s.comitted_size) / float(s.reserved)));
    }
}
}

#endif // !(TARGET_IPHONE_SIMULATOR == 1)
