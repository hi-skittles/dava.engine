#include "Logger/Logger.h"
#include "Network/ServicesProvider.h"
#include "Network/NetService.h"
#include "Network/PeerDesription.h"
#include "Network/NetCore.h"
#include "Network/NetConfig.h"
#include "Utils/StringFormat.h"
#include "Platform/TemplateWin32/UAPNetworkHelper.h"

namespace DAVA
{
namespace Net
{
namespace ServicesProviderDetails
{
// TCP ports are occupied 1) for providing of specified network services 2) for announcing
uint16 FIRST_ALLOWED_TCP_PORT = NetCore::DEFAULT_TCP_ANNOUNCE_PORT + 2; // number of first TCP port allowed to be occupied by ServiceProvider.
uint16 MAX_TCP_PORTS_ALLOWED = 20; // number of TCP ports allowed to be occupied by ServiceProvider
float32 WAITING_CONTROLLER_START_SEC = 2.f;

struct ServiceContext
{
    ServiceContext(std::shared_ptr<IChannelListener>& service)
        : netService(service)
    {
    }

    IChannelListener* ServiceCreatorFn(ServiceID serviceId, void*);
    void ServiceDeleterFn(IChannelListener* obj, void*);

    std::shared_ptr<IChannelListener> netService;
    bool serviceInUse = false;
};

IChannelListener* ServiceContext::ServiceCreatorFn(ServiceID serviceId, void*)
{
    if (!serviceInUse)
    {
        serviceInUse = true;
        return netService.get();
    }
    return nullptr;
};

void ServiceContext::ServiceDeleterFn(IChannelListener* obj, void*)
{
    serviceInUse = false;
}
} // namespace ServicesProviderDetails

class ServicesProvider::ServicesProviderImpl
{
public:
    ServicesProviderImpl(Engine& engine, const String& appName);
    ~ServicesProviderImpl();
    void AddService(ServiceID serviceId, std::shared_ptr<IChannelListener>& service);
    void Start();
    void Stop();

private:
    enum State
    {
        NOT_STARTED,
        STARTING_SERVICES_CONTROLLER,
        STARTING_ANNOUNCE_CONTROLLER,
        STARTED,
        STOPPED
    };

    void ChangeState(State);
    void QuitFromState(State);
    void EnterToState(State);
    void ReenterState(State);

    State currentState = NOT_STARTED;

private:
    void StartServicesController();
    void StartAnnounceController();
    void StopServicesController();
    void StopAnnounceController();
    void SetFirstTcpPort();
    bool SetNextTcpPort();
    void ResetTcpEndpoint();
    void TryUseNextPort();
    size_t AnnounceDataSupplier(size_t length, void* buffer);
    void OnUpdate(float32);
    void OnControllerStarted();

private:
    Engine& engine;
    String applicationName;

    std::unique_ptr<Net::NetConfig> config;
    PeerDescription peerDescr;

    NetCore::TrackId id_anno = Net::NetCore::INVALID_TRACK_ID;
    NetCore::TrackId id_net = Net::NetCore::INVALID_TRACK_ID;
    NetCore::TrackId startingControllerId = Net::NetCore::INVALID_TRACK_ID;

    uint16 tcpPortOffset = 0;
    Net::Endpoint tcpEndpoint;

    Map<ServiceID, ServicesProviderDetails::ServiceContext> services;

