#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include "Base/GlobalEnum.h"
#include "MemoryManager/AllocPools.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include <cassert>

#if defined(__DAVAENGINE_WIN32__)
#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <dbghelp.h>
#pragma warning(pop)
#elif defined(__DAVAENGINE_WIN_UAP__)
#elif defined(__DAVAENGINE_APPLE__)
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <mach/mach.h>
#if defined(__DAVAENGINE_MACOS__)
#include <mach/mach_vm.h>
#endif
#elif defined(__DAVAENGINE_ANDROID__)
#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#elif defined(__DAVAENGINE_LINUX__)
#include <malloc.h>
#else
#error "Unknown platform"
#endif

#include "Base/Hash.h"
#include "Debug/DVAssert.h"
#include "Debug/Backtrace.h"
#include "Concurrency/Thread.h"
#include "Math/MathHelpers.h"

#include "FileSystem/File.h"

#include "MemoryManager/MallocHook.h"
#include "MemoryManager/MemoryManager.h"

namespace DAVA
{
struct MemoryManager::MemoryBlock
{
    MemoryBlock* prev; // Pointer to previous block
    MemoryBlock* next; // Pointer to next block
    void* realBlockStart; // Pointer to real block start
    void* padding1; // Padding to make sure that
    uint32 padding2; //          struct size is integral multiple of 16 bytes
    uint32 orderNo; // Block order number
    uint32 allocByApp; // Size requested by application
    uint32 allocTotal; // Total allocated size
    uint32 bktraceHash; // Unique hash number to identify block backtrace
    uint32 pool; // Allocation pool block belongs to
    uint32 tags; // Tags block belongs to
    uint32 mark; // Mark to distinguish tracked memory blocks
};
static_assert(sizeof(MemoryManager::MemoryBlock) % 16 == 0, "sizeof(MemoryManager::MemoryBlock) % 16 != 0");

struct MemoryManager::InternalMemoryBlock
{
    uint32 allocByApp; // Size requested by application
    uint32 allocTotal; // Total allocated size
    uint32 padding;
    uint32 mark; // Mark to distinguish internal memory blocks
};
static_assert(sizeof(MemoryManager::InternalMemoryBlock) % 16 == 0, "sizeof(MemoryManager::InternalMemoryBlock) % 16 != 0");

struct MemoryManager::Backtrace
{
    size_t nref;
    uint32 hash;
    bool symbolsCollected;
    Array<void*, BACKTRACE_DEPTH> frames;
};

struct MemoryManager::AllocScopeItem
{
    AllocScopeItem* next;
    uint32 allocPool;
};

//////////////////////////////////////////////////////////////////////////

MMItemName MemoryManager::tagNames[MAX_TAG_COUNT];
MMItemName MemoryManager::allocPoolNames[MAX_ALLOC_POOL_COUNT];

uint32 MemoryManager::registeredTagCount = 0;
uint32 MemoryManager::registeredAllocPoolCount = 0;

void MemoryManager::RegisterAllocPoolName(uint32 index, const char8* name)
{
    DVASSERT(name != nullptr && 0 < strlen(name) && strlen(name) < MMItemName::MAX_NAME_LENGTH);
    DVASSERT(index < MAX_ALLOC_POOL_COUNT);
    DVASSERT(0 == index || allocPoolNames[index - 1].name[0] != '\0'); // Names should be registered sequentially with no gap

    strncpy(allocPoolNames[index].name, name, MMItemName::MAX_NAME_LENGTH);
    allocPoolNames[index].name[MMItemName::MAX_NAME_LENGTH - 1] = '\0';
    registeredAllocPoolCount += 1;
}

void MemoryManager::RegisterTagName(uint32 tagMask, const char8* name)
{
    DVASSERT(name != nullptr && 0 < strlen(name) && strlen(name) < MMItemName::MAX_NAME_LENGTH);
    DVASSERT(tagMask != 0 && IsPowerOf2(tagMask));

    const size_t index = HighestBitIndex(tagMask);
    DVASSERT(index < MAX_TAG_COUNT);

    DVASSERT(0 == index || UNTAGGED == index || tagNames[index - 1].name[0] != '\0'); // Names should be registered sequentially with no gap

    strncpy(tagNames[index].name, name, MMItemName::MAX_NAME_LENGTH);
    tagNames[index].name[MMItemName::MAX_NAME_LENGTH - 1] = '\0';

    if (UNTAGGED != index)
        registeredTagCount += 1;
}

//////////////////////////////////////////////////////////////////////////
MemoryManager::MemoryManager()
{
    RegisterAllocPoolName(ALLOC_POOL_TOTAL, "total");
    RegisterAllocPoolName(ALLOC_POOL_DEFAULT, "default");
    RegisterAllocPoolName(ALLOC_GPU_TEXTURE, "gpu texture");
    RegisterAllocPoolName(ALLOC_GPU_RDO_VERTEX, "gpu rdo vertex");
    RegisterAllocPoolName(ALLOC_GPU_RDO_INDEX, "gpu rdo index");
    RegisterAllocPoolName(ALLOC_POOL_SYSTEM, "system");
    RegisterAllocPoolName(ALLOC_POOL_FMOD, "fmod");
    RegisterAllocPoolName(ALLOC_POOL_BULLET, "bullet");
    RegisterAllocPoolName(ALLOC_POOL_BASEOBJECT, "base object");
    RegisterAllocPoolName(ALLOC_POOL_POLYGONGROUP, "polygon group");
    RegisterAllocPoolName(ALLOC_POOL_COMPONENT, "component");
    RegisterAllocPoolName(ALLOC_POOL_ENTITY, "entity");
    RegisterAllocPoolName(ALLOC_POOL_LANDSCAPE, "landscape");
    RegisterAllocPoolName(ALLOC_POOL_IMAGE, "image");
    RegisterAllocPoolName(ALLOC_POOL_TEXTURE, "texture");
    RegisterAllocPoolName(ALLOC_POOL_NMATERIAL, "nmaterial");

    RegisterAllocPoolName(ALLOC_POOL_RHI_BUFFER, "rhi buffer");
    RegisterAllocPoolName(ALLOC_POOL_RHI_VERTEX_MAP, "rhi vertex map");
    RegisterAllocPoolName(ALLOC_POOL_RHI_INDEX_MAP, "rhi index map");
    RegisterAllocPoolName(ALLOC_POOL_RHI_TEXTURE_MAP, "rhi texture map");
    RegisterAllocPoolName(ALLOC_POOL_RHI_RESOURCE_POOL, "rhi res pool");

    RegisterAllocPoolName(ALLOC_POOL_LUA, "lua engine");
    RegisterAllocPoolName(ALLOC_POOL_SQLITE, "sqlite");
    RegisterAllocPoolName(ALLOC_POOL_PHYSICS, "physics");
}

MemoryManager* MemoryManager::Instance()
{
    static MemoryManager mm;
    return &mm;
}

void MemoryManager::EnableLightWeightMode()
{
    lightWeightMode = true;
}

void MemoryManager::SetCallbacks(Function<void()> updateCallback_, Function<void(uint32, bool)> tagCallback_)
{
    updateCallback = updateCallback_;
    tagCallback = tagCallback_;
}

void MemoryManager::Update()
{
    if (nullptr == symbolCollectorThread)
    {
        symbolCollectorThread = Thread::Create(MakeFunction(this, &MemoryManager::SymbolCollectorThread));
        symbolCollectorThread->SetName("SymbolCollectorThread");
        symbolCollectorThread->Start();
    }

    if (updateCallback != nullptr)
    {
        updateCallback();
    }
}

void MemoryManager::Finish()
{
    if (symbolCollectorThread != nullptr)
    {
        symbolCollectorThread->Cancel();
        symbolCollectorCondVar.NotifyAll();
        symbolCollectorThread->Join();
        symbolCollectorThread->Release();
        symbolCollectorThread = nullptr;
    }
}

DAVA_NOINLINE void* MemoryManager::Allocate(size_t size, uint32 poolIndex)
{
    assert(ALLOC_POOL_TOTAL < poolIndex && poolIndex < MAX_ALLOC_POOL_COUNT);

    // On zero-sized allocation request allocate 1 byte to return unique memory block
    size_t totalSize = sizeof(MemoryBlock) + (size != 0 ? size : 1);
    if (totalSize & (BLOCK_ALIGN - 1))
    {
        totalSize += (BLOCK_ALIGN - (totalSize & (BLOCK_ALIGN - 1)));
    }

    MemoryBlock* block = static_cast<MemoryBlock*>(MallocHook::Malloc(totalSize));
    if (block != nullptr)
    {
        block->mark = BLOCK_MARK;
        block->pool = poolIndex;
        block->realBlockStart = static_cast<void*>(block);
        block->allocByApp = static_cast<uint32>(size);
        block->allocTotal = static_cast<uint32>(MallocHook::MallocSize(block->realBlockStart));
        block->bktraceHash = 0;
        if (0 == block->allocTotal)
            block->allocTotal = static_cast<uint32>(totalSize);

        if (tlsAllocScopeStack.IsCreated())
        {
            AllocScopeItem* scopeItem = tlsAllocScopeStack.Get();
            if (scopeItem != nullptr)
            {
                block->pool = scopeItem->allocPool;
            }
        }

        {
            LockType lock(allocMutex);
            block->tags = statGeneral.activeTags;
            block->orderNo = statGeneral.nextBlockNo++;
            InsertBlock(block);
        }
        {
            uint32 systemMemoryUsage = GetSystemMemoryUsage();
            LockType lock(statMutex);
            UpdateStatAfterAlloc(block, systemMemoryUsage);
        }
        if (!lightWeightMode)
        {
            Backtrace backtrace;
            CollectBacktrace(&backtrace, 1);
            block->bktraceHash = backtrace.hash;

            LockType lock(bktraceMutex);
            InsertBacktrace(backtrace);
        }
        return static_cast<void*>(block + 1);
    }
    return nullptr;
}

DAVA_NOINLINE void* MemoryManager::AlignedAllocate(size_t size, size_t align, uint32 poolIndex)
{
    // TODO: check whether size is integral multiple of align
    assert(size > 0);
    assert(align > 0 && 0 == (align & (align - 1))); // Check whether align is power of 2
    assert(ALLOC_POOL_TOTAL < poolIndex && poolIndex < MAX_ALLOC_POOL_COUNT);

    if (align < BLOCK_ALIGN)
    {
        align = BLOCK_ALIGN;
    }

    size_t totalSize = sizeof(MemoryBlock) + size + (align - 1);
    if (totalSize & (BLOCK_ALIGN - 1))
    {
        totalSize += (BLOCK_ALIGN - (totalSize & (BLOCK_ALIGN - 1)));
    }

    void* realPtr = MallocHook::Malloc(totalSize);
    if (realPtr != nullptr)
    {
        // Some pointer arithmetics
        uintptr_t aligned = uintptr_t(realPtr) + sizeof(MemoryBlock);
        aligned += align - (aligned & (align - 1));

        MemoryBlock* block = reinterpret_cast<MemoryBlock*>(aligned - sizeof(MemoryBlock));
        block->mark = BLOCK_MARK;
        block->pool = poolIndex;
        block->realBlockStart = realPtr;
        block->allocByApp = static_cast<uint32>(size);
        block->allocTotal = static_cast<uint32>(MallocHook::MallocSize(block->realBlockStart));
        block->bktraceHash = 0;
        if (0 == block->allocTotal)
            block->allocTotal = static_cast<uint32>(totalSize);

        if (tlsAllocScopeStack.IsCreated())
        {
            AllocScopeItem* scopeItem = tlsAllocScopeStack.Get();
            if (scopeItem != nullptr)
            {
                block->pool = scopeItem->allocPool;
            }
        }

        {
            LockType lock(allocMutex);
            block->tags = statGeneral.activeTags;
            block->orderNo = statGeneral.nextBlockNo++;
            InsertBlock(block);
        }
        {
            uint32 systemMemoryUsage = GetSystemMemoryUsage();
            LockType lock(statMutex);
            UpdateStatAfterAlloc(block, systemMemoryUsage);
        }
        if (!lightWeightMode)
        {
            Backtrace backtrace;
            CollectBacktrace(&backtrace, 1);
            block->bktraceHash = backtrace.hash;

            LockType lock(bktraceMutex);
            InsertBacktrace(backtrace);
        }
        return reinterpret_cast<void*>(aligned);
    }
    return nullptr;
}

void* MemoryManager::Reallocate(void* ptr, size_t newSize)
{
    if (nullptr == ptr)
    {
        return malloc(newSize);
    }

    MemoryBlock* block = static_cast<MemoryBlock*>(ptr) - 1;
    if (BLOCK_MARK == block->mark)
    {
        void* newPtr = malloc(newSize);
        if (newPtr != nullptr)
        {
            size_t n = block->allocByApp > newSize ? newSize : block->allocByApp;
            memcpy(newPtr, ptr, n);
            free(ptr);
            return newPtr;
        }
        else
            return nullptr;
    }
    else
    {
        return MallocHook::Realloc(ptr, newSize);
    }
}

bool IsMemoryAddressAccessible(void* blockStart)
{
// Sometimes memory allocations bypass memory manager, but memory freeing goes through memory manager.
// System can allocate such a memory block at the very beginning of heap, and memory manager should
// verify whether memory block is accessible (to prevent segmentation fault) to make check if block is
// tracked by memory manager

// Such behavior for now is observed only on OS X and iOS
#if defined(__DAVAENGINE_MACOS__)
    mach_vm_address_t addr = reinterpret_cast<mach_vm_address_t>(blockStart);
    mach_vm_size_t size = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t obj;
    kern_return_t status = mach_vm_region(mach_task_self(),
                                          &addr,
                                          &size,
                                          VM_REGION_BASIC_INFO_64,
                                          reinterpret_cast<vm_region_info_t>(&info),
                                          &count,
                                          &obj);
    if (0 == status &&
        addr <= reinterpret_cast<mach_vm_address_t>(blockStart) &&
        (info.protection & (VM_PROT_READ | VM_PROT_WRITE)) == (VM_PROT_READ | VM_PROT_WRITE))
    {
        return true;
    }
    return false;
#elif defined(__DAVAENGINE_IPHONE__)
// https://sourceforge.net/p/predef/wiki/Architectures/
#if defined(__aarch64__)
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
#else
    vm_region_basic_info_data_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO;
#endif
    vm_address_t addr = reinterpret_cast<vm_address_t>(blockStart);
    vm_size_t size = 0;
    mach_port_t obj;

#if defined(__aarch64__)
    kern_return_t status = vm_region_64(
#else
    kern_return_t status = vm_region(
#endif
    mach_task_self(),
    &addr,
    &size,
    flavor,
    reinterpret_cast<vm_region_info_t>(&info),
    &count,
    &obj);
    if (0 == status &&
        addr <= reinterpret_cast<vm_address_t>(blockStart) &&
        (info.protection & (VM_PROT_READ | VM_PROT_WRITE)) == (VM_PROT_READ | VM_PROT_WRITE))
    {
        return true;
    }
    return false;
#else
    return true;
#endif
}

void MemoryManager::Deallocate(void* ptr)
{
    if (ptr != nullptr)
    {
        void* ptrToFree = nullptr;
        MemoryBlock* block = static_cast<MemoryBlock*>(ptr) - 1;

        bool isAccessible = IsMemoryAddressAccessible(block);
        if (isAccessible && BLOCK_MARK == block->mark)
        {
            {
                LockType lock(allocMutex);
                RemoveBlock(block);
            }
            {
                uint32 systemMemoryUsage = GetSystemMemoryUsage();
                LockType lock(statMutex);
                UpdateStatAfterDealloc(block, systemMemoryUsage);
            }
            if (!lightWeightMode)
            {
                LockType lock(bktraceMutex);
                RemoveBacktrace(block->bktraceHash);
            }

            // Tracked memory block consists of header (of type struct MemoryBlock) and data block that returned to app.
            // Tracked memory blocks are distinguished by special mark in header.
            // In some cases, especially on iOS, memory allocations bypass memory manager, but memory freeing goes through
            // memory manager. So I need to erase header to properly free bypassed allocations as system can allocate memory at the same address
            block->mark = DEAD_BLOCK_MARK;
            ptrToFree = block->realBlockStart;
        }
        else
        {
            LockType lock(statMutex);
            statGeneral.ghostBlockCount += 1;
            statGeneral.ghostSize += static_cast<uint32>(MallocHook::MallocSize(ptr));
            ptrToFree = ptr;
        }
        MallocHook::Free(ptrToFree);
    }
}

uint32 MemoryManager::MemorySize(void* ptr)
{
    if (ptr != nullptr)
    {
        MemoryBlock* block = static_cast<MemoryBlock*>(ptr) - 1;

        bool isAccessible = IsMemoryAddressAccessible(block);
        if (isAccessible && BLOCK_MARK == block->mark)
        {
            return block->allocByApp;
        }
    }
    return static_cast<uint32>(MallocHook::MallocSize(ptr));
}

void* MemoryManager::InternalAllocate(size_t size)
{
    const size_t totalSize = sizeof(InternalMemoryBlock) + size;
    InternalMemoryBlock* block = static_cast<InternalMemoryBlock*>(MallocHook::Malloc(totalSize));
    if (block != nullptr)
    {
        block->mark = INTERNAL_BLOCK_MARK;
        block->allocByApp = static_cast<uint32>(size);
        block->allocTotal = static_cast<uint32>(MallocHook::MallocSize(block));
        if (0 == block->allocTotal)
            block->allocTotal = static_cast<uint32>(totalSize);

        // Update stat
        {
            LockType lock(statMutex);
            statGeneral.allocInternal += block->allocByApp;
            statGeneral.allocInternalTotal += block->allocTotal;
            statGeneral.internalBlockCount += 1;
        }
        return static_cast<void*>(block + 1);
    }
    return nullptr;
}

void MemoryManager::InternalDeallocate(void* ptr)
{
    if (ptr != nullptr)
    {
        InternalMemoryBlock* block = static_cast<InternalMemoryBlock*>(ptr) - 1;
        assert(INTERNAL_BLOCK_MARK == block->mark);

        // Update stat
        {
            LockType lock(statMutex);
            statGeneral.allocInternal -= block->allocByApp;
            statGeneral.allocInternalTotal -= block->allocTotal;
            statGeneral.internalBlockCount -= 1;
        }

        // Clear mark of deallocated block
        block->mark = DEAD_BLOCK_MARK;
        MallocHook::Free(block);
    }
}

uint32 MemoryManager::GetSystemMemoryUsage() const
{
#if defined(__DAVAENGINE_WINDOWS__)
    return 0;
#elif defined(__DAVAENGINE_APPLE__)
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    if (KERN_SUCCESS == task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &size))
    {
        return static_cast<uint32>(info.resident_size);
    }
    return 0;
#elif defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_LINUX__)
    // http://stackoverflow.com/questions/17109284/how-to-find-memory-usage-of-my-android-application-written-c-using-ndk
    // http://androidxref.com/source/xref/frameworks/base/core/jni/android_os_Debug.cpp (Jelly Bean 4.2)
    // For linux: http://man7.org/linux/man-pages/man3/mallinfo.3.html
    struct mallinfo info
    {
    };
    return static_cast<uint32>(info.uordblks);
#else
#error "Unknown platform"
#endif
}

uint32 MemoryManager::GetTrackedMemoryUsage(uint32 poolIndex) const
{
    assert(ALLOC_POOL_TOTAL <= poolIndex && poolIndex < MAX_ALLOC_POOL_COUNT);

    LockType lock(statMutex);
    return statAllocPool[poolIndex].allocByApp;
}

uint32 MemoryManager::GetTaggedMemoryUsage(uint32 tagIndex) const
{
    DVASSERT(tagIndex != 0 && IsPowerOf2(tagIndex));

    const size_t index = HighestBitIndex(tagIndex);

    DVASSERT(index < MAX_TAG_COUNT);

    LockType lock(statMutex);
    return statTag[index].allocByApp;
}

void MemoryManager::EnterTagScope(uint32 tag)
{
    DVASSERT(tag != 0 && IsPowerOf2(tag));
    DVASSERT((statGeneral.activeTags & tag) == 0); // Tag shouldn't be set earlier

    {
        LockType lock(allocMutex);
        statGeneral.activeTags |= tag;
        statGeneral.activeTagCount += 1;
    }
    if (tagCallback != nullptr)
    {
        tagCallback(tag, true);
    }
}

void MemoryManager::LeaveTagScope(uint32 tag)
{
    DVASSERT(tag != 0 && IsPowerOf2(tag));
    DVASSERT((statGeneral.activeTags & tag) == tag); // Tag should be set earlier

    {
        LockType lock(allocMutex);
        statGeneral.activeTags &= ~tag;
        statGeneral.activeTagCount -= 1;
    }
    if (tagCallback != nullptr)
    {
        tagCallback(tag, false);
    }
}

void MemoryManager::EnterAllocScope(uint32 allocPool)
{
    AllocScopeItem* newItem = new (InternalAllocate(sizeof(AllocScopeItem))) AllocScopeItem;
    newItem->next = tlsAllocScopeStack.Release();
    newItem->allocPool = allocPool;

    tlsAllocScopeStack.Reset(newItem);
}

void MemoryManager::LeaveAllocScope(uint32 allocPool)
{
    AllocScopeItem* topItem = tlsAllocScopeStack.Release();
    assert(topItem != nullptr);
    assert(topItem->allocPool == allocPool);

    tlsAllocScopeStack.Reset(topItem->next);
    InternalDeallocate(topItem);
}

void MemoryManager::TrackGpuAlloc(uint32 id, size_t size, uint32 gpuPoolIndex)
{
    LockType lock(gpuMutex);

    if (nullptr == gpuBlockMap)
    {
        static uint8 bufferForMap[sizeof(GpuBlockMap)];
        gpuBlockMap = new (bufferForMap) GpuBlockMap;
    }

    uint64 key = PackGPUKey(id, gpuPoolIndex);
    auto iter = gpuBlockMap->find(key);
    if (iter == gpuBlockMap->end())
    {
        MemoryBlock newBlock{};
        newBlock.orderNo = id; // Make use field 'orderNo' as GPU allocation id
        newBlock.pool = gpuPoolIndex;

        iter = gpuBlockMap->emplace(key, newBlock).first;
    }

    MemoryBlock& gpuBlock = iter->second;
    gpuBlock.allocByApp += static_cast<uint32>(size);
    gpuBlock.allocTotal = gpuBlock.allocByApp;
    gpuBlock.mark += 1; // Make use field 'mark' as number of GPU allocations with given id and pool index
    {
        LockType lock(statMutex);
        UpdateStatAfterGPUAlloc(&gpuBlock, size);
    }
}

void MemoryManager::TrackGpuDealloc(uint32 id, uint32 gpuPoolIndex)
{
    LockType lock(gpuMutex);

    uint64 key = PackGPUKey(id, gpuPoolIndex);
    auto iter = gpuBlockMap->find(key);
    DVASSERT(iter != gpuBlockMap->end());

    MemoryBlock& gpuBlock = iter->second;
    {
        LockType lock(statMutex);
        UpdateStatAfterGPUDealloc(&gpuBlock);
    }
    gpuBlockMap->erase(iter);
}

void MemoryManager::InsertBlock(MemoryBlock* block)
{
    if (head != nullptr)
    {
        block->next = head;
        block->prev = nullptr;
        head->prev = block;
        head = block;
    }
    else
    {
        block->next = nullptr;
        block->prev = nullptr;
        head = block;
    }
}

void MemoryManager::RemoveBlock(MemoryBlock* block)
{
    if (block->prev != nullptr)
        block->prev->next = block->next;
    if (block->next != nullptr)
        block->next->prev = block->prev;
    if (block == head)
        head = head->next;
}

void MemoryManager::UpdateStatAfterAlloc(MemoryBlock* block, uint32 systemMemoryUsage)
{
    { // Update memory usage reported by system
        statAllocPool[ALLOC_POOL_SYSTEM].allocByApp = systemMemoryUsage;
        statAllocPool[ALLOC_POOL_SYSTEM].allocTotal = systemMemoryUsage;
    }
    { // Update total statistics
        statAllocPool[ALLOC_POOL_TOTAL].allocByApp += block->allocByApp;
        statAllocPool[ALLOC_POOL_TOTAL].allocTotal += block->allocTotal;
        statAllocPool[ALLOC_POOL_TOTAL].blockCount += 1;

        if (block->allocByApp > statAllocPool[ALLOC_POOL_TOTAL].maxBlockSize)
            statAllocPool[ALLOC_POOL_TOTAL].maxBlockSize = block->allocByApp;
    }
    { // Update pool statistics
        const uint32 poolIndex = block->pool;
        statAllocPool[poolIndex].allocByApp += block->allocByApp;
        statAllocPool[poolIndex].allocTotal += block->allocTotal;
        statAllocPool[poolIndex].blockCount += 1;

        if (block->allocByApp > statAllocPool[poolIndex].maxBlockSize)
            statAllocPool[poolIndex].maxBlockSize = block->allocByApp;
    }

    { // Update tag statistics
        uint32 tags = statGeneral.activeTags;
        if (tags != 0)
        {
            for (size_t index = 0; tags != 0; ++index, tags >>= 1)
            {
                if (tags & 0x01)
                {
                    statTag[index].allocByApp += block->allocByApp;
                    statTag[index].blockCount += 1;
                }
            }
        }
        else
        {
            statTag[UNTAGGED].allocByApp += block->allocByApp;
            statTag[UNTAGGED].blockCount += 1;
        }
    }
}

void MemoryManager::UpdateStatAfterDealloc(MemoryBlock* block, uint32 systemMemoryUsage)
{
    { // Update memory usage reported by system
        statAllocPool[ALLOC_POOL_SYSTEM].allocByApp = systemMemoryUsage;
        statAllocPool[ALLOC_POOL_SYSTEM].allocTotal = systemMemoryUsage;
    }
    { // Update total statistics
        statAllocPool[ALLOC_POOL_TOTAL].allocByApp -= block->allocByApp;
        statAllocPool[ALLOC_POOL_TOTAL].allocTotal -= block->allocTotal;
        statAllocPool[ALLOC_POOL_TOTAL].blockCount -= 1;
    }
    { // Update pool statistics
        const uint32 poolIndex = block->pool;
        statAllocPool[poolIndex].allocByApp -= block->allocByApp;
        statAllocPool[poolIndex].allocTotal -= block->allocTotal;
        statAllocPool[poolIndex].blockCount -= 1;
    }
    { // Update tag statistics
        uint32 tags = block->tags;
        if (tags != 0)
        {
            for (size_t index = 0; tags != 0; ++index, tags >>= 1)
            {
                if (tags & 0x01)
                {
                    statTag[index].allocByApp -= block->allocByApp;
                    statTag[index].blockCount -= 1;
                }
            }
        }
        else
        {
            statTag[UNTAGGED].allocByApp -= block->allocByApp;
            statTag[UNTAGGED].blockCount -= 1;
        }
    }
}

void MemoryManager::UpdateStatAfterGPUAlloc(MemoryBlock* block, size_t sizeIncr)
{
    { // Update total statistics
        statAllocPool[ALLOC_POOL_TOTAL].allocByApp += static_cast<uint32>(sizeIncr);
        statAllocPool[ALLOC_POOL_TOTAL].allocTotal += static_cast<uint32>(sizeIncr);
    }
    { // Update pool statistics
        statAllocPool[block->pool].allocByApp += static_cast<uint32>(sizeIncr);
        statAllocPool[block->pool].allocTotal += static_cast<uint32>(sizeIncr);
        statAllocPool[block->pool].blockCount += 1;

        if (block->allocByApp > statAllocPool[block->pool].maxBlockSize)
            statAllocPool[block->pool].maxBlockSize = block->allocByApp;
    }
}

void MemoryManager::UpdateStatAfterGPUDealloc(MemoryBlock* block)
{
    { // Update total statistics
        statAllocPool[ALLOC_POOL_TOTAL].allocByApp -= block->allocByApp;
        statAllocPool[ALLOC_POOL_TOTAL].allocTotal -= block->allocTotal;
    }
    { // Update pool statistics
        statAllocPool[block->pool].allocByApp -= block->allocByApp;
        statAllocPool[block->pool].allocTotal -= block->allocTotal;
        statAllocPool[block->pool].blockCount -= block->mark;
    }
}

void MemoryManager::InsertBacktrace(Backtrace& backtrace)
{
    if (nullptr == bktraceMap)
    {
        static uint8 bufferForMap[sizeof(BacktraceMap)];

        bktraceMap = new (bufferForMap) BacktraceMap;
    }

    auto i = bktraceMap->find(backtrace.hash);
    if (i == bktraceMap->end())
    {
        backtrace.nref = 1;
        backtrace.symbolsCollected = false;

        const size_t BKTRACE_THRESHOLD = 100;
        bktraceGrowDelta += 1;
        if (bktraceGrowDelta >= BKTRACE_THRESHOLD)
        {
            bktraceGrowDelta -= BKTRACE_THRESHOLD;
            symbolCollectorCondVar.NotifyOne();
        }
        bktraceMap->emplace(backtrace.hash, backtrace);
    }
    else
    {
        i->second.nref += 1;
    }
}

void MemoryManager::RemoveBacktrace(uint32 hash)
{
    auto i = bktraceMap->find(hash);
    i->second.nref -= 1;
    if (i->second.nref == 0)
    {
        bktraceMap->erase(i);
    }
}

DAVA_NOINLINE void MemoryManager::CollectBacktrace(Backtrace* backtrace, size_t nskip)
{
    const size_t EXTRA_FRAMES = 5;
    const size_t FRAMES_COUNT = BACKTRACE_DEPTH + EXTRA_FRAMES;
    void* frames[FRAMES_COUNT] = { nullptr };
    Memset(backtrace, 0, sizeof(Backtrace));

    Debug::GetBacktrace(frames, FRAMES_COUNT);

    auto PointerToString = [](void* ptr, char* buf) -> size_t {
        auto hex = [](uint8 n) -> char { return n < 10 ? n + '0' : n - 10 + 'A'; };

        uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
        for (size_t i = 0; i < sizeof(uintptr_t); ++i)
        {
            uint8 byte = static_cast<uint8>(v);
            *buf = hex(byte & 0x0F);
            buf += 1;
            *buf = hex(byte >> 4);
            buf += 1;
            v >>= 8;
        }
        return sizeof(uintptr_t) * 2;
    };

    // Convert backtrace to string and compute that string's hash to greatly decrease hash collision
    // TODO: think about another method of backtrace identification
    const size_t MAX_BACKTRACE_STRING_LENGTH = sizeof(void*) * 2 * BACKTRACE_DEPTH;
    char8 bktraceString[MAX_BACKTRACE_STRING_LENGTH];
    char8* strPtr = bktraceString;
    size_t bktraceStringLength = 0;

    for (size_t idst = 0, isrc = nskip + 1; idst < BACKTRACE_DEPTH; ++idst, ++isrc)
    {
        backtrace->frames[idst] = frames[isrc];

        size_t n = PointerToString(frames[isrc], strPtr);
        strPtr += n;
        bktraceStringLength += n;
    }
    backtrace->hash = HashValue_N(bktraceString, static_cast<uint32>(bktraceStringLength));
    backtrace->nref = 1;
    backtrace->symbolsCollected = false;
}

void MemoryManager::ObtainBacktraceSymbols(const Backtrace* backtrace)
{
    if (nullptr == symbolMap)
    {
        static uint8 bufferForMap[sizeof(SymbolMap)];
        symbolMap = new (bufferForMap) SymbolMap;
    }

    for (size_t i = 0; i < backtrace->frames.size(); ++i)
    {
        if (backtrace->frames[i] != nullptr && symbolMap->find(backtrace->frames[i]) == symbolMap->cend())
        {
            String symbol = Debug::GetFrameSymbol(backtrace->frames[i], true);
            if (!symbol.empty())
                symbolMap->emplace(backtrace->frames[i], InternalString(symbol.c_str()));
        }
    }
}

uint32 MemoryManager::CalcStatConfigSize() const
{
    return sizeof(MMStatConfig) + sizeof(MMItemName) * registeredAllocPoolCount + sizeof(MMItemName) * (registeredTagCount + 1);
}

void MemoryManager::GetStatConfig(void* buffer, uint32 bufSize) const
{
    const uint32 requiredSize = CalcStatConfigSize();
    DVASSERT(requiredSize <= bufSize);

    MMStatConfig* config = static_cast<MMStatConfig*>(buffer);
    config->size = static_cast<uint32>(requiredSize);
    config->allocPoolCount = static_cast<uint32>(registeredAllocPoolCount);
    config->tagCount = static_cast<uint32>(registeredTagCount + 1);
    config->bktraceDepth = BACKTRACE_DEPTH;

    MMItemName* names = OffsetPointer<MMItemName>(config, sizeof(MMStatConfig));
    for (uint32 i = 0; i < registeredAllocPoolCount; ++i, ++names)
    {
        *names = allocPoolNames[i];
    }
    for (uint32 i = 0; i < registeredTagCount; ++i, ++names)
    {
        *names = tagNames[i];
    }
    *names = tagNames[UNTAGGED];
}

uint32 MemoryManager::CalcCurStatSize() const
{
    return sizeof(MMCurStat) + sizeof(AllocPoolStat) * registeredAllocPoolCount + sizeof(TagAllocStat) * (registeredTagCount + 1);
}

void MemoryManager::GetCurStat(uint64 timestamp, void* buffer, uint32 bufSize) const
{
    const uint32 requiredSize = CalcCurStatSize();
    DVASSERT(requiredSize <= bufSize);

    LockType lockAlloc(allocMutex);
    LockType lockStat(statMutex);

    MMCurStat* curStat = static_cast<MMCurStat*>(buffer);
    curStat->timestamp = timestamp;
    curStat->size = static_cast<uint32>(requiredSize);
    curStat->statGeneral = statGeneral;

    AllocPoolStat* pools = OffsetPointer<AllocPoolStat>(curStat, sizeof(MMCurStat));
    for (uint32 i = 0; i < registeredAllocPoolCount; ++i)
    {
        pools[i] = statAllocPool[i];
    }

    TagAllocStat* tags = OffsetPointer<TagAllocStat>(pools, sizeof(AllocPoolStat) * registeredAllocPoolCount);
    for (uint32 i = 0; i < registeredTagCount; ++i)
    {
        tags[i] = statTag[i];
    }
    tags[registeredTagCount] = statTag[UNTAGGED];
}

bool MemoryManager::GetMemorySnapshot(uint64 timestamp, File* file, uint32* snapshotSize)
{
    if (lightWeightMode)
    { // In lightweight mode snapshot has no sense as it doesn't contains backtraces and symbols
        if (snapshotSize != nullptr)
        {
            *snapshotSize = 0;
        }
        return false;
    }

    assert(file != nullptr);

    const uint32 BUF_SIZE = 64 * 1024;
    std::vector<uint8, InternalAllocator<uint8>> v(BUF_SIZE); // For automatic memory management
    void* buffer = static_cast<void*>(&*v.begin()); // For convenience

    const uint32 bktraceSize = sizeof(MMBacktrace) + sizeof(uint64) * BACKTRACE_DEPTH;

    MMSnapshot snapshot{};
    snapshot.timestamp = timestamp;
    snapshot.dataOffset = sizeof(MMSnapshot);
    snapshot.bktraceDepth = BACKTRACE_DEPTH;

    // Write empty header to force file internal buffer allocation to exclude
    // memory allocations under allocMutex (primarily for Win32 release builds)
    if (file->Write(&snapshot) != sizeof(MMSnapshot))
        return false;

    { // Store memory blocks into file
        LockType lock(allocMutex);

        const uint32 BLOCKS_IN_BUF = BUF_SIZE / sizeof(MMBlock);
        MMBlock* destBegin = static_cast<MMBlock*>(buffer);

        MemoryBlock* curBlock = head;
        while (curBlock != nullptr)
        {
            uint32 k = 0;
            for (; k < BLOCKS_IN_BUF && curBlock != nullptr; ++k)
            {
                MMBlock& dstBlock = destBegin[k];
                dstBlock.orderNo = curBlock->orderNo;
                dstBlock.allocByApp = curBlock->allocByApp;
                dstBlock.allocTotal = curBlock->allocTotal;
                dstBlock.bktraceHash = curBlock->bktraceHash;
                dstBlock.pool = curBlock->pool;
                dstBlock.tags = curBlock->tags;

                curBlock = curBlock->next;
            }
            snapshot.blockCount += k;
            if (file->Write(buffer, sizeof(MMBlock) * k) != sizeof(MMBlock) * k)
                return false;
        }
    }
    { // Store function names into file
        LockType lock(bktraceMutex);

        const size_t SYMBOLS_IN_BUF = BUF_SIZE / sizeof(MMSymbol);
        MMSymbol* symbols = static_cast<MMSymbol*>(buffer);

        auto itBegin = symbolMap->cbegin();
        auto itEnd = symbolMap->cend();
        while (itBegin != itEnd)
        {
            uint32 k = 0;
            for (; k < SYMBOLS_IN_BUF && itBegin != itEnd; ++k)
            {
                void* addr = itBegin->first;
                auto& name = itBegin->second;

                MMSymbol& dstSymbol = symbols[k];
                dstSymbol.addr = reinterpret_cast<uint64>(addr);
                strncpy(dstSymbol.name, name.c_str(), MMSymbol::NAME_LENGTH);
                dstSymbol.name[MMSymbol::NAME_LENGTH - 1] = '\0';

                ++itBegin;
            }
            snapshot.symbolCount += k;
            if (file->Write(buffer, sizeof(MMSymbol) * k) != sizeof(MMSymbol) * k)
                return false;
        }
    }
    { // Store backtraces into file
        LockType lock(bktraceMutex);

        const size_t BKTRACE_IN_BUF = BUF_SIZE / bktraceSize;
        MMBacktrace* bktrace = static_cast<MMBacktrace*>(buffer);

        auto itBegin = bktraceMap->cbegin();
        auto itEnd = bktraceMap->cend();
        while (itBegin != itEnd)
        {
            uint32 k = 0;
            for (; k < BKTRACE_IN_BUF && itBegin != itEnd; ++k)
            {
                auto& o = itBegin->second;

                bktrace->hash = o.hash;
                uint64* frames = OffsetPointer<uint64>(bktrace, sizeof(MMBacktrace));
                for (size_t i = 0; i < BACKTRACE_DEPTH; ++i)
                {
                    frames[i] = reinterpret_cast<uint64>(o.frames[i]);
                }

                bktrace = OffsetPointer<MMBacktrace>(bktrace, bktraceSize);
                ++itBegin;
            }
            snapshot.bktraceCount += k;
            if (file->Write(buffer, bktraceSize * k) != bktraceSize * k)
                return false;
            bktrace = static_cast<MMBacktrace*>(buffer);
        }
    }

    // Write down header
    const uint32 totalSize = sizeof(MMSnapshot)
    + sizeof(MMBlock) * snapshot.blockCount
    + sizeof(MMSymbol) * snapshot.symbolCount
    + bktraceSize * snapshot.bktraceCount;
    if (snapshotSize != nullptr)
    {
        *snapshotSize = totalSize;
    }
    snapshot.size = totalSize;

    assert(file->GetPos() == totalSize);

    file->Seek(0, File::SEEK_FROM_START);
    if (file->Write(&snapshot) != sizeof(MMSnapshot))
        return false;
    return true;
}

void MemoryManager::SymbolCollectorThread()
{
    const size_t BUF_CAPACITY = 1000; // Select some reasonable buffer for backtraces
    // to give some job to symbol collector
    Backtrace* bktraceBuf = new Backtrace[BUF_CAPACITY];

    while (!symbolCollectorThread->IsCancelling())
    {
        {
            UniqueLock<Mutex> lock(symbolCollectorMutex);
            symbolCollectorCondVar.Wait(lock);
        }

        size_t nplaced = 0;
        {
            LockType lock(bktraceMutex);

            for (auto i = bktraceMap->begin(), e = bktraceMap->end(); i != e && nplaced < BUF_CAPACITY; ++i)
            {
                const Backtrace& bktrace = i->second;
                if (!bktrace.symbolsCollected)
                {
                    bktraceBuf[nplaced] = bktrace;
                    nplaced += 1;
                }
            }
        }

        for (size_t i = 0; i < nplaced && !symbolCollectorThread->IsCancelling(); ++i)
        {
            ObtainBacktraceSymbols(&bktraceBuf[i]);

            LockType lock(bktraceMutex);
            auto ibktrace = bktraceMap->find(bktraceBuf[i].hash);
            if (ibktrace != bktraceMap->end())
            {
                ibktrace->second.symbolsCollected = true;
            }
        }
    }
}

} // namespace DAVA

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)

