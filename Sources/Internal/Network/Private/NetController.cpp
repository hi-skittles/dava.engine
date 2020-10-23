#include <algorithm>

#include <Functional/Function.h>
#include <Debug/DVAssert.h>

#include <Network/Base/IOLoop.h>
#include <Network/Base/NetworkUtils.h>
#include <Network/ServiceRegistrar.h>
#include <Network/NetConfig.h>

#include <Network/Private/ProtoDriver.h>
#include <Network/Private/TCPServerTransport.h>
#include <Network/Private/TCPClientTransport.h>

#include <Network/Private/NetController.h>

namespace DAVA
{
namespace Net
{
// Move dtor to source file to prevent clang warning: 'class' has no out-of-line virtual method definitions
IController::~IController() = default;
IServerTransport::~IServerTransport() = default;
IClientTransport::~IClientTransport() = default;
IServerListener::~IServerListener() = default;
IClientListener::~IClientListener() = default;

NetController::NetController(IOLoop* aLoop, const ServiceRegistrar& aRegistrar, void* aServiceContext, uint32 readTimeout_)
    : loop(aLoop)
    , role(SERVER_ROLE)
    , registrar(aRegistrar)
    , serviceContext(aServiceContext)
    , runningObjects(0)
    , isTerminating(false)
    , readTimeout(readTimeout_)
{
    DVASSERT(loop != NULL);
}

NetController::~NetController()
{
    DVASSERT(0 == runningObjects);
    if (SERVER_ROLE == role)
    {
        for (Vector<IServerTransport *>::iterator i = servers.begin(), e = servers.end(); i != e; ++i)
            delete *i;
    }
    else
    {
        for (ClientEntry& entry : clients)
        {
            SafeDelete(entry.driver);
            SafeDelete(entry.client);
        }
    }
}

bool NetController::ApplyConfig(const NetConfig& config, size_t trIndex)
{
    DVASSERT(true == config.Validate() && trIndex < config.Transports().size());
    if (false == config.Validate() || trIndex >= config.Transports().size())
        return false;

    const Vector<NetConfig::TransportConfig>& trConfig = config.Transports();

    role = config.Role();
    serviceIds = config.Services();
    if (SERVER_ROLE == role)
    {
        servers.reserve(trConfig.size());
        for (size_t i = 0, n = trConfig.size(); i < n; ++i)
        {
            IServerTransport* tr = CreateServerTransport(trConfig[i].type, trConfig[i].endpoint);
            DVASSERT(tr != NULL);
            if (tr != NULL)
                servers.push_back(tr);
        }
    }
    else // if (CLIENT_ROLE == role)
    {
        // For now create only one transport when operating as client
        IClientTransport* tr = CreateClientTransport(trConfig[trIndex].type, trConfig[trIndex].endpoint);
        DVASSERT(tr != NULL);
        if (tr != NULL)
        {
            ProtoDriver* driver = new ProtoDriver(loop, role, registrar, serviceContext);
            driver->SetTransport(tr, &*serviceIds.begin(), serviceIds.size());
            clients.push_back(ClientEntry(tr, driver));
        }
    }
    return true;
}

void NetController::Start()
{
    SERVER_ROLE == role ? loop->Post(MakeFunction(this, &NetController::DoStartServers))
                          :
                          loop->Post(MakeFunction(this, &NetController::DoStartClients));
}

void NetController::Stop(Function<void(IController*)> handler)
{
    DVASSERT(false == isTerminating && handler != nullptr);
    isTerminating = true;
    stopHandler = handler;
    SERVER_ROLE == role ? loop->Post(MakeFunction(this, &NetController::DoStopServers))
                          :
                          loop->Post(MakeFunction(this, &NetController::DoStopClients));
}

void NetController::Restart()
{
    DVASSERT(false == isTerminating);
    loop->Post(MakeFunction(this, &NetController::DoRestart));
}

void NetController::DoStartServers()
{
    size_t failedCount = 0;
    runningObjects = servers.size();
    for (IServerTransport* server : servers)
    {
        int32 res = server->Start(this);
        if (res != 0)
        {
            ++failedCount;
        }
    }

    if (!runningObjects)
    {
        status = NOT_STARTED;
    }
    else if (failedCount == 0)
    {
        status = STARTED;
    }
    else
    {
        status = START_FAILED;
    }
}

void NetController::DoStartClients()
{
    // For now there is always one transport in client role
    runningObjects = 1;
    int res = clients.front().client->Start(this);
    status = (res == 0 ? STARTED : START_FAILED);
}

void NetController::DoRestart()
{
    for (List<ClientEntry>::iterator i = clients.begin(), e = clients.end(); i != e; ++i)
    {
        ClientEntry& entry = *i;
        entry.client->Reset();
    }
    for (size_t i = 0, n = servers.size(); i < n; ++i)
        servers[i]->Reset();
}

void NetController::DoStopServers()
{
    status = NOT_STARTED;
    if (true == clients.empty())
    {
        for (size_t i = 0, n = servers.size(); i < n; ++i)
        {
            servers[i]->Stop();
        }
    }
    else
        DoStopClients();
}

void NetController::DoStopClients()
{
    status = NOT_STARTED;
    for (List<ClientEntry>::iterator i = clients.begin(), e = clients.end(); i != e; ++i)
    {
        ClientEntry& entry = *i;
        entry.client->Stop();
    }
}

void NetController::OnTransportSpawned(IServerTransport* parent, IClientTransport* child)
{
    DVASSERT(std::find(servers.begin(), servers.end(), parent) != servers.end());

    ProtoDriver* driver = new ProtoDriver(loop, role, registrar, serviceContext);
    driver->SetTransport(child, &*serviceIds.begin(), serviceIds.size());
    clients.push_back(ClientEntry(child, driver, parent));

    child->Start(this);
}

void NetController::OnTransportTerminated(IServerTransport* tr)
{
    DVASSERT(std::find(servers.begin(), servers.end(), tr) != servers.end());

    DVASSERT(runningObjects > 0);
    runningObjects -= 1;
    if (0 == runningObjects)
        stopHandler(this);
}

void NetController::OnTransportTerminated(IClientTransport* tr)
{
    List<ClientEntry>::iterator i = std::find(clients.begin(), clients.end(), tr);
    DVASSERT(i != clients.end());
    ClientEntry& entry = *i;
    entry.driver->ReleaseServices();

    if (SERVER_ROLE == role)
    {
        entry.parent->ReclaimClient(entry.client);
        SafeDelete(entry.driver);
        clients.erase(i);

        if (true == isTerminating && true == clients.empty())
        {
            DoStopServers();
        }
    }
    else
    {
        DVASSERT(runningObjects > 0);
        runningObjects -= 1;
        if (0 == runningObjects)
            stopHandler(this);
    }
}

void NetController::OnTransportConnected(IClientTransport* tr, const Endpoint& endp)
{
    DVASSERT(GetClientEntry(tr) != NULL);
    GetClientEntry(tr)->driver->OnConnected(endp);
}

void NetController::OnTransportDisconnected(IClientTransport* tr, int32 error)
{
    DVASSERT(GetClientEntry(tr) != NULL);
    GetClientEntry(tr)->driver->OnDisconnected(error ? ErrorToString(error) : "");
}

void NetController::OnTransportDataReceived(IClientTransport* tr, const void* buffer, size_t length)
{
    ClientEntry* entry = GetClientEntry(tr);
    DVASSERT(entry != NULL);

    if (false == entry->driver->OnDataReceived(buffer, length))
    {
        entry->client->Reset();
    }
}

void NetController::OnTransportSendComplete(IClientTransport* tr)
{
    DVASSERT(GetClientEntry(tr) != NULL);
    GetClientEntry(tr)->driver->OnSendComplete();
}

void NetController::OnTransportReadTimeout(IClientTransport* tr)
{
    DVASSERT(GetClientEntry(tr) != NULL);

    ClientEntry* entry = GetClientEntry(tr);
    if (false == entry->driver->OnTimeout())
    {
        entry->client->Reset();
    }
}

NetController::ClientEntry* NetController::GetClientEntry(IClientTransport* client)
{
    List<ClientEntry>::iterator i = std::find(clients.begin(), clients.end(), client);
    return i != clients.end() ? &*i
                                :
                                NULL;
}

IServerTransport* NetController::CreateServerTransport(eTransportType type, const Endpoint& endpoint)
{
    switch (type)
    {
    case TRANSPORT_TCP:
        return new TCPServerTransport(loop, endpoint, readTimeout);
    default:
        DVASSERT(0 && "Unknown transport type");
        return NULL;
    }
}

IClientTransport* NetController::CreateClientTransport(eTransportType type, const Endpoint& endpoint)
{
    switch (type)
    {
    case TRANSPORT_TCP:
        return new TCPClientTransport(loop, endpoint, readTimeout);
    default:
        DVASSERT(0 && "Unknown transport type");
        return NULL;
    }
}

} // namespace Net
} // namespace DAVA
