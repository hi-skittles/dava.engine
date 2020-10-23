#include "Base/Platform.h"
#if defined(__DAVAENGINE_WINDOWS__)

#include "Logger/Logger.h"

#include <thread>
#include "Concurrency/Thread.h"

namespace DAVA
{

#include <windows.h>
#include <process.h>

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

unsigned __stdcall ThreadFunc(void* param)
{
    Thread* t = static_cast<Thread*>(param);
    Thread::SetCurrentThreadName(t->name);

    Thread::ThreadFunction(param);
    return 0;
}

void Thread::Init()
{
}

void Thread::Shutdown()
{
    if (handle != nullptr)
    {
        Join();
        CloseHandle(handle);
        handle = nullptr;
    }
}

void Thread::Start()
{
    DVASSERT(STATE_CREATED == state);

    uintptr_t x = _beginthreadex(nullptr,
                                 static_cast<DWORD>(stackSize),
                                 &ThreadFunc,
                                 this,
                                 0,
                                 nullptr);

    if (x != 0)
    {
        handle = reinterpret_cast<HANDLE>(x);
        isJoinable.Set(true);
        state.CompareAndSwap(STATE_CREATED, STATE_RUNNING);
    }
    else
    {
        Logger::Error("Thread::Start failed to create thread: errno=%d", errno);
    }
}

void Thread::SetCurrentThreadName(const String& str)
{
    /*
        inside that ifdef we set thread name through raising speciefic exception.
        https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    */

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = str.c_str();
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<PULONG_PTR>(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}

void Thread::Join()
{
    if (isJoinable.CompareAndSwap(true, false))
    {
        if (WaitForSingleObjectEx(handle, INFINITE, FALSE) != WAIT_OBJECT_0)
        {
            DAVA::Logger::Error("Thread::Join failed in WaitForSingleObjectEx: error=%u", GetLastError());
        }
    }
}

void Thread::KillNative()
{
#if defined(__DAVAENGINE_WIN_UAP__)
    DAVA::Logger::Warning("Thread::KillNative() is not implemented for Windows Store platform");
#else
    if (TerminateThread(handle, 0))
    {
        CloseHandle(handle);
        handle = nullptr;
    }
#endif
}

Thread::Id Thread::GetCurrentId()
{
    return ::GetCurrentThreadId();
}

uint64 Thread::GetCurrentIdAsUInt64()
{
    return static_cast<uint64>(GetCurrentId());
}

bool Thread::BindToProcessor(unsigned proc_n)
{
    bool success = false;

#if defined(__DAVAENGINE_WIN_UAP__)
    PROCESSOR_NUMBER proc_number{};
    proc_number.Group = 0;
    proc_number.Number = proc_n;

    success = (::SetThreadIdealProcessorEx(handle, &proc_number, nullptr) == TRUE);
#else
    DWORD_PTR mask = DWORD_PTR(1) << proc_n;
    success = (::SetThreadAffinityMask(handle, mask) == 0);
#endif

    if (!success)
    {
        Logger::FrameworkDebug("Failed bind thread to processor %d, error %d", proc_n, GetLastError());
    }

    return success;
}

void Thread::SetPriority(eThreadPriority priority)
{
    DVASSERT(state == STATE_RUNNING);
    if (threadPriority == priority)
        return;

    threadPriority = priority;
    int prio = THREAD_PRIORITY_NORMAL;
    switch (threadPriority)
    {
    case PRIORITY_LOW:
        prio = THREAD_PRIORITY_LOWEST;
        break;
    case PRIORITY_HIGH:
        prio = THREAD_PRIORITY_HIGHEST;
        break;
    default:
        break;
    }

    if (::SetThreadPriority(handle, prio) == 0)
    {
        Logger::FrameworkDebug("[Thread::SetPriority]: Cannot set thread priority");
    }
}
}

#endif
