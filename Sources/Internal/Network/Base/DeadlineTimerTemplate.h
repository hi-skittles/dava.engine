#ifndef __DAVAENGINE_DEADLINETIMERTEMPLATE_H__
#define __DAVAENGINE_DEADLINETIMERTEMPLATE_H__

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include "Base/Noncopyable.h"

#include "Network/Base/IOLoop.h"

namespace DAVA
{
namespace Net
{
/*
 Template class DeadlineTimerTemplate wraps timer from underlying network library and provides interface to user
 through CRTP idiom. Class specified by template parameter T should inherit DeadlineTimerTemplate and provide some
 members that will be called by base class (DeadlineTimerTemplate) using compile-time polymorphism.
*/
template <typename T>
class DeadlineTimerTemplate : private Noncopyable
{
public:
    DeadlineTimerTemplate(IOLoop* loop);
    ~DeadlineTimerTemplate();

    void CancelWait();

    bool IsOpen() const;
    bool IsClosing() const;

protected:
    int32 DoOpen();
    int32 DoWait(uint32 timeout);
    void DoClose();

    // Thunks between C callbacks and C++ class methods
    static void HandleCloseThunk(uv_handle_t* handle);
    static void HandleTimerThunk(uv_timer_t* handle);

private:
    uv_timer_t uvhandle; // libuv handle itself
    IOLoop* loop; // IOLoop object handle is attached to
    bool isOpen; // Handle has been initialized and can be used in operations
    bool isClosing; // Close has been issued and waiting for close operation complete, used mainly for asserts
};

//////////////////////////////////////////////////////////////////////////
template <typename T>
DeadlineTimerTemplate<T>::DeadlineTimerTemplate(IOLoop* ioLoop)
    : uvhandle()
    , loop(ioLoop)
    , isOpen(false)
    , isClosing(false)
{
    DVASSERT(ioLoop != NULL);
}

template <typename T>
DeadlineTimerTemplate<T>::~DeadlineTimerTemplate()
{
    // libuv handle should be closed before destroying object
    DVASSERT(false == isOpen && false == isClosing);
}

template <typename T>
void DeadlineTimerTemplate<T>::CancelWait()
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(true == isOpen && false == isClosing);
    uv_timer_stop(&uvhandle);
#endif
}

template <typename T>
bool DeadlineTimerTemplate<T>::IsOpen() const
{
    return isOpen;
}

template <typename T>
bool DeadlineTimerTemplate<T>::IsClosing() const
{
    return isClosing;
}

template <typename T>
int32 DeadlineTimerTemplate<T>::DoOpen()
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(false == isOpen && false == isClosing);
    int32 error = uv_timer_init(loop->Handle(), &uvhandle);
    if (0 == error)
    {
        isOpen = true;
        uvhandle.data = this;
    }
    return error;
#else
    return -1;
#endif
}

template <typename T>
int32 DeadlineTimerTemplate<T>::DoWait(uint32 timeout)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(false == isClosing);
    int32 error = 0;
    if (false == isOpen)
        error = DoOpen(); // Automatically open on first call
    if (0 == error)
        error = uv_timer_start(&uvhandle, &HandleTimerThunk, timeout, 0);
    return error;
#else
    return -1;
#endif
}

template <typename T>
void DeadlineTimerTemplate<T>::DoClose()
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(true == isOpen && false == isClosing);
    isOpen = false;
    isClosing = true;
    uv_close(reinterpret_cast<uv_handle_t*>(&uvhandle), &HandleCloseThunk);
#endif
}

///   Thunks   ///////////////////////////////////////////////////////////
template <typename T>
void DeadlineTimerTemplate<T>::HandleCloseThunk(uv_handle_t* handle)
{
    DeadlineTimerTemplate* self = static_cast<DeadlineTimerTemplate*>(handle->data);
    self->isClosing = false; // Mark timer has been closed
    // And clear handle
    Memset(&self->uvhandle, 0, sizeof(self->uvhandle));

    static_cast<T*>(self)->HandleClose();
}

template <typename T>
void DeadlineTimerTemplate<T>::HandleTimerThunk(uv_timer_t* handle)
{
    DeadlineTimerTemplate* self = static_cast<DeadlineTimerTemplate*>(handle->data);
    static_cast<T*>(self)->HandleTimer();
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_DEADLINETIMERTEMPLATE_H__
