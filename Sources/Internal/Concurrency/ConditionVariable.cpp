#include "Base/Platform.h"
#ifndef USE_CPP11_CONCURRENCY

#include "Logger/Logger.h"
#include "Concurrency/ConditionVariable.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
//-------------------------------------------------------------------------------------------------
//Condition variable realization using POSIX Threads API
//-------------------------------------------------------------------------------------------------
ConditionVariable::ConditionVariable()
{
    int ret = pthread_cond_init(&cv, nullptr);
    if (ret != 0)
    {
        Logger::Error("ConditionVariable::ConditionVariable() error: %d", ret);
    }
}

ConditionVariable::~ConditionVariable() DAVA_NOEXCEPT
{
    int ret = pthread_cond_destroy(&cv);
    if (ret != 0)
    {
        Logger::Error("ConditionVariable::~ConditionVariable() error: %d", ret);
    }
}

void ConditionVariable::Wait(UniqueLock<Mutex>& guard)
{
    pthread_mutex_t* mutex = &guard.GetMutex()->mutex;
    int ret = pthread_cond_wait(&cv, mutex);

    if (ret != 0)
    {
        Logger::Error("ConditionVariable::Wait() error: %d", ret);
    }
}

void ConditionVariable::NotifyOne()
{
    int ret = pthread_cond_signal(&cv);
    if (ret != 0)
    {
        Logger::Error("ConditionVariable::NotifyOne() error: %d", ret);
    }
}

void ConditionVariable::NotifyAll()
{
    int ret = pthread_cond_broadcast(&cv);
    if (ret != 0)
    {
        Logger::Error("ConditionVariable::NotifyAll() error: %d", ret);
    }
}

} //  namespace DAVA

#endif //  !USE_CPP11_CONCURRENCY