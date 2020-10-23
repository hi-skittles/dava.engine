#ifndef __DAVAENGINE_TCPSOCKET_H__
#define __DAVAENGINE_TCPSOCKET_H__

#include <Functional/Function.h>

#include <Network/Base/TCPSocketTemplate.h>

namespace DAVA
{
namespace Net
{
/*
 Class TCPSocket provides a TCP socket.
 TCPSocket allows to establish connection and transfer data.
 All operations are executed asynchronously and all these operations must be started in thread context
 where IOLoop is running, i.e. they can be started during handler processing or using IOLoop Post method.
 List of methods starting async operations:
    Connect
    StartRead
    Write
    Shutdown
    Close

 TCPSocket notifies user about operation completion and its status through user-supplied functional objects (handlers or callbacks).
 Handlers are called with some parameters depending on operation. Each handler besides specific parameters is passed a
 pointer to TCPSocket instance.

 Note: handlers should not block, this will cause all network system to freeze.

 To start working with TCPSocket you should do one of the following:
    1. call Connect method and in its handler (if not failed) you can start reading and writing to socket
    2. pass it to Accept method of TCPAcceptor class while handling acceptor's connect callback

 Read operation is started only once, it will be automatically restarted after executing handler. User should
 only provide new destination buffer through ReadHere method if neccesary while in read handler. To stop reading and
 of course any other activity call Close method.

 Write operation supports sending of several buffers at a time. Write handler receives these buffers back and you get
 a chance to free buffers if neccesary.
 Note 1: user should ensure that buffers stay valid and untouched during write operation.
 Note 2: next write must be issued only after completion of previous write operation.

 Shutdown used for gracefull disconnect before calling Close, it shutdowns sending side of socket and waits for
 pending write operations to complete.

 Close also is executed asynchronously and in its handler it is allowed to destroy TCPSocket object.
*/
class TCPAcceptor;
class TCPSocket : public TCPSocketTemplate<TCPSocket>
{
    friend TCPSocketTemplate<TCPSocket>; // Make base class friend to allow it to call my Handle... methods
    friend TCPAcceptor; // Make TCPAcceptor friend to allow it to do useful work in its Accept

public:
    // Handler types
    using CloseHandlerType = Function<void(TCPSocket* socket)>;
    using ShutdownHandlerType = Function<void(TCPSocket* socket, int32 error)>;
    using ConnectHandlerType = Function<void(TCPSocket* socket, int32 error)>;
    using ReadHandlerType = Function<void(TCPSocket* socket, int32 error, size_t nread)>;
    using WriteHandlerType = Function<void(TCPSocket* socket, int32 error, const Buffer* buffers, size_t bufferCount)>;

public:
    TCPSocket(IOLoop* ioLoop);

    int32 Connect(const Endpoint& endpoint, ConnectHandlerType handler);
    int32 StartRead(Buffer buffer, ReadHandlerType handler);
    int32 Write(const Buffer* buffers, size_t bufferCount, WriteHandlerType handler);
    int32 Shutdown(ShutdownHandlerType handler);
    void Close(CloseHandlerType handler = CloseHandlerType());

    void ReadHere(Buffer buffer);

private:
    void HandleClose();
    void HandleConnect(int32 error);
    void HandleShutdown(int32 error);
    void HandleAlloc(Buffer* buffer);
    void HandleRead(int32 error, size_t nread);
    void HandleWrite(int32 error, const Buffer* buffers, size_t bufferCount);

private:
    Buffer readBuffer;
    CloseHandlerType closeHandler;
    ShutdownHandlerType shutdownHandler;
    ConnectHandlerType connectHandler;
    ReadHandlerType readHandler;
    WriteHandlerType writeHandler;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_TCPSOCKET_H__
