#ifndef __DAVAENGINE_TCPACCEPTORTEMPLATE_H__
#define __DAVAENGINE_TCPACCEPTORTEMPLATE_H__

#include "Base/BaseTypes.h"
#include "Base/Noncopyable.h"
#include "Debug/DVAssert.h"

#include "Network/Base/Endpoint.h"
#include "Network/Base/IOLoop.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
namespace Net
{
/*
 Template class TCPAcceptorTemplate wraps TCP acceptor from underlying network library and provides interface to user
 through CRTP idiom. Class specified by template parameter T should inherit TCPAcceptorTemplate and provide some
 members that will be called by base class (TCPAcceptorTemplate) using compile-time polymorphism.
*/
template <typename T>
class TCPAcceptorTemplate : private Noncopyable
{
public:
    TCPAcceptorTemplate(IOLoop* ioLoop);
    ~TCPAcceptorTemplate();

    int32 Bind(const Endpoint& endpoint);

    bool IsOpen() const;
    bool IsClosing() const;

protected:
    int32 DoOpen();
    int32 DoAccept(uv_tcp_t* client);
    int32 DoStartListen(int32 backlog);
    void DoClose();

private:
    // Thunks between C callbacks and C++ class methods
    static void HandleCloseThunk(uv_handle_t* handle);
    static void HandleConnectThunk(uv_stream_t* handle, int error);

private:
    uv_tcp_t uvhandle; // libuv handle itself
    IOLoop* loop; // IOLoop object handle is attached to
    bool isOpen; // Handle has been initialized and can be used in operations
    bool isClosing; // Close has been issued and waiting for close operation complete, used mainly for asserts
};

//////////////////////////////////////////////////////////////////////////
template <typename T>
TCPAcceptorTemplate<T>::TCPAcceptorTemplate(IOLoop* ioLoop)
    : uvhandle()
    , loop(ioLoop)
    , isOpen(false)
    , isClosing(false)
{
    DVASSERT(ioLoop != NULL);
}

template <typename T>
TCPAcceptorTemplate<T>::~TCPAcceptorTemplate()
{
    // libuv handle should be closed before destroying object
    DVASSERT(false == isOpen && false == isClosing);
}

template <typename T>
int32 TCPAcceptorTemplate<T>::Bind(const Endpoint& endpoint)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(false == isClosing);
    int32 error = 0;
    if (false == isOpen)
        error = DoOpen(); // Automatically open on first call
    if (0 == error)
        error = uv_tcp_bind(&uvhandle, endpoint.CastToSockaddr(), 0);
    return error;
#else
    return -1;
#endif
}

template <typename T>
bool TCPAcceptorTemplate<T>::IsOpen() const
{
    return isOpen;
}

template <typename T>
bool TCPAcceptorTemplate<T>::IsClosing() const
{
    return isClosing;
}

template <typename T>
int32 TCPAcceptorTemplate<T>::DoOpen()
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(false == isOpen && false == isClosing);
    int32 error = uv_tcp_init(loop->Handle(), &uvhandle);
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
int32 TCPAcceptorTemplate<T>::DoAccept(uv_tcp_t* client)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(true == isOpen && false == isClosing && client != NULL);
    return uv_accept(reinterpret_cast<uv_stream_t*>(&uvhandle), reinterpret_cast<uv_stream_t*>(client));
#else
    return -1;
#endif
}

template <typename T>
int32 TCPAcceptorTemplate<T>::DoStartListen(int32 backlog)
{
#if !defined(DAVA_NETWORK_DISABLE)
    // Acceptor should be bound first
    DVASSERT(true == isOpen && false == isClosing && backlog > 0);
    return uv_listen(reinterpret_cast<uv_stream_t*>(&uvhandle), backlog, &HandleConnectThunk);
#else
    return -1;
#endif
}

template <typename T>
void TCPAcceptorTemplate<T>::DoClose()
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
void TCPAcceptorTemplate<T>::HandleCloseThunk(uv_handle_t* handle)
{
    TCPAcceptorTemplate* self = static_cast<TCPAcceptorTemplate*>(handle->data);
    self->isClosing = false; // Mark acceptor has been closed
    // And clear handle
    Memset(&self->uvhandle, 0, sizeof(self->uvhandle));

    static_cast<T*>(self)->HandleClose();
}

template <typename T>
void TCPAcceptorTemplate<T>::HandleConnectThunk(uv_stream_t* handle, int error)
{
    TCPAcceptorTemplate* self = static_cast<TCPAcceptorTemplate*>(handle->data);
    static_cast<T*>(self)->HandleConnect(error);
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_TCPACCEPTORTEMPLATE_H__
