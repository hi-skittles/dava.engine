#ifndef __DAVAENGINE_UDPSOCKET_H__
#define __DAVAENGINE_UDPSOCKET_H__

#include <Functional/Function.h>

#include <Network/Base/UDPSocketTemplate.h>

namespace DAVA
{
namespace Net
{
/*
 Class UDPSocket provides a UDP socket.
 UDPSocket allows to send and receive UDP datagrams.
 All operations are executed asynchronously and all these operations must be started in thread context
 where IOLoop is running, i.e. they can be started during handler processing or using IOLoop Post method.
 List of methods starting async operations:
    StartReceive
    Send
    Close

 UDPSocket notifies user about operation completion and its status through user-supplied functional objects (handlers or callbacks).
 Handlers are called with some parameters depending on operation. Each handler besides specific parameters is passed a
 pointer to UDPSocket instance.

 Note: handlers should not block, this will cause all network system to freeze.

 To start working with UDPSocket you should do one of the following:
    1. call one of Bind methods to bind to specified port and after that you can read and write to socket
    2. call StartReceive

 Receive operation is started only once, it will be automatically restarted after executing handler. User should
 only provide new destination buffer through ReceiveHere method if neccesary while in receive handler. To stop receiving
 and of course any other activity call Close method.

 Send operation supports writing of several buffers at a time. Send handler receives these buffers back and you get
 a chance to free buffers if neccesary.
 Note 1: user should ensure that buffers stay valid and untouched during send operation.
 Note 2: next send must be issued only after completion of previous send operation.

 Close also is executed asynchronously and in its handler it is allowed to destroy UDPSocket object.
*/
class UDPSocket : public UDPSocketTemplate<UDPSocket>
{
private:
    friend UDPSocketTemplate<UDPSocket>; // Make base class friend to allow it to call my Handle... methods

public:
    using CloseHandlerType = Function<void(UDPSocket* socket)>;
    using ReceiveHandlerType = Function<void(UDPSocket* socket, int32 error, size_t nread, const Endpoint& endpoint, bool partial)>;
    using SendHandlerType = Function<void(UDPSocket* socket, int32 error, const Buffer* buffers, size_t bufferCount)>;

public:
    UDPSocket(IOLoop* ioLoop);

    int32 StartReceive(Buffer buffer, ReceiveHandlerType handler);
    int32 Send(const Endpoint& endpoint, const Buffer* buffers, size_t bufferCount, SendHandlerType handler);
    void Close(CloseHandlerType handler = CloseHandlerType());

    void ReceiveHere(Buffer buffer);

private:
    void HandleClose();
    void HandleAlloc(Buffer* buffer);
    void HandleReceive(int32 error, size_t nread, const Endpoint& endpoint, bool partial);
    void HandleSend(int32 error, const Buffer* buffers, size_t bufferCount);

private:
    Buffer readBuffer;
    CloseHandlerType closeHandler;
    ReceiveHandlerType receiveHandler;
    SendHandlerType sendHandler;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_UDPSOCKET_H__
