#ifndef __DAVAENGINE_TCPACCEPTOR_H__
#define __DAVAENGINE_TCPACCEPTOR_H__

#include <Functional/Function.h>

#include <Network/Base/TCPAcceptorTemplate.h>

namespace DAVA
{
namespace Net
{
/*
 Class TCPAcceptor provides a TCP acceptor type.
 TCPAcceptor allows to listen at specified port and accept incoming connection putting TCPSocket into connected state.
 All operations are executed asynchronously and all these operations must be started in thread context
 where IOLoop is running, i.e. they can be started during handler processing or using IOLoop Post method.
 List of methods starting async operations:
    StartListen
    Close

 TCPAcceptor notifies user about operation completion and its status through user-supplied functional objects (handlers or callbacks).
 Handlers are called with some parameters depending on operation. Each handler besides specific parameters is passed a
 pointer to TCPAcceptor instance.

 Note: handlers should not block, this will cause all network system to freeze.

 To start working with TCPAcceptor you should call one of the Bind methods and call StartListen and in its handler
 call Accept.

 Listen operation is started only once, it will be automatically restarted after executing handler. To stop listening
 call Close method.

 Close also is executed asynchronously and in its handler it is allowed to destroy TCPSocket object.
*/
class TCPSocket;
class TCPAcceptor : public TCPAcceptorTemplate<TCPAcceptor>
{
private:
    friend TCPAcceptorTemplate<TCPAcceptor>; // Make base class friend to allow it to call my Handle... methods

public:
    using CloseHandlerType = Function<void(TCPAcceptor* acceptor)>;
    using ConnectHandlerType = Function<void(TCPAcceptor* acceptor, int32 error)>;

public:
    TCPAcceptor(IOLoop* ioLoop);

    int32 StartListen(ConnectHandlerType handler, int32 backlog = SOMAXCONN);
    void Close(CloseHandlerType handler = CloseHandlerType());

    int32 Accept(TCPSocket* socket);

private:
    void HandleClose();
    void HandleConnect(int32 error);

private:
    CloseHandlerType closeHandler;
    ConnectHandlerType connectHandler;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_TCPACCEPTOR_H__
