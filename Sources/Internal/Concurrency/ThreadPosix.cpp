#include "Base/Platform.h"

#if defined(__DAVAENGINE_POSIX__)

#include <time.h>
#include <thread>

#include "Concurrency/PosixThreads.h"
#include "Concurrency/Thread.h"
#include "Logger/Logger.h"

#if defined(__DAVAENGINE_ANDROID__)
#include <sys/syscall.h>
#include <unistd.h>
#include "Engine/PlatformApiAndroid.h"
#elif defined(__DAVAENGINE_APPLE__)
#import <Foundation/NSAutoreleasePool.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#endif

namespace DAVA
{
// android have no way to kill a thread, so we will use signal to determine
// if we need to end thread
#if defined(__DAVAENGINE_ANDROID__)
void thread_exit_handler(int sig)
{
    if (SIGRTMIN == sig)
    {
        JNI::DetachCurrentThreadFromJVM();
        pthread_exit(0);
    }
}
#endif

void Thread::Init()
{
#if defined(__DAVAENGINE_ANDROID__)
    handle = 0;
    struct sigaction cancelThreadAction;
    Memset(&cancelThreadAction, 0, sizeof(cancelThreadAction));
    sigemptyset(&cancelThreadAction.sa_mask);
    cancelThreadAction.sa_flags = 0;
    cancelThreadAction.sa_handler = thread_exit_handler;
    sigaction(SIGRTMIN, &cancelThreadAction, nullptr);
#endif
}

void Thread::Shutdown()
{
    Join();
}

void Thread::KillNative()
{
    uint32 ret = 0;
#if defined(__DAVAENGINE_APPLE__)
    ret = pthread_cancel(handle);
#endif
#if defined(__DAVAENGINE_ANDROID__)
    ret = pthread_kill(handle, SIGRTMIN);
#endif
    if (0 != ret)
    {
        Logger::FrameworkDebug("[Thread::Cancel] cannot kill thread: id = %d, error = %d", Thread::GetCurrentId(), ret);
    }
}

void Thread::SetCurrentThreadName(const String& str)
{
#if defined(__DAVAENGINE_ANDROID__)
    pthread_setname_np(pthread_self(), str.c_str());
#elif defined(__DAVAENGINE_APPLE__)
    pthread_setname_np(str.c_str());
#endif
}

void* PthreadMain(void* param)
{
#if defined(__DAVAENGINE_APPLE__)
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif

#if defined(__DAVAENGINE_ANDROID__)
    JNI::AttachCurrentThreadToJVM();
#endif

    Thread* t = static_cast<Thread*>(param);
    Thread::SetCurrentThreadName(t->name);
#if defined(__DAVAENGINE_ANDROID__)
    t->system_handle = gettid();
#endif

    Thread::ThreadFunction(param);

#if defined(__DAVAENGINE_ANDROID__)
    JNI::DetachCurrentThreadFromJVM();
#endif

#if defined(__DAVAENGINE_APPLE__)
    [pool release];
#endif
    pthread_exit(0);
}

void Thread::Start()
{
    DVASSERT(STATE_CREATED == state);

    pthread_attr_t attr{};
    pthread_attr_init(&attr);
    if (stackSize != 0)
        pthread_attr_setstacksize(&attr, stackSize);

    int err = pthread_create(&handle, &attr, PthreadMain, this);
    if (0 == err)
    {
        isJoinable.Set(true);
        state.CompareAndSwap(STATE_CREATED, STATE_RUNNING);
    }
    else
    {
        Memset(&handle, 0, sizeof(handle));
        Logger::Error("Thread::Start failed to create thread: error=%d", err);
    }

    pthread_attr_destroy(&attr);
}

void Thread::Join()
{
    if (isJoinable.CompareAndSwap(true, false))
    {
        pthread_join(handle, nullptr);
    }
}

Thread::Id Thread::GetCurrentId()
{
    return pthread_self();
}

uint64 Thread::GetCurrentIdAsUInt64()
{
#if defined(__DAVAENGINE_APPLE__)
    return reinterpret_cast<uint64>(GetCurrentId());
#elif defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_LINUX__)
    return static_cast<uint64>(GetCurrentId());
#endif
}

#if defined(__DAVAENGINE_APPLE__)

bool BindToProcessorApple(pthread_t thread, unsigned proc_n)
{
    thread_affinity_policy_data_t policy_data = { int(proc_n) };
    thread_policy_t policy = reinterpret_cast<thread_policy_t>(&policy_data);
    thread_port_t mach_thread = pthread_mach_thread_np(thread);

    auto res = thread_policy_set(mach_thread,
                                 THREAD_AFFINITY_POLICY,
                                 policy, 1);
    return res == KERN_SUCCESS;
}

#elif defined(__DAVAENGINE_ANDROID__)

bool BindToProcessorAndroid(pid_t pid, unsigned proc_n)
{
    int mask = 1 << proc_n;
    int res = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
    return res == 0;
}

#endif

bool Thread::BindToProcessor(unsigned proc_n)
{
    DVASSERT(proc_n < std::thread::hardware_concurrency());
    if (proc_n >= std::thread::hardware_concurrency())
        return false;

#if defined(__DAVAENGINE_APPLE__)
    return BindToProcessorApple(handle, proc_n);
#elif defined(__DAVAENGINE_ANDROID__)
    return BindToProcessorAndroid(system_handle, proc_n);
#else

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(proc_n, &cpuset);

    int error = pthread_setaffinity_np(handle, sizeof(cpuset), &cpuset);
    return error == 0;

#endif
}

bool calculatePriority(int priority, int* sched_policy, int* sched_prio)
{
    const int lowestPrio = Thread::PRIORITY_LOW;
    const int highestPrio = Thread::PRIORITY_HIGH;

    int prio_min = sched_get_priority_min(*sched_policy);
    int prio_max = sched_get_priority_max(*sched_policy);

    if (prio_min == -1 || prio_max == -1)
        return false;

    int prio = ((priority - lowestPrio) * (prio_max - prio_min) / highestPrio) + prio_min;
    prio = std::max(prio_min, std::min(prio_max, prio));

    *sched_prio = prio;
    return true;
}

void Thread::SetPriority(eThreadPriority priority)
{
    DVASSERT(state == STATE_RUNNING);
    if (threadPriority == priority)
        return;

    threadPriority = priority;
    int sched_policy = 0;
    sched_param param{};

    if (pthread_getschedparam(handle, &sched_policy, &param) != 0)
    {
        Logger::FrameworkDebug("[Thread::SetPriority]: Cannot get schedule parameters");
        return;
    }

    int prio = 0;
    if (!calculatePriority(priority, &sched_policy, &prio))
    {
        Logger::FrameworkDebug("[Thread::SetPriority]: Cannot determine scheduler priority range");
        return;
    }

    param.sched_priority = prio;
    int status = pthread_setschedparam(handle, sched_policy, &param);

    if (status != 0)
    {
        Logger::FrameworkDebug("[Thread::SetPriority]: Cannot set schedule parameters");
        return;
    }
}

} //  namespace DAVA

#endif // __DAVAENGINE_POSIX__
