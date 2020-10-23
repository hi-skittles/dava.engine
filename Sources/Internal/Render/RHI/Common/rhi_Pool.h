#pragma once

#define RHI_RESOURCE_INCLUDE_BACKTRACE 0

#include "../rhi_Type.h"
#include "Utils/StringFormat.h"
#include "Concurrency/Spinlock.h"
#include "Concurrency/LockGuard.h"
#include "MemoryManager/MemoryProfiler.h"

#if (RHI_RESOURCE_INCLUDE_BACKTRACE)
#include "Debug/Backtrace.h"
#endif

#ifdef __DAVAENGINE_WIN_UAP__

#include "Logger/Logger.h"

#define RHI_POOL_ASSERT(expr, ...) \
{ \
    if (!(expr)) \
    { \
        DVASSERT(expr, ##__VA_ARGS__); \
        DAVA::Logger::Error(##__VA_ARGS__); \
    } \
}

#else

#define RHI_POOL_ASSERT(expr, ...) DVASSERT(expr, ##__VA_ARGS__)

#endif

namespace rhi
{
enum eHandleMasks : uint32
{
    HANDLE_INDEX_MASK = 0x0000FFFFU,
    HANDLE_INDEX_SHIFT = 0,

    HANDLE_GENERATION_MASK = 0x00FF0000U,
    HANDLE_GENERATION_SHIFT = 16,

    HANDLE_TYPE_MASK = 0xFF000000U,
    HANDLE_TYPE_SHIFT = 24,

    HANDLE_FORCEUINT32 = 0xFFFFFFFFU
};

#define RHI_HANDLE_INDEX(h) ((h & HANDLE_INDEX_MASK) >> HANDLE_INDEX_SHIFT)

template <class T, ResourceType RT, typename DT, bool need_restore = false>
class
ResourcePool
{
    struct Entry;

public:
    static Handle Alloc();
    static void Free(Handle h);

    static T* Get(Handle h);
    static bool IsAlive(Handle h);

    static void Reserve(unsigned maxCount);
    static unsigned ReleaseAll();
    static unsigned ReCreateAll();
    static void LogUnrestoredBacktraces();

    static uint32 PendingRestoreCount();

    static void Lock();
    static void Unlock();

    class
    Iterator
    {
    public:
        void operator++()
        {
            do
            {
                ++entry;
            } while (entry != end && !entry->allocated);
        }
        T* operator->()
        {
            return &(entry->object);
        }
        T& operator*()
        {
            return entry->object;
        }

        bool operator!=(const Iterator& i)
        {
            return i.entry != this->entry;
        }

        Entry* GetEntry() const
        {
            return entry;
        }

    private:
        friend class ResourcePool<T, RT, DT, need_restore>;

        Iterator(Entry* e, Entry* e_end)
            : entry(e)
            , end(e_end)
        {
            while (entry != end && !entry->allocated)
            {
                ++entry;
            }
        }

        Entry* entry;
        Entry* end;
    };

    static Iterator Begin();
    static Iterator End();

private:
    struct Entry
    {
        T object;

        uint32 allocated : 1;
        uint32 generation : 8;
        uint32 nextObjectIndex : 16;
        uint32 pad : 7;

#if (RHI_RESOURCE_INCLUDE_BACKTRACE)
        enum : uint32
        {
            MAX_BACKTRACE_SIZE = 32,
            FRAMES_TO_SKIP = 3
        };

        void* backtrace[MAX_BACKTRACE_SIZE];
        uint32 backtraceFrameCount = 0;

        void CaptureBacktrace()
        {
            memset(backtrace, sizeof(backtrace), 0);
            backtraceFrameCount = static_cast<uint32>(DAVA::Debug::GetBacktrace(backtrace, MAX_BACKTRACE_SIZE));
        }
#endif
    };

    static Entry* Object;
    static uint32 ObjectCount;
    static uint32 HeadIndex;
    static DAVA::Spinlock ObjectSync;
};

#define RHI_IMPL_POOL(T, RT, DT, nr) \
template <> rhi::ResourcePool<T, RT, DT, nr>::Entry* rhi::ResourcePool<T, RT, DT, nr>::Object = 0;    \
template <> uint32 rhi::ResourcePool<T, RT, DT, nr>::ObjectCount = 2048; \
template <> uint32 rhi::ResourcePool<T, RT, DT, nr>::HeadIndex = 0;    \
template <> DAVA::Spinlock rhi::ResourcePool<T, RT, DT, nr>::ObjectSync = {};   \

#define RHI_IMPL_POOL_SIZE(T, RT, DT, nr, sz) \
template <> rhi::ResourcePool<T, RT, DT, nr>::Entry* rhi::ResourcePool<T, RT, DT, nr>::Object = 0;    \
template <> uint32 rhi::ResourcePool<T, RT, DT, nr>::ObjectCount = sz;   \
template <> uint32 rhi::ResourcePool<T, RT, DT, nr>::HeadIndex = 0;    \
template <> DAVA::Spinlock rhi::ResourcePool<T, RT, DT, nr>::ObjectSync = {};

//------------------------------------------------------------------------------

template <class T, ResourceType RT, class DT, bool nr>
inline void
ResourcePool<T, RT, DT, nr>::Reserve(unsigned maxCount)
{
    DAVA::LockGuard<DAVA::Spinlock> lock(ObjectSync);
    DVASSERT(Object == nullptr);
    DVASSERT(maxCount < HANDLE_INDEX_MASK);
    ObjectCount = maxCount;
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, class DT, bool nr>
inline Handle ResourcePool<T, RT, DT, nr>::Alloc()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(ObjectSync);

    uint32 handle = InvalidHandle;
    if (!Object)
    {
        DAVA_MEMORY_PROFILER_ALLOC_SCOPE(DAVA::ALLOC_POOL_RHI_RESOURCE_POOL);
        Object = new Entry[ObjectCount];

        uint32 objectIndex = 0;
        while (objectIndex < ObjectCount)
        {
            Entry& e = Object[objectIndex];
            e.allocated = false;
            e.generation = 0;

            ++objectIndex;
            e.nextObjectIndex = objectIndex;
        }

        (Object + ObjectCount - 1)->nextObjectIndex = 0;
    }

    Entry* e = Object + HeadIndex;
    RHI_POOL_ASSERT(!e->allocated, DAVA::Format("[RHIPool] Failed to allocate handle: pool is empty | Pool<%d>", RT).c_str());
    HeadIndex = e->nextObjectIndex;

    e->allocated = true;
    ++e->generation;

#if (RHI_RESOURCE_INCLUDE_BACKTRACE)
    e->CaptureBacktrace();
#endif

    handle = 0;
    handle = ((uint32(e - Object) << HANDLE_INDEX_SHIFT) & HANDLE_INDEX_MASK) |
    (((e->generation) << HANDLE_GENERATION_SHIFT) & HANDLE_GENERATION_MASK) |
    ((RT << HANDLE_TYPE_SHIFT) & HANDLE_TYPE_MASK);

    return handle;
}

//------------------------------------------------------------------------------

#define HANDLE_DECOMPOSE(h) ((h & HANDLE_TYPE_MASK) >> HANDLE_TYPE_SHIFT), ((h & HANDLE_INDEX_MASK) >> HANDLE_INDEX_SHIFT), ((h & HANDLE_GENERATION_MASK) >> HANDLE_GENERATION_SHIFT)

template <class T, ResourceType RT, typename DT, bool nr>
inline void ResourcePool<T, RT, DT, nr>::Free(Handle h)
{
    uint32 index = (h & HANDLE_INDEX_MASK) >> HANDLE_INDEX_SHIFT;
    uint32 type = (h & HANDLE_TYPE_MASK) >> HANDLE_TYPE_SHIFT;
    RHI_POOL_ASSERT(type == RT, DAVA::Format("[RHIPool] Failed to free handle: mismatch resource type | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());
    RHI_POOL_ASSERT(index < ObjectCount, DAVA::Format("[RHIPool] Failed to free handle: index out of bounds | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());

    Entry* e = Object + index;
    RHI_POOL_ASSERT(e->allocated, DAVA::Format("[RHIPool] Failed to free handle: handle already freed | Pool<%d>", RT).c_str());

    ObjectSync.Lock();
    e->nextObjectIndex = HeadIndex;
    HeadIndex = index;
    e->allocated = false;
    ObjectSync.Unlock();
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline T* ResourcePool<T, RT, DT, nr>::Get(Handle h)
{
    RHI_POOL_ASSERT(h != InvalidHandle, DAVA::Format("[RHIPool] Failed to get resource by handle: handle is InvalidHandle | Pool<%d>", RT).c_str());
    RHI_POOL_ASSERT(((h & HANDLE_TYPE_MASK) >> HANDLE_TYPE_SHIFT) == RT, DAVA::Format("[RHIPool] Failed to get resource by handle: invalid resource type | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());
    uint32 index = (h & HANDLE_INDEX_MASK) >> HANDLE_INDEX_SHIFT;
    RHI_POOL_ASSERT(index < ObjectCount, DAVA::Format("[RHIPool] Failed to get resource by handle: index out of bounds | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());
    Entry* e = Object + index;
    RHI_POOL_ASSERT(e->allocated, DAVA::Format("[RHIPool] Failed to get resource by handle: not allocated | Pool<%d>, handle(type: %d, index: %d, generation: %d), last valid generation was %d", RT, HANDLE_DECOMPOSE(h), e->generation).c_str());
    RHI_POOL_ASSERT(e->generation == ((h & HANDLE_GENERATION_MASK) >> HANDLE_GENERATION_SHIFT), DAVA::Format("[RHIPool] Failed to get resource by handle: requested generation mismatch | Pool<%d>, handle(type: %d, index: %d, generation: %d), current valid generation is %d", RT, HANDLE_DECOMPOSE(h), e->generation).c_str());

    return &(e->object);
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline bool ResourcePool<T, RT, DT, nr>::IsAlive(Handle h)
{
    RHI_POOL_ASSERT(h != InvalidHandle, DAVA::Format("[RHIPool] Failed to check (is alive) resource by handle: handle is InvalidHandle | Pool<%d>", RT).c_str());
    RHI_POOL_ASSERT(((h & HANDLE_TYPE_MASK) >> HANDLE_TYPE_SHIFT) == RT, DAVA::Format("[RHIPool] Failed to check (is alive) resource by handle: invalid resource type | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());
    uint32 index = (h & HANDLE_INDEX_MASK) >> HANDLE_INDEX_SHIFT;
    RHI_POOL_ASSERT(index < ObjectCount, DAVA::Format("[RHIPool] Failed to check (is alive) resource by handle: index out of bounds | Pool<%d>, handle(type: %d, index: %d, generation: %d)", RT, HANDLE_DECOMPOSE(h)).c_str());

    Entry* e = Object + index;
    return e->allocated && (e->generation == ((h & HANDLE_GENERATION_MASK) >> HANDLE_GENERATION_SHIFT));
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline typename ResourcePool<T, RT, DT, nr>::Iterator
ResourcePool<T, RT, DT, nr>::Begin()
{
    return (Object) ? Iterator(Object, Object + ObjectCount) : Iterator(nullptr, nullptr);
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline typename ResourcePool<T, RT, DT, nr>::Iterator
ResourcePool<T, RT, DT, nr>::End()
{
    return (Object) ? Iterator(Object + ObjectCount, Object + ObjectCount) : Iterator(nullptr, nullptr);
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline unsigned
ResourcePool<T, RT, DT, nr>::ReleaseAll()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(ObjectSync);

    unsigned count = 0;
    for (Iterator i = Begin(), i_end = End(); i != i_end; ++i)
    {
        i->SetRecreatePending(true);
        i->Destroy(true);
        ++count;
    }
    return count;
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline unsigned
ResourcePool<T, RT, DT, nr>::ReCreateAll()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(ObjectSync);

    unsigned count = 0;
    for (Iterator i = Begin(), i_end = End(); i != i_end; ++i)
    {
        if (i->RecreatePending())
        {
            i->Create(i->CreationDesc(), true);
            i->SetRecreatePending(false);
            i->MarkNeedRestore();
        }
        ++count;
    }
    return count;
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline void
ResourcePool<T, RT, DT, nr>::LogUnrestoredBacktraces()
{
#if (RHI_RESOURCE_INCLUDE_BACKTRACE)
    uint32 total = 0;
    uint32 unrestored = 0;
    uint32 counter = ResourceImpl<T, DT>::PendingRestoreCount();

    Lock();
    for (Iterator i = Begin(), e = End(); i != e; ++i)
    {
        if (i->NeedRestore())
        {
            DAVA::Logger::Error("----------------------------");
            Entry* entry = i.GetEntry();
            for (uint32 frame = Entry::FRAMES_TO_SKIP; frame < entry->backtraceFrameCount; ++frame)
            {
                DAVA::String symbol = DAVA::Debug::GetFrameSymbol(entry->backtrace[frame]);
                DAVA::Logger::Error(symbol.c_str());
            }
            ++unrestored;
        }
        ++total;
    }
    Unlock();

    DAVA::Logger::Error("----------------------------------------------");
    DAVA::Logger::Error("Unrestored resources: %u of %u, counter = %u", unrestored, total, counter);
    DAVA::Logger::Error("----------------------------------------------");
#endif
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline void
ResourcePool<T, RT, DT, nr>::Lock()
{
    ObjectSync.Lock();
}

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline void
ResourcePool<T, RT, DT, nr>::Unlock()
{
    ObjectSync.Unlock();
}

//------------------------------------------------------------------------------

template <class T, class DT>
class
ResourceImpl
{
public:
    const DT& CreationDesc() const
    {
        return creationDesc;
    }

    void UpdateCreationDesc(const DT& desc)
    {
        creationDesc = desc;
        Memset(&creationDesc.initialData, 0, sizeof(creationDesc.initialData));
    }

    bool NeedRestore() const
    {
        return needRestore;
    }

    void MarkNeedRestore()
    {
        if (needRestore || (creationDesc.needRestore == false))
            return;

        needRestore = true;
        ++ObjectsToRestore;
    }

    void MarkRestored()
    {
        if (needRestore)
        {
            needRestore = false;
            DVASSERT(PendingRestoreCount() > 0);
            --ObjectsToRestore;
        }
    }

    bool RecreatePending() const
    {
        return recreatePending;
    }

    void SetRecreatePending(bool value)
    {
        recreatePending = value;
    }

    static uint32 PendingRestoreCount()
    {
        return ObjectsToRestore.Get();
    }

private:
    DT creationDesc;
    bool needRestore = false;
    bool recreatePending = false;
    static DAVA::Atomic<uint32> ObjectsToRestore;
};

//------------------------------------------------------------------------------

template <class T, ResourceType RT, typename DT, bool nr>
inline uint32
ResourcePool<T, RT, DT, nr>::PendingRestoreCount()
{
    return ResourceImpl<T, DT>::PendingRestoreCount();
}

#define RHI_IMPL_RESOURCE(T, DT) template <> DAVA::Atomic<uint32> rhi::ResourceImpl<T, DT>::ObjectsToRestore(0);
}
