#pragma once

#include "Base/BaseTypes.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/AutoResetEvent.h"
#include "Concurrency/Thread.h"
#include "Functional/Function.h"

#include "Debug/DeadlockMonitor.h"

#include <atomic>

namespace DAVA
{
/**
    \ingroup engine
    Template class that implements event dispatcher where template parameter T specifies event type.
    T must be copy or move constructible.

    Dispatcher manages event queue and provides methods to place events to queue and extract events from queue.
    Application can place events from any thread, but extraction **must** be performed only from single thread
    which should stay the same until dispatcher dies. Event extraction thread is set by `LinkToCurrentThread` method.
*/
template <typename T>
class Dispatcher final
{
public:
    /**
        Enum that specifies how to execute event when SendEvent is invoked from the same thread as dispatcher linked thread
    */
    enum class eSendPolicy
    {
        QUEUED_EXECUTION = 0, //<! Default behavior: firstly execute all events in queue and then sent event
        IMMEDIATE_EXECUTION = 1, //<! Immediately execute sent event, do not execute events in queue
    };

    /**
        Dispatcher constructor

        \param handler Function object which will be invoked for each event when application calls `ProcessEvents`
        \param trigger Function object to trigger (initiate) blocking call
    */
    Dispatcher(const Function<void(const T&)>& handler, const Function<void()> trigger = []() {});
    ~Dispatcher() = default;

    // Explicitly delete copy and move assignment only for msvc 2013
    // as it does not fully conform c++11
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    Dispatcher(Dispatcher&&) = delete;
    Dispatcher& operator=(Dispatcher&&) = delete;

    /** Set thread where events are processed by application */
    void LinkToCurrentThread();
    uint64 GetLinkedThread() const;

    /** Check whether dispatcher has any events */
    bool HasEvents() const;

    /**
        Place event into queue and immediately return (do not wait event procession).
    */
    template <typename U>
    void PostEvent(U&& e);

    /**
        Place event into queue and wait until event is processed.
    */
    template <typename U>
    void SendEvent(U&& e, eSendPolicy policy = eSendPolicy::QUEUED_EXECUTION);

    /**
        Process events that are currently in queue. For each event in queue dispatcher
        invokes `handler` set in dispatcher constructor.
        Nested calls are not allowed.
    */
    void ProcessEvents();

    /**
        Examine event queue, for each event `viewer` is invoked.
    */
    void ViewEventQueue(const Function<void(const T&)>& viewer);

private:
    mutable Mutex mutex; // Mutex to guard event queue
    Vector<T> eventQueue;
    Vector<T> readyEvents;
    Function<void(const T&)> eventHandler;
    Function<void()> sendEventTrigger;
    size_t curEventIndex = 0;

    uint64 linkedThreadId = 0; // Identifier of thread that calls Dispatcher::ProcessEvents method
    Semaphore semaphore; // Semaphore to ensure only one blocking call
    AutoResetEvent signalEvent; // Event to signal about blocking call completion
    bool blockingCallInQueue = false; // Flag indicating that queue contains blocking event
    bool processEventsInProgress = false; // Flag indicating that ProcessEvents in progress to prevent nested event processing
};

template <typename T>
Dispatcher<T>::Dispatcher(const Function<void(const T&)>& handler, const Function<void()> trigger)
    : eventHandler(handler)
    , sendEventTrigger(trigger)
    , semaphore(1)
{
}

template <typename T>
void Dispatcher<T>::LinkToCurrentThread()
{
    linkedThreadId = Thread::GetCurrentIdAsUInt64();
}

template <typename T>
uint64 Dispatcher<T>::GetLinkedThread() const
{
    return linkedThreadId;
}

template <typename T>
bool Dispatcher<T>::HasEvents() const
{
    LockGuard<Mutex> lock(mutex);
    return !eventQueue.empty();
}

template <typename T>
template <typename U>
void Dispatcher<T>::PostEvent(U&& e)
{
    LockGuard<Mutex> lock(mutex);
    eventQueue.emplace_back(std::forward<U>(e));
}

template <typename T>
template <typename U>
void Dispatcher<T>::SendEvent(U&& e, eSendPolicy policy)
{
    DVASSERT(linkedThreadId != 0, "Before calling SendEvent you must call LinkToCurrentThread");

    uint64 curThreadId = Thread::GetCurrentIdAsUInt64();
    if (linkedThreadId == curThreadId)
    {
        switch (policy)
        {
        case eSendPolicy::QUEUED_EXECUTION:
            // clang-format off
            {
                LockGuard<Mutex> lock(mutex);
                eventQueue.emplace_back(std::forward<U>(e));
            }
            // clang-format on
            ProcessEvents();
            break;
        case eSendPolicy::IMMEDIATE_EXECUTION:
            eventHandler(e);
            break;
        default:
            DVASSERT(0);
            break;
        }
    }
    else
    {
        // Wait till current blocking call completion if any
        semaphore.Wait();

        {
            LockGuard<Mutex> lock(mutex);
            eventQueue.emplace_back(std::forward<U>(e));
            blockingCallInQueue = true;
        }

        DAVA_BEGIN_BLOCKING_CALL(linkedThreadId);

        sendEventTrigger();

        signalEvent.Wait();
        semaphore.Post(1);

        DAVA_END_BLOCKING_CALL(linkedThreadId);
    }
}

template <typename T>
void Dispatcher<T>::ProcessEvents()
{
    DVASSERT(!processEventsInProgress, "Dispatcher: nested call of ProcessEvents detected");

    bool shouldCompleteBlockingCall = false;
    processEventsInProgress = true;
    {
        LockGuard<Mutex> lock(mutex);
        eventQueue.swap(readyEvents);
        std::swap(blockingCallInQueue, shouldCompleteBlockingCall);
    }

    curEventIndex = 0;
    for (const T& w : readyEvents)
    {
        eventHandler(w);
        curEventIndex += 1;
    }
    readyEvents.clear();
    if (shouldCompleteBlockingCall)
    {
        signalEvent.Signal();
    }
    processEventsInProgress = false;
}

template <typename T>
void Dispatcher<T>::ViewEventQueue(const Function<void(const T&)>& viewer)
{
    for (size_t i = curEventIndex + 1, n = readyEvents.size(); i < n; ++i)
    {
        viewer(readyEvents[i]);
    }
}

} // namespace DAVA
