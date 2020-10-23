#include <Debug/DVAssert.h>

#include <Network/Base/TCPSocket.h>

namespace DAVA
{
namespace Net
{
TCPSocket::TCPSocket(IOLoop* ioLoop)
    : TCPSocketTemplate<TCPSocket>(ioLoop)
    , readBuffer()
    , closeHandler()
    , shutdownHandler()
    , connectHandler()
    , readHandler()
{
}

int32 TCPSocket::Connect(const Endpoint& endpoint, ConnectHandlerType handler)
{
    DVASSERT(handler != nullptr);
    connectHandler = handler;
    return DoConnect(endpoint);
}

int32 TCPSocket::StartRead(Buffer buffer, ReadHandlerType handler)
{
    DVASSERT(buffer.base != nullptr && buffer.len > 0 && handler != nullptr);
    readBuffer = buffer;
    readHandler = handler;
    return DoStartRead();
}

int32 TCPSocket::Write(const Buffer* buffers, size_t bufferCount, WriteHandlerType handler)
{
    DVASSERT(buffers != nullptr && bufferCount > 0 && handler != nullptr);
    writeHandler = handler;
    return DoWrite(buffers, bufferCount);
}

int32 TCPSocket::Shutdown(ShutdownHandlerType handler)
{
    DVASSERT(handler != nullptr);
    shutdownHandler = handler;
    return DoShutdown();
}

void TCPSocket::Close(CloseHandlerType handler)
{
    closeHandler = handler;
    IsOpen() ? DoClose()
               :
               HandleClose(); // Execute user handle in any case
}

void TCPSocket::ReadHere(Buffer buffer)
{
    DVASSERT(buffer.base != nullptr && buffer.len > 0);
    readBuffer = buffer;
}

void TCPSocket::HandleClose()
{
    if (closeHandler != nullptr)
    {
        closeHandler(this);
    }
}

void TCPSocket::HandleConnect(int32 error)
{
    connectHandler(this, error);
}

void TCPSocket::HandleShutdown(int32 error)
{
    shutdownHandler(this, error);
}

void TCPSocket::HandleAlloc(Buffer* buffer)
{
    *buffer = readBuffer;
}

void TCPSocket::HandleRead(int32 error, size_t nread)
{
    readHandler(this, error, nread);
}

void TCPSocket::HandleWrite(int32 error, const Buffer* buffers, size_t bufferCount)
{
    writeHandler(this, error, buffers, bufferCount);
}

} // namespace Net
} // namespace DAVA
