#include "Concurrency/SyncBarrier.h"
#include "Concurrency/UniqueLock.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
SyncBarrier::SyncBarrier(size_t threadCount)
    : threadCount(threadCount)
{
    DVASSERT(threadCount > 0);
}

void SyncBarrier::Wait()
{
    UniqueLock<Mutex> lock(mutex);
    int curGeneration = generation;

    waitingThreads += 1;
    if (waitingThreads == threadCount)
    {
        generation += 1;
        waitingThreads = 0;
        lock.Unlock();
        cv.NotifyAll();
        return;
    }

    // If generation has changed then last thread has reached the barrier
    // so unblock current thread.
    while (curGeneration == generation)
    {
        cv.Wait(lock);
    }
}

} // namespace DAVA
