#include "Concurrency/SyncBarrier.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "UnitTests/UnitTests.h"

#include <mutex>

using namespace DAVA;

DAVA_TESTCLASS (SyncBarrierTest)
{
    static const int NTHREADS = 10;
    SyncBarrier barrier;
    Vector<Thread*> threads;
    std::atomic<int> counter1;
    std::atomic<int> counter2;
    bool testComplete = false;
    std::once_flag onceFlag;

    SyncBarrierTest()
        : barrier(NTHREADS)
    {
    }

    bool TestComplete(const String&)const override
    {
        if (testComplete)
        {
            for (Thread* t : threads)
            {
                t->Release();
            }
        }
        return testComplete;
    }

    DAVA_TEST (Test)
    {
        for (int i = 0; i < NTHREADS; ++i)
        {
            Thread* t = Thread::Create([this, i]() { ThreadFunc(i); });
            t->Start();
            threads.push_back(t);
        }
    }

    void ThreadFunc(int n)
    {
        for (int i = 0; i < 10; ++i)
        {
            counter1 = 0;
            counter2 = 0;
            barrier.Wait();

            counter1 += 1;
            Thread::Sleep((n % 4) * 100);
            barrier.Wait();
            TEST_VERIFY(counter1 == NTHREADS);

            counter2 += 1;
            barrier.Wait();
            TEST_VERIFY(counter2 == NTHREADS);

            counter1 = 0;
            barrier.Wait();
            TEST_VERIFY(counter1 == 0);

            barrier.Wait();
        }

        std::call_once(onceFlag, &RunOnMainThreadAsync, [this]() { testComplete = true; });
    }
};