ENUM_DECLARE(DAVA::ePredefAllocPools)
{
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_TOTAL, "ALLOC_POOL_TOTAL");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_DEFAULT, "ALLOC_POOL_DEFAULT");
    ENUM_ADD_DESCR(DAVA::ALLOC_GPU_TEXTURE, "ALLOC_GPU_TEXTURE");
    ENUM_ADD_DESCR(DAVA::ALLOC_GPU_RDO_VERTEX, "ALLOC_GPU_RDO_VERTEX");
    ENUM_ADD_DESCR(DAVA::ALLOC_GPU_RDO_INDEX, "ALLOC_GPU_RDO_INDEX");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_SYSTEM, "ALLOC_POOL_SYSTEM");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_FMOD, "ALLOC_POOL_FMOD");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_BULLET, "ALLOC_POOL_BULLET");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_BASEOBJECT, "ALLOC_POOL_BASEOBJECT");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_POLYGONGROUP, "ALLOC_POOL_POLYGONGROUP");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_COMPONENT, "ALLOC_POOL_COMPONENT");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_ENTITY, "ALLOC_POOL_ENTITY");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_LANDSCAPE, "ALLOC_POOL_LANDSCAPE");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_IMAGE, "ALLOC_POOL_IMAGE");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_TEXTURE, "ALLOC_POOL_TEXTURE");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_NMATERIAL, "ALLOC_POOL_NMATERIAL");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_RHI_BUFFER, "ALLOC_POOL_RHI_BUFFER");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_RHI_VERTEX_MAP, "ALLOC_POOL_RHI_VERTEX_MAP");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_RHI_INDEX_MAP, "ALLOC_POOL_RHI_INDEX_MAP");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_RHI_TEXTURE_MAP, "ALLOC_POOL_RHI_TEXTURE_MAP");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_RHI_RESOURCE_POOL, "ALLOC_POOL_RHI_RESOURCE_POOL");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_LUA, "ALLOC_POOL_LUA");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_SQLITE, "ALLOC_POOL_SQLITE");
    ENUM_ADD_DESCR(DAVA::ALLOC_POOL_PHYSICS, "ALLOC_POOL_PHYSICS");
};
