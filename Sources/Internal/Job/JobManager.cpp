#include "Job/JobManager.h"

#include "Debug/DVAssert.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/UniqueLock.h"
#include "Job/JobThread.h"
#include "Platform/DeviceInfo.h"

namespace DAVA
{
JobManager::JobManager(Engine* e)
    : engine(e)
    , mainJobIDCounter(1)
    , mainJobLastExecutedID(0)
    , workerDoneSem(0)
{
    uint32 cpuCoresCount = DeviceInfo::GetCpuCount();
    workerThreads.reserve(cpuCoresCount);

    for (uint32 i = 0; i < cpuCoresCount; ++i)
    {
        JobThread* thread = new JobThread(&workerQueue, &workerDoneSem);
        workerThreads.push_back(thread);
    }

    e->update.Connect(this, &JobManager::Update);
}

JobManager::~JobManager()
{
    engine->update.Disconnect(this);

    {
        LockGuard<Mutex> guard(mainQueueMutex);
        mainJobs.clear();
    }
    mainJobLastExecutedID = mainJobIDCounter;
    mainJobIDCounter = 0;
    mainCV.NotifyAll();

    for (uint32 i = 0; i < workerThreads.size(); ++i)
    {
        SafeDelete(workerThreads[i]);
    }

    workerThreads.clear();
}

void JobManager::Update(float32 /*frameDelta*/)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::JOB_MANAGER);

    bool hasFinishedJobs = false;

    mainQueueMutex.Lock();
    if (!mainJobs.empty())
    {
        // extract all jobs from queue
        while (!mainJobs.empty())
        {
            curMainJob = mainJobs.front();
            mainJobs.pop_front();

            if (curMainJob.type == JOB_MAINBG)
            {
                // TODO:
                // need implementation
                // be careful with job ID, because waiting depends on id order
                // ...

                DVASSERT(false);
            }

            if (curMainJob.invokerThreadId != Thread::Id() && curMainJob.fn != nullptr)
            {
                // unlock queue mutex until function execution finished
                mainQueueMutex.Unlock();
                curMainJob.fn();
                mainJobLastExecutedID = curMainJob.id;
                mainQueueMutex.Lock();
            }

            curMainJob = MainJob();
        }

        hasFinishedJobs = true;
    }
    mainQueueMutex.Unlock();

    // signal that jobs are finished
    if (hasFinishedJobs)
    {
        LockGuard<Mutex> cvguard(mainCVMutex);
        mainCV.NotifyAll();
    }
}

uint32 JobManager::GetWorkersCount() const
{
    return static_cast<uint32>(workerThreads.size());
}

uint32 JobManager::CreateMainJob(const Function<void()>& fn, eMainJobType mainJobType)
{
    uint32 jobID = 0;

    // if we are already in main thread and requested job shouldn't executed lazy
    // perform that job immediately
    if (Thread::IsMainThread() && mainJobType != JOB_MAINLAZY)
    {
        fn();
    }
    else if (mainJobIDCounter > 0)
    {
        // reserve job ID
        jobID = ++mainJobIDCounter;

        // push requested job into queue
        MainJob job;
        job.fn = fn;
        job.invokerThreadId = Thread::GetCurrentId();
        job.type = mainJobType;
        job.id = jobID;

        {
            LockGuard<Mutex> guard(mainQueueMutex);
            mainJobs.push_back(job);
        }
    }

    return jobID;
}

void JobManager::WaitMainJobs(Thread::Id invokerThreadId /* = 0 */)
{
    if (Thread::IsMainThread())
    {
        // if wait was invoked from main-thread
        // and there are some jobs user is waiting for
        // we should immediately execute them
        if (HasMainJobs(invokerThreadId))
        {
            // just run update, it will execute all of main-thread jobs
            Update();

            // assert is something goes wrong
            DVASSERT(!HasMainJobs() && "Job exepected to be executed at this point, but seems it is still in queue");
        }
    }
    else
    {
        // If main thread is locked by WaitWorkerJobs this instruction will unlock
        // main thread, allowing it to perform all scheduled main-thread jobs
        workerDoneSem.Post();

        // Now check if there are some jobs in the queue and wait for them
        UniqueLock<Mutex> lock(mainCVMutex);
        while (HasMainJobs(invokerThreadId))
        {
            mainCV.Wait(lock);
        }
    }
}

void JobManager::WaitMainJobID(uint32 mainJobID)
{
    if (Thread::IsMainThread())
    {
        // if wait was invoked from main-thread
        // and there are some jobs user is waiting for
        // we should immediately execute them
        if (HasMainJobID(mainJobID))
        {
            // just run update, it will execute all of main-thread jobs
            Update();

            // assert is something goes wrong
            DVASSERT(!HasMainJobID(mainJobID) && "Job exepected to be executed at this point, but seems it is still in queue");
        }
    }
    else
    {
        // If main thread is locked by WaitWorkerJobs this instruction will unlock
        // main thread, allowing it to perform all scheduled main-thread jobs
        workerDoneSem.Post();

        // Now check if there are some jobs in the queue and wait for them
        UniqueLock<Mutex> lock(mainCVMutex);
        while (HasMainJobID(mainJobID))
        {
            mainCV.Wait(lock);
        }
    }
}

bool JobManager::HasMainJobs(Thread::Id invokerThreadId /* = 0 */)
{
    bool ret = false;

    // tread id = 0 as current thread id, so we should get it
    if (Thread::Id() == invokerThreadId)
    {
        invokerThreadId = Thread::GetCurrentId();
    }

    {
        LockGuard<Mutex> guard(mainQueueMutex);
        if (curMainJob.invokerThreadId == invokerThreadId)
        {
            ret = true;
        }
        else
        {
            Deque<MainJob>::const_iterator i = mainJobs.begin();
            Deque<MainJob>::const_iterator end = mainJobs.end();
            for (; i != end; ++i)
            {
                if (i->invokerThreadId == invokerThreadId)
                {
                    ret = true;
                    break;
                }
            }
        }
    }

    return ret;
}

bool JobManager::HasMainJobID(uint32 mainJobID)
{
    return (mainJobID > mainJobLastExecutedID);
}

void JobManager::CreateWorkerJob(const Function<void()>& fn)
{
    workerQueue.Push(fn);
    workerQueue.Signal();
}

void JobManager::WaitWorkerJobs()
{
    while (HasWorkerJobs())
    {
        if (Thread::IsMainThread())
        {
            // We want to be able to wait worker jobs, but at the same time
            // allow any worker job execute main job. Potentially this will cause
            // dead lock, but there is a simple solution:
            //
            // Every time, worker job is trying to execute WaitMainJobs it will
            // post workerDoneSem semaphore, that will give a chance to execute main jobs
            // in the following Update() call
            //
            Update();
        }

        workerDoneSem.Wait();
    }
}

bool JobManager::HasWorkerJobs()
{
    return !workerQueue.IsEmpty();
}
}
