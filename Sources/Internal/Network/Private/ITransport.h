#ifndef __DAVAENGINE_ITRANSPORT_H__
#define __DAVAENGINE_ITRANSPORT_H__

#include <Network/Base/Buffer.h>
#include <Network/Base/Endpoint.h>
#include <Network/NetworkCommon.h>

namespace DAVA
{
namespace Net
{
struct IClientTransport;
struct IServerListener;

struct IServerTransport
{
    virtual ~IServerTransport();

    virtual int32 Start(IServerListener* listener) = 0;
    virtual void Stop() = 0;
    virtual void Reset() = 0;
    virtual void ReclaimClient(IClientTransport* client) = 0;
};

struct IServerListener
{
    virtual ~IServerListener();

    virtual void OnTransportSpawned(IServerTransport* parent, IClientTransport* child) = 0;
    virtual void OnTransportTerminated(IServerTransport* tr) = 0;
};

//////////////////////////////////////////////////////////////////////////
struct IClientListener;

struct IClientTransport
{
    virtual ~IClientTransport();

    virtual int32 Start(IClientListener* listener) = 0;
    virtual void Stop() = 0;
    virtual void Reset() = 0;
    virtual int32 Send(const Buffer* buffers, size_t bufferCount) = 0;
};

struct IClientListener
{
    virtual ~IClientListener();

    virtual void OnTransportTerminated(IClientTransport* tr) = 0;
    virtual void OnTransportConnected(IClientTransport* tr, const Endpoint& endp) = 0;
    virtual void OnTransportDisconnected(IClientTransport* tr, int32 error) = 0;
    virtual void OnTransportDataReceived(IClientTransport* tr, const void* buffer, size_t length) = 0;
    virtual void OnTransportSendComplete(IClientTransport* tr) = 0;
    virtual void OnTransportReadTimeout(IClientTransport* tr) = 0;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_ITRANSPORT_H__
