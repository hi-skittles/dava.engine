#pragma once

#include "Base/BaseTypes.h"
#include "Functional/Function.h"
#include "Concurrency/ConditionVariable.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Spinlock.h"

namespace DAVA
{
class JobQueueWorker
{
public:
    JobQueueWorker(uint32 maxCount = 1024);
    virtual ~JobQueueWorker();

    void Push(const Function<void()>& fn);
    bool PopAndExec();

    bool IsEmpty();

    void Signal();
    void Broadcast();
    void Wait();

protected:
    uint32 jobsMaxCount;
    Function<void()>* jobs;

    uint32 nextPushIndex;
    uint32 nextPopIndex;
    int32 processingCount;

    Spinlock lock;
    ConditionVariable jobsInQueueCV;
    Mutex jobsInQueueMutex;
};
}
