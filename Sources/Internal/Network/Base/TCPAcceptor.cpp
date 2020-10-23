#include <Debug/DVAssert.h>

#include <Network/Base/TCPSocket.h>
#include <Network/Base/TCPAcceptor.h>

namespace DAVA
{
namespace Net
{
TCPAcceptor::TCPAcceptor(IOLoop* ioLoop)
    : TCPAcceptorTemplate(ioLoop)
    , closeHandler()
    , connectHandler()
{
}

int32 TCPAcceptor::StartListen(ConnectHandlerType handler, int32 backlog)
{
    DVASSERT(handler != nullptr);
    connectHandler = handler;
    return DoStartListen(backlog);
}

void TCPAcceptor::Close(CloseHandlerType handler)
{
    closeHandler = handler;
    IsOpen() ? DoClose()
               :
               HandleClose(); // Execute user handle in any case
}

int32 TCPAcceptor::Accept(TCPSocket* socket)
{
    DVASSERT(socket != NULL && false == socket->IsOpen());
    // To do the following TCPAcceptor must be friend of TCPSocket
    int32 error = socket->DoOpen();
    if (0 == error)
        error = DoAccept(&socket->uvhandle);
    return error;
}

void TCPAcceptor::HandleClose()
{
    if (closeHandler != nullptr)
    {
        closeHandler(this);
    }
}

void TCPAcceptor::HandleConnect(int32 error)
{
    connectHandler(this, error);
}

} // namespace Net
} // namespace DAVA
