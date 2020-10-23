#pragma once

#include "Base/Platform.h"

#if defined(USE_CPP11_CONCURRENCY)
#include <mutex> //for std::mutex and std::recursive_mutex
#else
#include "Concurrency/PosixThreads.h"
#endif

namespace DAVA
{
//-----------------------------------------------------------------------------
//Mutex realization
//Direct using of mutex is inadvisable.
//Use LockGuard or ConcurrentObject instead
//-----------------------------------------------------------------------------
#if defined(USE_CPP11_CONCURRENCY)

template <typename MutexT>
class MutexBase
{
    friend class ConditionVariable;

public:
    MutexBase() = default;
    MutexBase(const MutexBase&) = delete;
    MutexBase& operator=(const MutexBase&) = delete;

    void Lock()
    {
        mutex.lock();
    }
    void Unlock()
    {
        mutex.unlock();
    }
    bool TryLock()
    {
        return mutex.try_lock();
    }

private:
    MutexT mutex;
};

class Mutex final : public MutexBase<std::mutex>
{
public:
    Mutex() = default;
};

class RecursiveMutex final : public MutexBase<std::recursive_mutex>
{
public:
    RecursiveMutex() = default;
};

#else

//Base mutex class
class MutexBase
{
    friend class ConditionVariable;

public:
    MutexBase() = default;
    ~MutexBase();

    MutexBase(const MutexBase&) = delete;
    MutexBase& operator=(const MutexBase&) = delete;

    void Lock();
    void Unlock();
    bool TryLock();

protected:
    pthread_mutex_t mutex;
};

//Specialized mutexes
class Mutex final : public MutexBase
{
public:
    Mutex();
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
};

class RecursiveMutex final : public MutexBase
{
public:
    RecursiveMutex();
    RecursiveMutex(const RecursiveMutex&) = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;
};

#endif // defined(USE_CPP11_CONCURRENCY)
};
