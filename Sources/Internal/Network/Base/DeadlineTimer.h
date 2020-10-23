#ifndef __DAVAENGINE_DEADLINETIMER_H__
#define __DAVAENGINE_DEADLINETIMER_H__

#include <Functional/Function.h>

#include <Network/Base/DeadlineTimerTemplate.h>

namespace DAVA
{
namespace Net
{
/*
 Class DeadlineTimer provides a waitable timer.
 DeadlineTimer allows to wait for specified amount of time.
 All operations are executed asynchronously and all these operations must be started in thread context
 where IOLoop is running, i.e. they can be started during handler processing or using IOLoop Post method.
 List of methods starting async operations:
    Wait
    Close

 DeadlineTimer notifies user about wait or close  operation completion through user-supplied functional objects (handlers or callbacks).
 Handlers are called with one parameter: pointer to DeadlineTimer instance.

 Note: handlers should not block, this will cause all network system to freeze.

 To start working with DeadlineTimer simply call Wait with desired amount of time in ms.

 Close also is executed asynchronously and in its handler it is allowed to destroy DeadlineTimer object.
*/
class DeadlineTimer : public DeadlineTimerTemplate<DeadlineTimer>
{
    friend DeadlineTimerTemplate<DeadlineTimer>; // Make base class friend to allow it to call my Handle... methods

public:
    using CloseHandlerType = Function<void(DeadlineTimer* timer)>;
    using WaitHandlerType = Function<void(DeadlineTimer* timer)>;

public:
    DeadlineTimer(IOLoop* loop);

    int32 Wait(uint32 timeout, WaitHandlerType handler);
    void Close(CloseHandlerType handler = CloseHandlerType());

private:
    void HandleClose();
    void HandleTimer();

private:
    CloseHandlerType closeHandler;
    WaitHandlerType waitHandler;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_DEADLINETIMER_H__
