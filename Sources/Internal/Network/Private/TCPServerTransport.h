#ifndef __DAVAENGINE_TCPSERVERTRANSPORT_H__
#define __DAVAENGINE_TCPSERVERTRANSPORT_H__

#include <Network/Base/Endpoint.h>
#include <Network/Base/TCPAcceptor.h>

#include <Network/Private/ITransport.h>

namespace DAVA
{
namespace Net
{
class IOLoop;
class TCPServerTransport : public IServerTransport
{
public:
    TCPServerTransport(IOLoop* aLoop, const Endpoint& aEndpoint, uint32 readTimeoutMs);
    virtual ~TCPServerTransport();

    // IServerTransport
    virtual int32 Start(IServerListener* listener);
    virtual void Stop();
    virtual void Reset();
    virtual void ReclaimClient(IClientTransport* client);

private:
    int32 DoStart();
    void DoStop();

    void AcceptorHandleClose(TCPAcceptor* acceptor);
    void AcceptorHandleConnect(TCPAcceptor* acceptor, int32 error);

private:
    IOLoop* loop;
    Endpoint endpoint;
    TCPAcceptor acceptor;
    IServerListener* listener; // Who receive notifications; also indicator that Start has been called
    bool isTerminating; // Stop has been invoked
    uint32 readTimeout = 0;

    Set<IClientTransport*> spawnedClients;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_TCPSERVERTRANSPORT_H__
