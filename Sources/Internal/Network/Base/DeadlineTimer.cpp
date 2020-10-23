#include <Debug/DVAssert.h>

#include <Network/Base/DeadlineTimer.h>

namespace DAVA
{
namespace Net
{
DeadlineTimer::DeadlineTimer(IOLoop* loop)
    : DeadlineTimerTemplate<DeadlineTimer>(loop)
    , closeHandler()
    , waitHandler()
{
}

int32 DeadlineTimer::Wait(uint32 timeout, WaitHandlerType handler)
{
    DVASSERT(handler != nullptr);
    waitHandler = handler;
    return DoWait(timeout);
}

void DeadlineTimer::Close(CloseHandlerType handler)
{
    closeHandler = handler;
    IsOpen() ? DoClose()
               :
               HandleClose(); // Execute user handle in any case
}

void DeadlineTimer::HandleClose()
{
    if (closeHandler != nullptr)
    {
        closeHandler(this);
    }
}

void DeadlineTimer::HandleTimer()
{
    waitHandler(this);
}

} // namespace Net
} // namespace DAVA
