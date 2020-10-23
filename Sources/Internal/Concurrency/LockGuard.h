#pragma once

namespace DAVA
{
//-----------------------------------------------------------------------------
//LockGuard class - RAII wrapper for mutex object
//-----------------------------------------------------------------------------
template <class MutexT>
class LockGuard
{
    using MutexType = MutexT;

public:
    // construct and lock
    explicit LockGuard(MutexType& mutex);

    // clean up
    ~LockGuard();

    // no copy construct and assign operator
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    MutexType& mutex_ref;
};

//-----------------------------------------------------------------------------
//Realization of LockGuard
//-----------------------------------------------------------------------------
template <class MutexT>
LockGuard<MutexT>::LockGuard(MutexType& mutex)
    : mutex_ref(mutex)
{
    mutex_ref.Lock();
}

template <class MutexT>
LockGuard<MutexT>::~LockGuard()
{
    mutex_ref.Unlock();
}

} //  namespace DAVA
