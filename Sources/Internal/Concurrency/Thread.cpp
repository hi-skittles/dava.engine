#include <thread>
#include "Concurrency/Thread.h"
#include "Concurrency/LockGuard.h"
#include "Logger/Logger.h"

#ifndef __DAVAENGINE_WINDOWS__
#include <time.h>
#include <errno.h>
#endif

namespace DAVA
{
Thread::Id Thread::mainThreadId;
const char Thread::davaMainThreadName[] = "DAVA Engine Main Thread";

ConcurrentObject<Set<Thread*>>& GetThreadList()
{
    static ConcurrentObject<Set<Thread*>> threadList;
    return threadList;
}

void Thread::InitMainThread()
{
    mainThreadId = GetCurrentId();
    Thread::SetCurrentThreadName(davaMainThreadName);
}

bool Thread::IsMainThread()
{
    if (Thread::Id() == mainThreadId)
    {
        Logger::Error("Main thread not initialized");
    }

    Id currentId = GetCurrentId();
    return currentId == mainThreadId;
}

Thread* Thread::Current()
{
    const Id currentId = GetCurrentId();

    auto threadListAccessor = GetThreadList().GetAccessor();
    for (Thread* t : *threadListAccessor)
    {
        if (t->GetId() == currentId)
        {
            return t;
        }
    }

    DVASSERT(false, "Couldn't get current thread");

    return nullptr;
}

Thread* Thread::Create(const Message& msg)
{
    return new Thread(msg);
}

Thread* Thread::Create(const Procedure& proc)
{
    return new Thread(proc);
}

void Thread::Kill()
{
    // it is possible to kill thread just after creating or starting and the problem is - thred changes state
    // to STATE_RUNNING insite threaded function - so that could not happens in that case. Need some time.
    DVASSERT(STATE_CREATED != state);

    // Important - DO NOT try to wait RUNNING state because that state wll not appear if thread is not started!!!
    // You can wait RUNNING state, but not from thred which should call Start() for created Thread.

    if (STATE_RUNNING == state)
    {
        KillNative();
        state = STATE_KILLED;
        Release();
    }
}

void Thread::KillAll()
{
    auto threadListAccessor = GetThreadList().GetAccessor();
    for (auto& x : *threadListAccessor)
    {
        x->Kill();
    }
}

void Thread::CancelAll()
{
    auto threadListAccessor = GetThreadList().GetAccessor();
    for (auto& x : *threadListAccessor)
    {
        x->Cancel();
    }
}

Thread::Thread()
    : state(STATE_CREATED)
    , isCancelling(false)
    , stackSize(0)
    , handle(Handle())
    , id(Id())
    , name("DAVA::Thread")
{
    Init();

    auto threadListAccessor = GetThreadList().GetAccessor();
    threadListAccessor->insert(this);
}

Thread::Thread(const Message& msg)
    : Thread()
{
    Message message = msg;
    Thread* caller = this;
    threadFunc = [=] { message(caller); };
}

Thread::Thread(const Procedure& proc)
    : Thread()
{
    threadFunc = proc;
}

Thread::~Thread()
{
    Cancel();
    Shutdown();

    auto threadListAccessor = GetThreadList().GetAccessor();
    threadListAccessor->erase(this);
}

void Thread::ThreadFunction(void* param)
{
    Thread* t = reinterpret_cast<Thread*>(param);
    t->id = GetCurrentId();

    t->threadFunc();

    // Zero id to mark thread as finished in thread list obtained through GetThreadList() function.
    // This prevents from retrieving invalid Thread instance through Thread::Current()
    // as system can reuse thread ids.
    std::memset(&t->id, 0, sizeof(t->id));
    t->state = STATE_ENDED;
}

void Thread::Yield()
{
    std::this_thread::yield();
}

void Thread::Sleep(uint32 timeMS)
{
    std::chrono::milliseconds ms(timeMS);
    std::this_thread::sleep_for(ms);
}
};
