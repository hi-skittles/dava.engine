#pragma once

#include "Base/BaseTypes.h"
#include "Concurrency/Thread.h"
#include "Debug/DVAssert.h"
#include "Debug/InterthreadBlockingCallMonitor.h"

// TODO: choose default deadlock checking policy
// 1. by default deadlock checking is disabled, DAVA_ENABLE_DEADLOCK_MONITOR must be explicilty defined to perform checks, or
// 2. by default deadlock checking is enabled, DAVA_DISABLE_DEADLOCK_MONITOR should be defined to disable checks
#if !defined(DAVA_DISABLE_DEADLOCK_MONITOR)

// How to use deadlock checking in interthread blocking calls
// 1. You should know identifier of thread that will process the call
// 2. Before performing blocking call to another thread place DAVA_BEGIN_BLOCKING_CALL with target thread id as parameter
// 3. If deadlock is detected assert is raised with call chain that has lead to a deadlock
// 4. After control has returned from blocking call place DAVA_END_BLOCKING_CALL with target thread id as parameter
//
// Example:
// Given thread A and thread B
// void thread_a()
// {
//     uint64 id_b = ...;               // somehow get id of thread B
//     DAVA_BEGIN_BLOCKING_CALL(id_b);  // here remember call chain from thread A to thread B
//                                      // if deadlock is detected then DAVA_BEGIN_BLOCKING_CALL will raise assert
//     blocking_call_to_thread_b();     // perform blocking call to thread B
//     DAVA_END_BLOCKING_CALL(id_b);    // when call forget call chain from thread A to thread B
// }
//
// void thread_b()
// {
//     uint64 id_a = ...;               // somehow get id of thread A
//     DAVA_BEGIN_BLOCKING_CALL(id_a);  // for now thread A has already made blocking call to thread B (this thread)
//                                      // and thread B tries to call back to thread A which is blocked
//                                      // so DAVA_BEGIN_BLOCKING_CALL will raise assert with call chain that lead to deadlock
//                                      //   thread_b --> thread_a --> thread_b
//     blocking_call_to_thread_a();     // perform blocking call back to thread A
//     DAVA_END_BLOCKING_CALL(id_a);
// }

#define DAVA_BEGIN_BLOCKING_CALL(targetThread) \
    do { \
        using namespace DAVA; \
        Vector<uint64> callChain; \
        Debug::InterthreadBlockingCallMonitor* monitor = Debug::InterthreadBlockingCallMonitor::Instance(); \
        bool deadlock = monitor->BeginBlockingCall(Thread::GetCurrentIdAsUInt64(), targetThread, callChain); \
        if (deadlock) \
        { \
            String s = monitor->CallChainToString(callChain); \
            DVASSERT(0, ("Deadlock callchain: " + s).c_str()); \
        } \
    } while (0)

#define DAVA_END_BLOCKING_CALL(targetThread) \
    do { \
        using namespace DAVA; \
        Debug::InterthreadBlockingCallMonitor::Instance()->EndBlockingCall(Thread::GetCurrentIdAsUInt64(), targetThread); \
    } while (0)

#else

#define DAVA_BEGIN_BLOCKING_CALL(targetThread)
#define DAVA_END_BLOCKING_CALL(targetThread)

#endif
