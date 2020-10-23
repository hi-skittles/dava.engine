#pragma once

#include "Semaphore.h"
#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include <atomic>

namespace DAVA
{
/*! 
    \brief SemaphoreLite
    Lite semaphore that do partial spinning before going into real semaphore.
*/
class SemaphoreLite final
{
public:
    static const uint32 defaultSpinCount = 4000;

    SemaphoreLite(uint32 semCount_, uint32 spinCount_ = defaultSpinCount)
        : spinCount(spinCount_)
    {
        int32 c = static_cast<int32>(semCount_);
        DVASSERT(c >= 0);

        waitCount.store(c);
    }

    inline void Post(uint32 count = 1)
    {
        int32 c = static_cast<int32>(count);
        DVASSERT(c > 0);

        int32 oldWaitCount = waitCount.fetch_add(c, std::memory_order_release);
        int32 toReleaseCount = std::min(-oldWaitCount, static_cast<int32>(count));
        if (toReleaseCount > 0)
        {
            sem.Post(toReleaseCount);
        }
    }

    inline void Wait()
    {
        // try to spin and wait for count > 0
        uint32 spin = spinCount;
        while (spin--)
        {
            if (TryWait())
                return;

            // prevent compiler from instruction reordering
            std::atomic_signal_fence(std::memory_order_acquire);
        }

        // we failed waiting for (count > 0) while spinning,
        // so now we should wait in kernel semaphore
        int32 oldWaitCount = waitCount.fetch_sub(1, std::memory_order_acquire);
        if (oldWaitCount <= 0)
        {
            sem.Wait();
        }
    }

    inline bool TryWait()
    {
        // check if count is > 0 and if so try to decrement it
        // return true if decrement was successful
        int32 oldWaitCount = waitCount.load(std::memory_order_relaxed);
        return (oldWaitCount > 0 && waitCount.compare_exchange_strong(oldWaitCount, oldWaitCount - 1, std::memory_order_acquire));
    }

private:
    Semaphore sem;
    uint32 spinCount;
    std::atomic<int32> waitCount;
};

} // namespace DAVA
