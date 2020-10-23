#ifndef __DAVAENGINE_MEMORYMANAGER_H__
#define __DAVAENGINE_MEMORYMANAGER_H__

#include "Base/BaseTypes.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include <type_traits>

#include "Functional/Function.h"
#include "Concurrency/Spinlock.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/ThreadLocalPtr.h"
#include "Concurrency/ConditionVariable.h"
#include "Concurrency/Mutex.h"

#include "MemoryManager/AllocPools.h"
#include "MemoryManager/MemoryManagerTypes.h"
#include "MemoryManager/InternalAllocator.h"

namespace DAVA
{
class File;
class Thread;
class BaseObject;

/*
 MemoryManager
*/
class MemoryManager final
{
    static const uint32 BLOCK_MARK = 0xBA0BAB;
    static const uint32 INTERNAL_BLOCK_MARK = 0x55AACC11;
    static const uint32 DEAD_BLOCK_MARK = 0xECECECEC;
    static const size_t BLOCK_ALIGN = 16;
    static const uint32 BACKTRACE_DEPTH = 32;

public:
    static const uint32 MAX_ALLOC_POOL_COUNT = 32;
    static const uint32 MAX_TAG_COUNT = 32;
    static const uint32 UNTAGGED = MAX_TAG_COUNT - 1;

    struct MemoryBlock;
    struct InternalMemoryBlock;
    struct Backtrace;
    struct AllocScopeItem;

public:
    class AllocPoolScope final
    {
    public:
        explicit AllocPoolScope(int32 aPool)
            : allocPool(aPool)
        {
            MemoryManager::Instance()->EnterAllocScope(allocPool);
        }
        ~AllocPoolScope()
        {
            MemoryManager::Instance()->LeaveAllocScope(allocPool);
        }

    private:
        int32 allocPool;
    };

public:
    static MemoryManager* Instance();

    static void RegisterAllocPoolName(uint32 index, const char8* name);
    static void RegisterTagName(uint32 tagMask, const char8* name);

    void EnableLightWeightMode();
    void SetCallbacks(Function<void()> updateCallback, Function<void(uint32, bool)> tagCallback);
    void Update();
    void Finish();

    DAVA_NOINLINE void* Allocate(size_t size, uint32 poolIndex);
    DAVA_NOINLINE void* AlignedAllocate(size_t size, size_t align, uint32 poolIndex);
    void* Reallocate(void* ptr, size_t newSize);
    void Deallocate(void* ptr);
    uint32 MemorySize(void* ptr);

    void EnterTagScope(uint32 tag);
    void LeaveTagScope(uint32 tag);

    void EnterAllocScope(uint32 allocPool);
    void LeaveAllocScope(uint32 allocPool);

    void TrackGpuAlloc(uint32 id, size_t size, uint32 gpuPoolIndex);
    void TrackGpuDealloc(uint32 id, uint32 gpuPoolIndex);

    uint32 GetSystemMemoryUsage() const;
    uint32 GetTrackedMemoryUsage(uint32 poolIndex = ALLOC_POOL_TOTAL) const;

    uint32 GetTaggedMemoryUsage(uint32 tagIndex) const;

    uint32 CalcStatConfigSize() const;
    void GetStatConfig(void* buffer, uint32 bufSize) const;

    uint32 CalcCurStatSize() const;
    void GetCurStat(uint64 timestamp, void* buffer, uint32 bufSize) const;

    bool GetMemorySnapshot(uint64 timestamp, File* file, uint32* snapshotSize = nullptr);

private:
    // Make construtor and destructor private to disallow external creation of MemoryManager
    MemoryManager();
    ~MemoryManager() = default;

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    // Methods for memory allocating for internal data structures
    void* InternalAllocate(size_t size);
    void InternalDeallocate(void* ptr);

    // Make these functions friends to allow access to InternalAllocate and InternalDeallocate methods
    friend void* InternalAlloc(size_t size);
    friend void InternalDealloc(void* ptr);

private:
    void InsertBlock(MemoryBlock* block);
    void RemoveBlock(MemoryBlock* block);

    void UpdateStatAfterAlloc(MemoryBlock* block, uint32 systemMemoryUsage);
    void UpdateStatAfterDealloc(MemoryBlock* block, uint32 systemMemoryUsage);

    void UpdateStatAfterGPUAlloc(MemoryBlock* block, size_t sizeIncr);
    void UpdateStatAfterGPUDealloc(MemoryBlock* block);

    uint64 PackGPUKey(uint32 id, uint32 allocPool) const;

    void InsertBacktrace(Backtrace& backtrace);
    void RemoveBacktrace(uint32 hash);

    DAVA_NOINLINE void CollectBacktrace(Backtrace* backtrace, size_t nskip);
    void ObtainBacktraceSymbols(const Backtrace* backtrace);

    void SymbolCollectorThread();

private:
    MemoryBlock* head = nullptr; // Linked list of tracked memory blocks

    GeneralAllocStat statGeneral; // General statistics
    AllocPoolStat statAllocPool[MAX_ALLOC_POOL_COUNT]; // Statistics by allocation pools
    TagAllocStat statTag[MAX_TAG_COUNT]; // Statistics by tags

    using MutexType = Spinlock;
    using LockType = LockGuard<MutexType>;

    mutable MutexType allocMutex; // Mutex for managing list of allocated memory blocks
    mutable MutexType statMutex; // Mutex for updating memory statistics
    mutable MutexType gpuMutex; // Mutex for managing GPU allocations

    using GpuBlockMap = std::unordered_map<uint64, MemoryBlock, std::hash<uint64>, std::equal_to<uint64>, InternalAllocator<std::pair<const uint64, MemoryBlock>>>;

    GpuBlockMap* gpuBlockMap = nullptr;

    using InternalString = std::basic_string<char8, std::char_traits<char8>, InternalAllocator<char8>>;
    using BacktraceMap = std::unordered_map<uint32, Backtrace, std::hash<uint32>, std::equal_to<uint32>, InternalAllocator<std::pair<const uint32, Backtrace>>>;
    using SymbolMap = std::unordered_map<void*, InternalString, std::hash<void*>, std::equal_to<void*>, InternalAllocator<std::pair<void* const, InternalString>>>;

    mutable MutexType bktraceMutex; // Mutex for working with backtraces

    BacktraceMap* bktraceMap = nullptr;
    SymbolMap* symbolMap = nullptr;

    Thread* symbolCollectorThread = nullptr;
    ConditionVariable symbolCollectorCondVar;
    Mutex symbolCollectorMutex;
    size_t bktraceGrowDelta = 0;
    bool lightWeightMode = false; // Flag enabling lightweight mode: no backtrace and symbols, should increase performance

    Function<void()> updateCallback;
    Function<void(uint32, bool)> tagCallback;

private:
    // Make the following data members static to allow initialization of predefined values not in constructor
    static uint32 registeredTagCount; // Number of registered tags
    static uint32 registeredAllocPoolCount; // Number of registered allocation pools including predefined

    static MMItemName tagNames[MAX_TAG_COUNT]; // Names of tags
    static MMItemName allocPoolNames[MAX_ALLOC_POOL_COUNT]; // Names of allocation pools

    ThreadLocalPtr<AllocScopeItem> tlsAllocScopeStack;
};

//////////////////////////////////////////////////////////////////////////
inline uint64 MemoryManager::PackGPUKey(uint32 id, uint32 allocPool) const
{
    return static_cast<uint64>(id) | (static_cast<uint64>(allocPool) << 32);
}

} // namespace DAVA

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)

#endif // __DAVAENGINE_MEMORYMANAGER_H__
