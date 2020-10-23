#include "JobThread.h"

namespace DAVA
{
JobThread::JobThread(JobQueueWorker* _workerQueue, Semaphore* _workerDoneSem)
    : workerQueue(_workerQueue)
    , workerDoneSem(_workerDoneSem)
    , threadCancel(false)
    , threadFinished(false)
{
    thread = Thread::Create(MakeFunction(this, &JobThread::ThreadFunc));
    thread->SetName("DAVA::JobThread");
    thread->Start();
}

JobThread::~JobThread()
{
    // cancel thread
    threadCancel = true;
    while (!threadFinished)
    {
        workerQueue->Broadcast();
        Thread::Sleep(10); // sleep 10 ms until other check
    }

    // join and release thread
    thread->Join();
    SafeRelease(thread);
}

void JobThread::ThreadFunc()
{
    while (!threadCancel)
    {
        workerQueue->Wait();

        while (workerQueue->PopAndExec())
        {
        }

        workerDoneSem->Post();
    }

    threadFinished = true;
}

};
;