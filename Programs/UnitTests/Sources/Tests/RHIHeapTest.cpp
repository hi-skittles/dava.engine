#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"
#include "Render/RHI/Common/rhi_RemoteHeap.h"

using namespace DAVA;

namespace RHIHeapTestDetails
{
const uint32 MaxBlockCount = 8192;
const uint32 PageSize = 2 * 1024 * 1024;
uint8* BaseAddress = reinterpret_cast<uint8*>(0x0100);
using TestHeap = rhi::SimpleRemoteHeap;
}

DAVA_TESTCLASS (RHIHeapTest)
{
    void AllocFreeInterleaved(RHIHeapTestDetails::TestHeap * heap)
    {
        uint32 allocMinSize = 32;
        uint32 allocMaxSize = 256;
        uint32 iterations = RHIHeapTestDetails::PageSize / allocMaxSize;

        List<uint8*> pointers;
        for (uint32 i = 0; i < iterations; ++i)
        {
            uint32 allocSize = allocMinSize + rand() % (allocMaxSize - allocMinSize);
            uint8* ptr = reinterpret_cast<uint8*>(heap->alloc(allocSize));

            if (ptr != nullptr)
                pointers.emplace_back(ptr);

            if ((ptr == nullptr) || ((i > iterations / 8) && (i % 2 == 0)))
            {
                auto it = pointers.begin();
                std::advance(it, rand() % pointers.size());
                heap->free(*it);
                pointers.erase(it);
            }
        };

        while (!pointers.empty())
        {
            auto it = pointers.begin();
            std::advance(it, rand() % pointers.size());
            heap->free(*it);
            pointers.erase(it);
        }
    }

    DAVA_TEST (Alloc_Free)
    {
        RHIHeapTestDetails::TestHeap heap(RHIHeapTestDetails::MaxBlockCount);
        heap.initialize(RHIHeapTestDetails::BaseAddress, RHIHeapTestDetails::PageSize);
        TEST_VERIFY(heap.Empty());
        uint32 allocSize = 2;
        while (allocSize <= RHIHeapTestDetails::PageSize)
        {
            uint8* ptr = reinterpret_cast<uint8*>(heap.alloc(allocSize));
            TEST_VERIFY(ptr != nullptr);
            TEST_VERIFY(ptr >= RHIHeapTestDetails::BaseAddress);
            TEST_VERIFY(ptr <= RHIHeapTestDetails::BaseAddress + RHIHeapTestDetails::PageSize);

            heap.free(ptr);
            TEST_VERIFY(heap.Empty());

            allocSize *= 2;
        }
        TEST_VERIFY(heap.Empty());
    }

    DAVA_TEST (AllocAll_FreeAll)
    {
        RHIHeapTestDetails::TestHeap heap(RHIHeapTestDetails::MaxBlockCount);
        heap.initialize(RHIHeapTestDetails::BaseAddress, RHIHeapTestDetails::PageSize);
        TEST_VERIFY(heap.Empty());

        uint32 allocSize = 256;
        uint32 allocCount = RHIHeapTestDetails::PageSize / allocSize;

        Vector<uint8*> pointers(allocCount);
        for (uint32 i = 0; i < allocCount; ++i)
        {
            pointers[i] = reinterpret_cast<uint8*>(heap.alloc(allocSize));
            TEST_VERIFY(pointers[i] != nullptr);
            TEST_VERIFY(pointers[i] >= RHIHeapTestDetails::BaseAddress);
            TEST_VERIFY(pointers[i] <= RHIHeapTestDetails::BaseAddress + RHIHeapTestDetails::PageSize);
        }
        TEST_VERIFY(heap.Empty() == false);

        for (uint32 i = 0; i < allocCount; ++i)
        {
            heap.free(pointers[i]);
        }

        TEST_VERIFY(heap.Empty());
    }

    DAVA_TEST (Alloc_Free_Interleaved)
    {
        rhi::SimpleRemoteHeap heap(RHIHeapTestDetails::MaxBlockCount);
        heap.initialize(RHIHeapTestDetails::BaseAddress, RHIHeapTestDetails::PageSize);
        TEST_VERIFY(heap.Empty());
        AllocFreeInterleaved(&heap);
        TEST_VERIFY(heap.Empty());
    }

    /*
     * Remote heap is not thread-safe anymore, sorry
     *
    DAVA_TEST(Alloc_Free_Interleaved_Multithreaded)
    {
        rhi::SimpleRemoteHeap heap(RHIHeapTestDetails::MaxBlockCount);
        heap.initialize(RHIHeapTestDetails::BaseAddress, RHIHeapTestDetails::PageSize);
        TEST_VERIFY(heap.Empty());

        Function<void()> fn = Bind(&RHIHeapTest::AllocFreeInterleaved, this, &heap);
        ScopedPtr<Thread> t1(Thread::Create(fn));
        ScopedPtr<Thread> t2(Thread::Create(fn));
        ScopedPtr<Thread> t3(Thread::Create(fn));
        t1->Start();
        t2->Start();
        t3->Start();
        t1->Join();
        t2->Join();
        t3->Join();

        TEST_VERIFY(heap.Empty());
    }
    // */
};
