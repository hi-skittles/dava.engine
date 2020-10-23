#pragma once

#include "Concurrency/Atomic.h"

namespace DAVA
{
//-----------------------------------------------------------------------------
//Spinlock class - non-blocking sync primitive
//Direct using of spinlock is inadvisable.
//Use LockGuard or ConcurrentObject instead
//-----------------------------------------------------------------------------
class Spinlock
{
public:
    void Lock();
    bool TryLock();
    void Unlock();

private:
    Atomic<size_t> flag;
};

//-----------------------------------------------------------------------------
//Realization
//-----------------------------------------------------------------------------
inline void Spinlock::Lock()
{
    while (!flag.CompareAndSwap(0, 1))
    {
        while (0 != flag.GetRelaxed())
        {
        }
    }
}

inline bool Spinlock::TryLock()
{
    return flag.CompareAndSwap(0, 1);
}

inline void Spinlock::Unlock()
{
    flag.Set(0);
}
}
