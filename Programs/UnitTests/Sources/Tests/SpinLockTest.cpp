#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

namespace
{
uint32 sharedCounter{ 0 };
const uint32 result{ 20000000 };
const uint32 numThreads{ 5 };
Spinlock spin;

void ThreadFunc(DAVA::BaseObject* obj, void*, void*)
{
    uint32 count{ 0 };
    while (count < (result / numThreads))
    {
        spin.Lock();
        ++sharedCounter;
        spin.Unlock();
        ++count;
    }
}
}

DAVA_TESTCLASS (SpinLockTest)
{
    DAVA_TEST (TestFunc)
    {
        static_assert(result % numThreads == 0, "numThreads equal for each thread?");

        List<Thread*> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.push_back(Thread::Create(Message(ThreadFunc)));
        }

        for (auto thread : threads)
        {
            thread->Start();
        }

        for (auto thread : threads)
        {
            thread->Join();
            SafeRelease(thread);
        }

        TEST_VERIFY(result == sharedCounter);
    }
}
;
