#ifndef __DAVAENGINE_TCPCLIENTTRANSPORT_H__
#define __DAVAENGINE_TCPCLIENTTRANSPORT_H__

#include <Network/Base/Endpoint.h>
#include <Network/Base/TCPSocket.h>
#include <Network/Base/DeadlineTimer.h>

#include <Network/Private/ITransport.h>

namespace DAVA
{
namespace Net
{
class IOLoop;
class TCPClientTransport : public IClientTransport
{
    static const uint32 RESTART_DELAY_PERIOD = 3000;

public:
    // Constructor for accepted connection
    TCPClientTransport(IOLoop* aLoop, uint32 readTimeout);
    // Constructor for connection initiator
    TCPClientTransport(IOLoop* aLoop, const Endpoint& aEndpoint, uint32 readTimeout);
    virtual ~TCPClientTransport();

    TCPSocket& Socket();

    // IClientTransport
    virtual int32 Start(IClientListener* aListener);
    virtual void Stop();
    virtual void Reset();
    virtual int32 Send(const Buffer* buffers, size_t bufferCount);

private:
    void DoStart();
    int32 DoConnected();
    void CleanUp(int32 error);
    void RunningObjectStopped();
    void DoBye();

    void TimerHandleClose(DeadlineTimer* timer);
    void TimerHandleTimeout(DeadlineTimer* timer);
    void TimerHandleDelay(DeadlineTimer* timer);

    void SocketHandleClose(TCPSocket* socket);
    void SocketHandleConnect(TCPSocket* socket, int32 error);
    void SocketHandleRead(TCPSocket* socket, int32 error, size_t nread);
    void SocketHandleWrite(TCPSocket* socket, int32 error, const Buffer* buffers, size_t bufferCount);

private:
    IOLoop* loop;
    Endpoint endpoint;
    Endpoint remoteEndpoint;
    size_t runningObjects;
    TCPSocket socket;
    DeadlineTimer timer;
    IClientListener* listener; // Who receive notifications; also indicator that Start has been called
    uint32 readTimeout;
    bool isInitiator; // true: establishes connection; false: created from accepted connection
    bool isTerminating; // Stop has been invoked
    bool isConnected; // Connections has been established

    static const size_t INBUF_SIZE = 10 * 1024;
    uint8 inbuf[INBUF_SIZE];

    static const size_t SENDBUF_COUNT = 2;
    Buffer sendBuffers[SENDBUF_COUNT];
    size_t sendBufferCount;
};

//////////////////////////////////////////////////////////////////////////
inline TCPSocket& TCPClientTransport::Socket()
{
    return socket;
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_TCPCLIENTTRANSPORT_H__