    Token updateSignalId;
    float32 elapsedSinceStartMs = 0.f;
};

ServicesProvider::ServicesProviderImpl::ServicesProviderImpl(Engine& engine, const String& appName)
    : engine(engine)
    , applicationName(appName)
{
}

ServicesProvider::ServicesProviderImpl::~ServicesProviderImpl()
{
    Stop();
    for (auto& serviceEntry : services)
    {
        NetCore::Instance()->UnregisterService(serviceEntry.first);
    }
}

void ServicesProvider::ServicesProviderImpl::AddService(ServiceID serviceId, std::shared_ptr<IChannelListener>& service)
{
    DVASSERT(currentState == NOT_STARTED);
    DVASSERT(service);

    using namespace ServicesProviderDetails;

    auto result = services.emplace(serviceId, ServiceContext(service));

    if (result.second == false)
    {
        DVASSERT(false, Format("Service %u is already added", serviceId).c_str());
        return;
    }

    ServiceContext& serviceContext = result.first->second;
    ServiceCreator serviceCreator = MakeFunction(&serviceContext, &ServiceContext::ServiceCreatorFn);
    ServiceDeleter serviceDeleter = MakeFunction(&serviceContext, &ServiceContext::ServiceDeleterFn);
    NetCore::Instance()->RegisterService(serviceId, serviceCreator, serviceDeleter);
}

void ServicesProvider::ServicesProviderImpl::Start()
{
    DVASSERT(services.empty() == false);
    DVASSERT(currentState == NOT_STARTED);
    SetFirstTcpPort();
    ChangeState(State::STARTING_SERVICES_CONTROLLER);
}

void ServicesProvider::ServicesProviderImpl::Stop()
{
    ChangeState(State::STOPPED);
}

void ServicesProvider::ServicesProviderImpl::ChangeState(ServicesProvider::ServicesProviderImpl::State newState)
{
    DVASSERT(currentState != newState);
    QuitFromState(currentState);
    currentState = newState;
    EnterToState(currentState);
}

void ServicesProvider::ServicesProviderImpl::QuitFromState(ServicesProvider::ServicesProviderImpl::State state)
{
    switch (state)
    {
    case STARTING_SERVICES_CONTROLLER:
    case STARTING_ANNOUNCE_CONTROLLER:
        engine.update.Disconnect(updateSignalId);
        break;
    default:
        break;
    }
}

void ServicesProvider::ServicesProviderImpl::EnterToState(ServicesProvider::ServicesProviderImpl::State state)
{
    switch (state)
    {
    case STARTING_SERVICES_CONTROLLER:
        updateSignalId = engine.update.Connect(this, &ServicesProviderImpl::OnUpdate);
        StartServicesController();
        break;
    case STARTING_ANNOUNCE_CONTROLLER:
        updateSignalId = engine.update.Connect(this, &ServicesProviderImpl::OnUpdate);
        StartAnnounceController();
        break;
    case STOPPED:
        StopServicesController();
        StopAnnounceController();
    default:
        break;
    }
}

void ServicesProvider::ServicesProviderImpl::ReenterState(ServicesProvider::ServicesProviderImpl::State state)
{
    switch (state)
    {
    case STARTING_SERVICES_CONTROLLER:
        StopServicesController();
        StartServicesController();
        break;
    case STARTING_ANNOUNCE_CONTROLLER:
        StopAnnounceController();
        StartAnnounceController();
        break;
    default:
        DVASSERT(false, Format("Unexpected state to reenter: %u", state).c_str());
        Stop();
        break;
    }
}

void ServicesProvider::ServicesProviderImpl::StartServicesController()
{
    config.reset(new NetConfig(SERVER_ROLE));
    config->AddTransport(TRANSPORT_TCP, tcpEndpoint);

    for (auto& serviceEntry : services)
    {
        config->AddService(serviceEntry.first);
    }

    elapsedSinceStartMs = 0.f;
    id_net = NetCore::Instance()->CreateController(*config, nullptr);
    startingControllerId = id_net;
}

void ServicesProvider::ServicesProviderImpl::StartAnnounceController()
{
    peerDescr = PeerDescription(applicationName, *config);
    Net::Endpoint annoUdpEndpoint(NetCore::defaultAnnounceMulticastGroup, NetCore::DEFAULT_UDP_ANNOUNCE_PORT);
    id_anno = NetCore::Instance()->CreateAnnouncer(annoUdpEndpoint, DEFAULT_ANNOUNCE_TIME_PERIOD_SEC, MakeFunction(this, &ServicesProviderImpl::AnnounceDataSupplier), tcpEndpoint);
    startingControllerId = id_anno;
}

void ServicesProvider::ServicesProviderImpl::StopServicesController()
{
    if (id_net != Net::NetCore::INVALID_TRACK_ID)
    {
        NetCore::Instance()->DestroyControllerBlocked(id_net);
        id_net = Net::NetCore::INVALID_TRACK_ID;
        startingControllerId = Net::NetCore::INVALID_TRACK_ID;
    }
}

void ServicesProvider::ServicesProviderImpl::StopAnnounceController()
{
    if (id_anno != Net::NetCore::INVALID_TRACK_ID)
    {
        NetCore::Instance()->DestroyControllerBlocked(id_anno);
        id_anno = Net::NetCore::INVALID_TRACK_ID;
        startingControllerId = Net::NetCore::INVALID_TRACK_ID;
    }
}

void ServicesProvider::ServicesProviderImpl::SetFirstTcpPort()
{
    tcpPortOffset = 0;
    ResetTcpEndpoint();
}

bool ServicesProvider::ServicesProviderImpl::SetNextTcpPort()
{
    if (++tcpPortOffset < ServicesProviderDetails::MAX_TCP_PORTS_ALLOWED)
    {
        ResetTcpEndpoint();
        return true;
    }
    else
    {
        Logger::Error("Can't start net controller: no more allowed ports remaining");
        return false;
    }
}

void ServicesProvider::ServicesProviderImpl::ResetTcpEndpoint()
{
    tcpEndpoint = Net::Endpoint(ServicesProviderDetails::FIRST_ALLOWED_TCP_PORT + tcpPortOffset);
}

void ServicesProvider::ServicesProviderImpl::OnUpdate(float32 elapsedMs)
{
    IController::Status status = NetCore::Instance()->GetControllerStatus(startingControllerId);
    switch (status)
    {
    case IController::STARTED:
    {
        OnControllerStarted();
        break;
    }
    case IController::START_FAILED:
    {
        TryUseNextPort();
        break;
    }
    default:
    {
        elapsedSinceStartMs += elapsedMs;
        if (elapsedSinceStartMs >= ServicesProviderDetails::WAITING_CONTROLLER_START_SEC)
        {
            TryUseNextPort();
        }
        break;
    }
    }
}

void ServicesProvider::ServicesProviderImpl::OnControllerStarted()
{
    switch (currentState)
    {
    case STARTING_SERVICES_CONTROLLER:
    {
        DAVA::Logger::FrameworkDebug("Port %u is occupied by '%s' for network services", tcpEndpoint.Port(), applicationName.c_str());
        bool isOk = SetNextTcpPort();
        if (isOk)
        {
            ChangeState(STARTING_ANNOUNCE_CONTROLLER);
        }
        else
        {
            Stop();
        }
        break;
    }
    case STARTING_ANNOUNCE_CONTROLLER:
        DAVA::Logger::FrameworkDebug("Port %u is occupied by '%s' for network announcing", tcpEndpoint.Port(), applicationName.c_str());
        ChangeState(STARTED);
        break;
    default:
        DVASSERT(false, Format("unexpected state: %u", currentState).c_str());
        Stop();
        break;
    }
}

void ServicesProvider::ServicesProviderImpl::TryUseNextPort()
{
    bool isOk = SetNextTcpPort();
    if (isOk)
    {
        ReenterState(currentState);
    }
    else
    {
        Stop();
    }
}

size_t ServicesProvider::ServicesProviderImpl::AnnounceDataSupplier(size_t length, void* buffer)
{
    if (true == peerDescr.NetworkInterfaces().empty())
    {
        peerDescr.SetNetworkInterfaces(NetCore::Instance()->InstalledInterfaces());
    }
    return peerDescr.Serialize(buffer, length);
}

//////////////////////////////////////////////////////////////////////////

ServicesProvider::ServicesProvider(Engine& engine, const String& appName)
{
    impl = std::make_unique<ServicesProviderImpl>(engine, appName);
}

ServicesProvider::~ServicesProvider()
{
}

void ServicesProvider::AddService(ServiceID serviceId, std::shared_ptr<IChannelListener>& service)
{
    impl->AddService(serviceId, service);
}

void ServicesProvider::Start()
{
    impl->Start();
}

void ServicesProvider::Stop()
{
    impl->Stop();
}

std::pair<uint16, uint16> ServicesProvider::GetTcpPortsRange()
{
    return std::pair<uint16, uint16>(DAVA::Net::NetCore::DEFAULT_TCP_ANNOUNCE_PORT, ServicesProviderDetails::FIRST_ALLOWED_TCP_PORT + ServicesProviderDetails::MAX_TCP_PORTS_ALLOWED);
}

} // NetCore
} // DAVA
