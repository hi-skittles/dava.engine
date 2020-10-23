#pragma once

#include "Concurrency/ConditionVariable.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
/**
 Class SyncBarrier is synchronization point between multiple threads.
 SyncBarrier is configured for a particular number of threads and each thread after reaching the barrier waits
 other threads to arrive. When the last thread reaches the barrier all threads can continue and the barrier resets.
 
 SyncBarrier is not copyable and not movable.
 
 SyncBarrier's interface is similar to interface of boost::barrier.
 
 \code
 SyncBarrier barrier(2);    // Barrier configired for two threads
 
 void thread1()
 {
    // do something useful
    barrier.Wait();     // wait until thread2 calls barrier.Wait()
    
    // barrier has been reset and threads can syncronize again
    barrier.Wait();
 }
 
 void thread2()
 {
    // do something important
    barrier.Wait();     // wait until thread1 calls barrier.Wait()
    
    // barrier has been reset and threads can syncronize again
    barrier.Wait();
 }
 
 \endcode
*/
class SyncBarrier final
{
public:
    /**
     Construct barrier for a number of threads.
     threadCount must be greater than zero.
    */
    SyncBarrier(size_t threadCount);

    SyncBarrier(const SyncBarrier&) = delete;
    SyncBarrier(SyncBarrier&&) = delete;

    /**
     Block until `threadCount` threads have called Wait on SyncBarrier instance.
     When threadCount-th thread calls Wait, the barrier is reset and all waiting threads are unblocked.
    */
    void Wait();

private:
    Mutex mutex;
    ConditionVariable cv;

    size_t threadCount = 0;
    size_t waitingThreads = 0;
    int generation = 0;
};

} // namespace DAVA
