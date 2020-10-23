#pragma once

#include "Base/Platform.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/UniqueLock.h"

#ifdef USE_CPP11_CONCURRENCY
#include <condition_variable> //for std::condition_variable
#include "Debug/DVAssert.h"
#else
#include "Concurrency/PosixThreads.h"
#endif

namespace DAVA
{
//-------------------------------------------------------------------------------------------------
//Condition variable class
//-------------------------------------------------------------------------------------------------
class ConditionVariable
{
public:
    ConditionVariable();
    ~ConditionVariable() DAVA_NOEXCEPT;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    template <typename Predicate>
    void Wait(UniqueLock<Mutex>& guard, Predicate pred);
    void Wait(UniqueLock<Mutex>& guard);

    //mutex must be locked
    template <typename Predicate>
    void Wait(Mutex& mutex, Predicate pred);
    void Wait(Mutex& mutex);

    void NotifyOne();
    void NotifyAll();

private:

#ifdef USE_CPP11_CONCURRENCY
    std::condition_variable cv;
#else
    pthread_cond_t cv;
#endif
};

template <typename Predicate>
void ConditionVariable::Wait(UniqueLock<Mutex>& guard, Predicate pred)
{
    while (!pred())
    {
        Wait(guard);
    }
}

template <typename Predicate>
void ConditionVariable::Wait(Mutex& mutex, Predicate pred)
{
    UniqueLock<Mutex> lock(mutex, AdoptLock());
    Wait(lock, pred);
    lock.Release();
}

inline void ConditionVariable::Wait(Mutex& mutex)
{
    UniqueLock<Mutex> lock(mutex, AdoptLock());
    Wait(lock);
    lock.Release();
}

#ifdef USE_CPP11_CONCURRENCY

//-------------------------------------------------------------------------------------------------
//Condition variable realization using std::condition_variable
//-------------------------------------------------------------------------------------------------
inline ConditionVariable::ConditionVariable()
{
}
inline ConditionVariable::~ConditionVariable() DAVA_NOEXCEPT
{
}

inline void ConditionVariable::Wait(UniqueLock<Mutex>& guard)
{
    DVASSERT(guard.OwnsLock(), "Mutex must be locked and UniqueLock must own it");

    std::unique_lock<std::mutex> lock(guard.GetMutex()->mutex, std::adopt_lock_t());
    cv.wait(lock);
    lock.release();
}

inline void ConditionVariable::NotifyOne()
{
    cv.notify_one();
}

inline void ConditionVariable::NotifyAll()
{
    cv.notify_all();
}

#endif //  USE_CPP11_CONCURRENCY

} //  namespace DAVA
