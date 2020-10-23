#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include "MemoryManager/MemoryProfiler.h"

DAVA_TESTCLASS (MemoryManagerTest)
{
    volatile uint32 capturedTag = 0;
    volatile uint32 capturedCheckpoint = 0;

    DAVA_TEST (TestZeroAlloc)
    {
        void* ptr1 = MemoryManager::Instance()->Allocate(0, ALLOC_POOL_DEFAULT);
        void* ptr2 = MemoryManager::Instance()->Allocate(0, ALLOC_POOL_DEFAULT);

        // When allocating zero bytes MemoryManager returns unique pointer which can be safely deallocated
        TEST_VERIFY(ptr1 != nullptr);
        TEST_VERIFY(ptr1 != ptr2);

        MemoryManager::Instance()->Deallocate(ptr1);
        MemoryManager::Instance()->Deallocate(ptr2);
    }

    DAVA_TEST (TestAlignedAlloc)
    {
        // Alignment should be power of 2
        size_t align[] = {
            2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
        };
        for (auto x : align)
        {
            void* ptr = MemoryManager::Instance()->AlignedAllocate(128, x, ALLOC_POOL_DEFAULT);
            // On many platforms std::size_t can safely store the value of any non-member pointer, in which case it is synonymous with std::uintptr_t.
            size_t addr = reinterpret_cast<size_t>(ptr);
            TEST_VERIFY(addr % x == 0);
            MemoryManager::Instance()->Deallocate(ptr);
        }
    }

    DAVA_TEST (TestGPUTracking)
    {
        const size_t statSize = MemoryManager::Instance()->CalcCurStatSize();
        void* buffer = ::operator new(statSize);
        MMCurStat* stat = static_cast<MMCurStat*>(buffer);
        AllocPoolStat* poolStat = OffsetPointer<AllocPoolStat>(buffer, sizeof(MMCurStat));

        MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));
        uint32 oldAllocByApp = poolStat[ALLOC_GPU_TEXTURE].allocByApp;
        uint32 oldBlockCount = poolStat[ALLOC_GPU_TEXTURE].blockCount;

        DAVA_MEMORY_PROFILER_GPU_ALLOC(101, 100, ALLOC_GPU_TEXTURE);
        DAVA_MEMORY_PROFILER_GPU_ALLOC(101, 200, ALLOC_GPU_TEXTURE);

        MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));
        TEST_VERIFY(oldAllocByApp + 300 == poolStat[ALLOC_GPU_TEXTURE].allocByApp);
        TEST_VERIFY(oldBlockCount + 2 == poolStat[ALLOC_GPU_TEXTURE].blockCount);

        DAVA_MEMORY_PROFILER_GPU_DEALLOC(101, ALLOC_GPU_TEXTURE);
        MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));

        TEST_VERIFY(oldAllocByApp == poolStat[ALLOC_GPU_TEXTURE].allocByApp);
        TEST_VERIFY(oldBlockCount == poolStat[ALLOC_GPU_TEXTURE].blockCount);

        ::operator delete(buffer);
    }

    DAVA_TEST (TestAllocScope)
    {
        const size_t statSize = MemoryManager::Instance()->CalcCurStatSize();
        void* buffer = ::operator new(statSize);
        MMCurStat* stat = static_cast<MMCurStat*>(buffer);
        AllocPoolStat* poolStat = OffsetPointer<AllocPoolStat>(buffer, sizeof(MMCurStat));

        {
            MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));
            uint32 oldAllocByApp = poolStat[ALLOC_POOL_BULLET].allocByApp;
            uint32 oldBlockCount = poolStat[ALLOC_POOL_BULLET].blockCount;

            DAVA_MEMORY_PROFILER_ALLOC_SCOPE(ALLOC_POOL_BULLET);

            // Use volatile keyword to prevent optimizer to throw allocations out
            uint32 allocDelta = 0;
            uint32 blockDelta = 0;

#if !defined(__DAVAENGINE_WIN_UAP__) && !(defined(_WIN64))
            // For now memory profiler intercepts only operators new/delete but not functions malloc/free
            // TODO: remove define after full memory profiler implementation on win10
            void* volatile ptr1 = malloc(222);
            allocDelta += 222;
            blockDelta += 1;
#endif
            char* volatile ptr2 = new char[111];
            allocDelta += 111;
            blockDelta += 1;

            MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));
            TEST_VERIFY(oldAllocByApp + allocDelta == poolStat[ALLOC_POOL_BULLET].allocByApp);
            TEST_VERIFY(oldBlockCount + blockDelta == poolStat[ALLOC_POOL_BULLET].blockCount);

#if !defined(__DAVAENGINE_WIN_UAP__) && !(defined(_WIN64))
            free(ptr1);
#endif
            delete[] ptr2;

            MemoryManager::Instance()->GetCurStat(0, buffer, static_cast<uint32>(statSize));
            TEST_VERIFY(oldAllocByApp == poolStat[ALLOC_POOL_BULLET].allocByApp);
            TEST_VERIFY(oldBlockCount == poolStat[ALLOC_POOL_BULLET].blockCount);
        }

        ::operator delete(buffer);
    }

    DAVA_TEST (TestCallback)
    {
        const uint32 TAG = 1;

        MemoryManager::Instance()->SetCallbacks(nullptr, MakeFunction(this, &MemoryManagerTest::TagCallback));

        DAVA_MEMORY_PROFILER_ENTER_TAG(TAG);
        DAVA_MEMORY_PROFILER_LEAVE_TAG(TAG);

        TEST_VERIFY(enteredTag == TAG);
        TEST_VERIFY(leftTag == TAG);
    }

    void TagCallback(uint32 tag, bool entering)
    {
        if (entering)
            enteredTag = tag;
        else
            leftTag = tag;
    }

    volatile uint32 enteredTag = 0;
    volatile uint32 leftTag = 0;
};

#endif // DAVA_MEMORY_PROFILING_ENABLE
