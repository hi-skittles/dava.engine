#include <Debug/DVAssert.h>

#include <Network/Base/UDPSocket.h>

namespace DAVA
{
namespace Net
{
UDPSocket::UDPSocket(IOLoop* ioLoop)
    : UDPSocketTemplate<UDPSocket>(ioLoop)
    , readBuffer()
    , closeHandler()
    , receiveHandler()
{
}

int32 UDPSocket::StartReceive(Buffer buffer, ReceiveHandlerType handler)
{
    DVASSERT(buffer.base != nullptr && buffer.len > 0 && handler != nullptr);
    readBuffer = buffer;
    receiveHandler = handler;
    return DoStartReceive();
}

int32 UDPSocket::Send(const Endpoint& endpoint, const Buffer* buffers, size_t bufferCount, SendHandlerType handler)
{
    DVASSERT(buffers != nullptr && bufferCount > 0 && handler != nullptr);
    sendHandler = handler;
    return DoSend(buffers, bufferCount, endpoint);
}

void UDPSocket::Close(CloseHandlerType handler)
{
    closeHandler = handler;
    IsOpen() ? DoClose()
               :
               HandleClose(); // Execute user handle in any case
}

void UDPSocket::ReceiveHere(Buffer buffer)
{
    DVASSERT(buffer.base != nullptr && buffer.len > 0);
    readBuffer = buffer;
}

void UDPSocket::HandleClose()
{
    if (closeHandler != nullptr)
    {
        closeHandler(this);
    }
}

void UDPSocket::HandleAlloc(Buffer* buffer)
{
    *buffer = readBuffer;
}

void UDPSocket::HandleReceive(int32 error, size_t nread, const Endpoint& endpoint, bool partial)
{
    receiveHandler(this, error, nread, endpoint, partial);
}

void UDPSocket::HandleSend(int32 error, const Buffer* buffers, size_t bufferCount)
{
    sendHandler(this, error, buffers, bufferCount);
}

} // namespace Net
} // namespace DAVA
