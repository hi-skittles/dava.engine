#pragma once

#include "Base/BaseTypes.h"
#include "Concurrency/Atomic.h"

#include "Network/NetworkCommon.h"
#include "Network/IController.h"
#include "Network/Private/ITransport.h"

namespace DAVA
{
namespace Net
{
class IOLoop;
class ServiceRegistrar;
class NetConfig;
class ProtoDriver;

class NetController : public IController,
                      public IServerListener,
                      public IClientListener
{
private:
    struct ClientEntry
    {
        ClientEntry(IClientTransport* aClient, ProtoDriver* aDriver, IServerTransport* aParent = NULL);

        IClientTransport* client;
        IServerTransport* parent;
        ProtoDriver* driver;
    };

    friend bool operator==(const ClientEntry& entry, const IClientTransport* obj);

public:
    NetController(IOLoop* aLoop, const ServiceRegistrar& aRegistrar, void* aServiceContext, uint32 readTimeout = DEFAULT_READ_TIMEOUT);
    virtual ~NetController();

    bool ApplyConfig(const NetConfig& config, size_t trIndex = 0);

    // IController
    Status GetStatus() const override;
    void Start() override;
    void Stop(Function<void(IController*)> handler) override;
    void Restart() override;

    // IServerListener
    void OnTransportSpawned(IServerTransport* parent, IClientTransport* child) override;
    void OnTransportTerminated(IServerTransport* tr) override;

    // IClientListener
    void OnTransportTerminated(IClientTransport* tr) override;
    void OnTransportConnected(IClientTransport* tr, const Endpoint& endp) override;
    void OnTransportDisconnected(IClientTransport* tr, int32 error) override;
    void OnTransportDataReceived(IClientTransport* tr, const void* buffer, size_t length) override;
    void OnTransportSendComplete(IClientTransport* tr) override;
    void OnTransportReadTimeout(IClientTransport* tr) override;

private:
    void DoStartServers();
    void DoStartClients();
    void DoRestart();
    void DoStopServers();
    void DoStopClients();

    ClientEntry* GetClientEntry(IClientTransport* client);

    IServerTransport* CreateServerTransport(eTransportType type, const Endpoint& endpoint);
    IClientTransport* CreateClientTransport(eTransportType type, const Endpoint& endpoint);

private:
    IOLoop* loop;
    eNetworkRole role;
    const ServiceRegistrar& registrar;
    void* serviceContext;
    size_t runningObjects;
    Function<void(IController*)> stopHandler;
    bool isTerminating;
    uint32 readTimeout = 0;

    Atomic<Status> status{ NOT_STARTED };

    Vector<uint32> serviceIds;
    Vector<IServerTransport*> servers;
    List<ClientEntry> clients;
};

//////////////////////////////////////////////////////////////////////////
inline NetController::ClientEntry::ClientEntry(IClientTransport* aClient, ProtoDriver* aDriver, IServerTransport* aParent)
    : client(aClient)
    , parent(aParent)
    , driver(aDriver)
{
}

inline bool operator==(const NetController::ClientEntry& entry, const IClientTransport* obj)
{
    return entry.client == obj;
}

inline IController::Status NetController::GetStatus() const
{
    return status.Get();
}

} // namespace Net
} // namespace DAVA
