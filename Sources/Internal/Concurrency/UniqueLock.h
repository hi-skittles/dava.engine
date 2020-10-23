#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
// indicates adopt lock
struct AdoptLock
{
};
// indicates defer lock
struct DeferLock
{
};
// indicates try to lock
struct TryToLock
{
};

//-----------------------------------------------------------------------------
//UniqueLock class - wrapper for mutex object
//-----------------------------------------------------------------------------
template <class MutexT>
class UniqueLock
{
    using MutexType = MutexT;

public:
    // default construct
    UniqueLock() DAVA_NOEXCEPT;

    // construct and lock
    explicit UniqueLock(MutexType& mutex);

    // construct and assume already locked
    UniqueLock(MutexType& mutex, AdoptLock);

    // construct but don't lock
    UniqueLock(MutexType& mutex, DeferLock) DAVA_NOEXCEPT;

    // construct and try to lock
    UniqueLock(MutexType& mutex, TryToLock);

    // destructive copy
    UniqueLock(UniqueLock&& other) DAVA_NOEXCEPT;

    // destructive copy
    UniqueLock& operator=(UniqueLock&& other) DAVA_NOEXCEPT;

    // clean up
    ~UniqueLock() DAVA_NOEXCEPT;

    // no copy construct and assign operator
    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;

    // lock the mutex
    void Lock();

    // try to lock the mutex
    bool TryLock() DAVA_NOEXCEPT;

    // unlock the mutex
    void Unlock();

    // return true if this object owns the lock
    bool OwnsLock() const DAVA_NOEXCEPT;

    // return pointer to the mutex
    MutexType* GetMutex() const DAVA_NOEXCEPT;

    // release the mutex
    MutexType* Release() DAVA_NOEXCEPT;

private:
    MutexType* mutex_ptr;
    bool owns;
};

//-----------------------------------------------------------------------------
//Native realization of LockGuard
//-----------------------------------------------------------------------------
template <class MutexT>
UniqueLock<MutexT>::UniqueLock() DAVA_NOEXCEPT
: mutex_ptr(nullptr),
  owns(false)
{
}

template <class MutexT>
UniqueLock<MutexT>::UniqueLock(MutexType& mutex)
    : mutex_ptr(&mutex)
    , owns(false)
{
    mutex_ptr->Lock();
    owns = true;
}

template <class MutexT>
UniqueLock<MutexT>::UniqueLock(MutexType& mutex, AdoptLock)
    : mutex_ptr(&mutex)
    , owns(true)
{
}

template <class MutexT>
UniqueLock<MutexT>::UniqueLock(MutexType& mutex, DeferLock) DAVA_NOEXCEPT
: mutex_ptr(&mutex),
  owns(false)
{
}

template <class MutexT>
UniqueLock<MutexT>::UniqueLock(MutexType& mutex, TryToLock)
    : mutex_ptr(&mutex)
    , owns(mutex_ptr->TryLock())
{
}

template <class MutexT>
UniqueLock<MutexT>::UniqueLock(UniqueLock&& other) DAVA_NOEXCEPT
: mutex_ptr(other.mutex_ptr),
  owns(other.owns)
{
    other.mutex_ptr = 0;
    other.owns = false;
}

template <class MutexT>
UniqueLock<MutexT>& UniqueLock<MutexT>::operator=(UniqueLock&& other) DAVA_NOEXCEPT
{
    if (this != &other)
    {
        if (owns)
            mutex_ptr->Unlock();
        mutex_ptr = other.mutex_ptr;
        owns = other.owns;
        other.mutex_ptr = 0;
        other.owns = false;
    }
    return (*this);
}

template <class MutexT>
UniqueLock<MutexT>::~UniqueLock() DAVA_NOEXCEPT
{
    if (owns)
        mutex_ptr->Unlock();
}

template <class MutexT>
void UniqueLock<MutexT>::Lock()
{
    mutex_ptr->Lock();
    owns = true;
}

template <class MutexT>
bool UniqueLock<MutexT>::TryLock() DAVA_NOEXCEPT
{
    owns = mutex_ptr->TryLock();
    return owns;
}

template <class MutexT>
void UniqueLock<MutexT>::Unlock()
{
    mutex_ptr->Unlock();
    owns = false;
}

template <class MutexT>
bool UniqueLock<MutexT>::OwnsLock() const DAVA_NOEXCEPT
{
    return owns;
}

template <class MutexT>
typename UniqueLock<MutexT>::MutexType* UniqueLock<MutexT>::GetMutex() const DAVA_NOEXCEPT
{
    return mutex_ptr;
}

template <class MutexT>
typename UniqueLock<MutexT>::MutexType* UniqueLock<MutexT>::Release() DAVA_NOEXCEPT
{
    MutexType* mtx = mutex_ptr;
    mutex_ptr = nullptr;
    owns = false;
    return mtx;
}

} //  namespace DAVA
