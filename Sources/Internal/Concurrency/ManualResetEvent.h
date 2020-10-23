#pragma once

#include "SemaphoreLite.h"
#include <atomic>

namespace DAVA
{
/*!
    \brief ManualResetEvent
    Notifies one or more waiting threads that an event has occurred.

    When a thread begins an activity that must complete before other threads proceed, it calls Reset
    to put ManualResetEvent in the non-signaled state. This thread can be thought of as controlling 
    the ManualResetEvent. Threads that call Wait() on the ManualResetEvent will block, awaiting the 
    signal. When the controlling thread completes the activity, it calls Signal() to signal that the
    waiting threads can proceed. All waiting threads are released.

    Once it has been signaled, ManualResetEvent remains signaled until it is manually reset. That is,
    calls to Wait() return immediately.
*/
class ManualResetEvent final
{
public:
    ManualResetEvent(bool isSignaled = true, uint32 spinCount_ = SemaphoreLite::defaultSpinCount)
        : sem(0, spinCount_)
    {
        status.store(isSignaled ? 1 : 0, std::memory_order_release);
    }

    void Signal()
    {
        int oldStatus = status.exchange(1, std::memory_order_release);
        if (oldStatus < 0)
        {
            sem.Post(-oldStatus);
        }
    }

    void Reset()
    {
        int curValue = 1;
        status.compare_exchange_weak(curValue, 0, std::memory_order_release, std::memory_order_acquire);
    }

    void Wait()
    {
        int curStatus = status.load(std::memory_order_relaxed);
        while (curStatus < 1)
        {
            // status is in reset state, so we should
            // subtract 1 from it and sleep on semaphore
            int newStatus = curStatus - 1;

            // try to compare and swap old value (curStatus) with new one (newStatus)
            // if value in status changed so that it isn't equal to the curStatus
            // CAS operation will fail and curStatus variable will be renewed
            if (status.compare_exchange_weak(curStatus, newStatus, std::memory_order_release, std::memory_order_relaxed))
            {
                sem.Wait();
                break;
            }
        }
    }

private:
    // status == 1: is signaled
    // status == 0: is reset and no threads are waiting
    // status == -N: is reset and N threads are waiting
    std::atomic<int> status;
    DAVA::SemaphoreLite sem;
};

} // namespace DAVA
