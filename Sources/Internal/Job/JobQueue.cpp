#include "Job/JobQueue.h"
#include "Job/JobManager.h"
#include "Concurrency/LockGuard.h"

namespace DAVA
{
JobQueueWorker::JobQueueWorker(uint32 maxCount /* = 1024 */)
    : jobsMaxCount(maxCount)
    , nextPushIndex(0)
    , nextPopIndex(0)
    , processingCount(0)
{
    jobs = new Function<void()>[jobsMaxCount];
}

JobQueueWorker::~JobQueueWorker()
{
    SafeDeleteArray(jobs);
}

void JobQueueWorker::Push(const Function<void()>& fn)
{
    if (fn != nullptr)
    {
        LockGuard<Spinlock> guard(lock);
        if (nextPushIndex == nextPopIndex && 0 == processingCount)
        {
            nextPushIndex = 0;
            nextPopIndex = 0;
        }

        DVASSERT(nextPushIndex < jobsMaxCount);

        jobs[nextPushIndex++] = fn;
        processingCount++;
    }
}

bool JobQueueWorker::PopAndExec()
{
    bool ret = false;
    Function<void()> fn;

    {
        LockGuard<Spinlock> guard(lock);
        if (nextPopIndex < nextPushIndex)
        {
            fn = std::move(jobs[nextPopIndex++]);
        }
    }

    if (fn != nullptr)
    {
        fn();

        {
            LockGuard<Spinlock> guard(lock);
            DVASSERT(processingCount > 0);
            processingCount--;
        }

        ret = true;
    }

    return ret;
}

bool JobQueueWorker::IsEmpty()
{
    LockGuard<Spinlock> guard(lock);
    return (nextPopIndex == nextPushIndex && 0 == processingCount);
}

void JobQueueWorker::Signal()
{
    LockGuard<Mutex> guard(jobsInQueueMutex);
    jobsInQueueCV.NotifyOne();
}

void JobQueueWorker::Broadcast()
{
    LockGuard<Mutex> guard(jobsInQueueMutex);
    jobsInQueueCV.NotifyAll();
}

void JobQueueWorker::Wait()
{
    UniqueLock<Mutex> lock(jobsInQueueMutex);
    jobsInQueueCV.Wait(lock);
}
}
