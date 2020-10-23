#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Concurrency/Dispatcher.h"
#include "Functional/Function.h"

#include "Network/Base/IOLoop.h"
#include "Network/Base/IfAddress.h"
#include "Network/Base/Endpoint.h"
#include "Network/ServiceRegistrar.h"
#include "Network/IController.h"
#include "Network/NetworkCommon.h"
#include "Network/NetCore.h"
#include "Concurrency/Thread.h"
#include "Concurrency/ManualResetEvent.h"

namespace DAVA
{
class Engine;
namespace Net
{
class NetConfig;

/** NetCore is a main object for using DAVA Engine network functionality

    NetCore is a DAVA Engine module, thus it will be created by Engine if it is passed in modules list
    NetCore can run network logic in a separate thread. To do that, option "separate_net_thread = true"
    should be passed to Engine.

    Example:
        \code
        Vector<String> modules = { "NetCore" };

        KeyedArchive* options = new KeyedArchive;
        options->SetBool("separate_net_thread", true);

        Engine e;
        e.Init(eEngineRunMode::CONSOLE_MODE, modules, options);
        e.Run();
        \endcode

    If NetCore is running it separate thread, all callbacks from network will be invoked it that network thread.
    In that case, it is recommended to perform processing as soon as possible to prevent freeze of network.
    Optionally, user could use proxy classes that pass callbacks from network thread using NetEventsDispatcher.
    NetCore assumes that user may use NetEventsDispatcher so it regularly checks dispatcher and executes callbacks from it.

    Typical use of NetCore functionality:

        \code
        ServiceCreator myServiceCreatorCb = MakeFunction(this, &MyClass::Creator);
        ServiceDeleter myServiceDeleterCb = MakeFunction(this, &MyClass::Deleter);
        NetCore::Instance()->RegisterService(MY_SERVICE_ID, myServiceCreatorCb, myServiceDeleterCb);

        Net::NetConfig config(role);
        config.AddTransport(transport, endpoint);
        config.AddService(MY_SERVICE_ID);
        auto controllerId = NetCore::Instance()->CreateController(config, this);

        ....

        NetCore::Instance()->DestoryController(controllerId);
        \endcode

    Note that DAVA network functionality can be used without NetCore.
    In that case, user should create and manage its own IOLoop, ServiceRegistrar objects.



    Dispatcher<Function(void)> that is retrieved by GetNetEventsDispatcher()
    is intended to pass events from network thread to any other thread.

    Using this dispatcher makes sense only when running network on a separate thread.
    In this case, there might be cases when user wants to get callback that is invoked
    in network thread to be executed on user logic thread (e.g. on main thread).

    For example:

    \code
    Dispatcher<Function<void()>>* dispatcher = NetCore::Instance()->GetNetEventsDispatcher();

    class A
    {
        // say this function is called in network thread
        // but we want to process this callback in main thread
        void OnChannelOpen_Network()
        {
        Function<void()> f = MakeFunction(&A::OnChannelOpen_UserLogic);
        dispatcher->PostEvent(f);
        }

        // must be called in main thread
        void OnChannelOpen_UserLogic()
        {
            // user logic code
        }
    }

    // in main thread
    void OnUpdate()
    {
        // processing events accumulated by dispatcher
        if (dispatcher->HasEvents())
        {
            dispatcher->ProcessEvents(); // there callback on OnChannelOpen_UserLogic will be called
        }
    }
    \endcode

    Note that its own instances of Dispatcher<Function(void)> could be used.
    In that case, user should dispatch events from it on its own.
*/
class NetCore : public Singleton<NetCore>
{
public:
    using TrackId = uintptr_t;
    static const TrackId INVALID_TRACK_ID = 0;

    enum eDefaultPorts
    {
        DEFAULT_TCP_ANNOUNCE_PORT = 9998,
        DEFAULT_UDP_ANNOUNCE_PORT = 9998,
        DEFAULT_TCP_PORT = 9999
    };

    static const char8 defaultAnnounceMulticastGroup[];

public:
    NetCore(Engine* e);
    ~NetCore();

    IOLoop* Loop() const;

    bool RegisterService(ServiceID serviceId, ServiceCreator creator, ServiceDeleter deleter, const char8* serviceName = NULL);
    bool UnregisterService(ServiceID serviceId);
    void UnregisterAllServices();
    bool IsServiceRegistered(ServiceID serviceId) const;
    const char8* ServiceName(ServiceID serviceId) const;

    TrackId CreateController(const NetConfig& config, void* context = nullptr, uint32 readTimeout = DEFAULT_READ_TIMEOUT);
    IController::Status GetControllerStatus(TrackId) const;

    TrackId CreateAnnouncer(const Endpoint& endpoint, uint32 sendPeriod, Function<size_t(size_t, void*)> needDataCallback, const Endpoint& tcpEndpoint = Endpoint(DEFAULT_TCP_ANNOUNCE_PORT));
    TrackId CreateDiscoverer(const Endpoint& endpoint, Function<void(size_t, const void*, const Endpoint&)> dataReadyCallback);

    /** destroy controller with given `id` and invoke `callback` on destroy. Callback may be invoked synchronously*/
    void DestroyController(TrackId id, Function<void()> callback = nullptr);
    DAVA_DEPRECATED(void DestroyControllerBlocked(TrackId id)); // blocked functions are deprecated for use
    void DestroyAllControllers(Function<void()> callback);
    DAVA_DEPRECATED(void DestroyAllControllersBlocked()); // blocked functions are deprecated for use

    void RestartAllControllers();

    size_t ControllersCount() const;

    int32 Run();
    void Poll(float32 frameDelta = 0.0f);
    void Finish(bool runOutLoop = false);

    enum DiscoverStartResult
    {
        CONTROLLER_NOT_CREATED,
        CONTROLLER_NOT_STARTED_YET,
        DISCOVER_STARTED,
        CLOSING_PREVIOUS_DISCOVER
    };
    DiscoverStartResult TryDiscoverDevice(const Endpoint& endpoint);

    Vector<IfAddress> InstalledInterfaces() const;

    static bool IsNetworkEnabled();

    Dispatcher<Function<void()>>* GetNetEventsDispatcher();

    void Update(float32 frameDelta = 0.0f);

private:
    void ProcessPendingEvents();

    TrackId StartController(std::unique_ptr<IController> controller);

    void NetThreadHandler();
    void DoStart(IController* ctrl);
    void DoRestart();

    bool PostAllToDestroy();
    void WaitForAllDestroyed();
    void WaitForDestroyed(TrackId);
    void DoDestroy(IController* ctrl);
    void AllDestroyed();

    void TrackedObjectStopped(IController* obj);

    TrackId ObjectToTrackId(const IController* obj) const;
    IController* TrackIdToObject(TrackId id) const;

private:
    Engine* engine = nullptr;
    IOLoop* loop = nullptr; // Heart of NetCore and network library - event loop
    bool useSeparateThread = false;

    struct ControllerContext
    {
        enum Status
        {
            STARTING,
            RUNNING,
            DESTROYING
        } status = STARTING;
        std::unique_ptr<IController> ctrl;
        Function<void()> controllerStoppedCallback;
    };

    mutable Mutex controllersMutex;
    UnorderedMap<TrackId, ControllerContext> controllers;

    ServiceRegistrar registrar; //-V730_NOINIT
    Function<void()> allControllersStoppedCallback;

    enum State
    {
        ACTIVE,
        FINISHING,
        FINISHED
    };
    Atomic<State> state{ State::ACTIVE };

#if !defined(DAVA_NETWORK_DISABLE)
    TrackId discovererId = INVALID_TRACK_ID;
#endif
    RefPtr<Thread> netThread;
    std::unique_ptr<Dispatcher<Function<void()>>> netEventsDispatcher;

    ManualResetEvent loopCreatedEvent; // using for waiting of separate thread starup
};

//////////////////////////////////////////////////////////////////////////
inline bool NetCore::RegisterService(ServiceID serviceId, ServiceCreator creator, ServiceDeleter deleter, const char8* serviceName)
{
    return registrar.Register(serviceId, creator, deleter, serviceName);
}

inline bool NetCore::UnregisterService(ServiceID serviceId)
{
    return registrar.UnRegister(serviceId);
}

inline void NetCore::UnregisterAllServices()
{
    registrar.UnregisterAll();
}

inline bool NetCore::IsServiceRegistered(ServiceID serviceId) const
{
    return registrar.IsRegistered(serviceId);
}

inline const char8* NetCore::ServiceName(ServiceID serviceId) const
{
    return registrar.Name(serviceId);
}

inline Vector<IfAddress> NetCore::InstalledInterfaces() const
{
    return IfAddress::GetInstalledInterfaces(false);
}

inline int32 NetCore::Run()
{
    return loop->Run(IOLoop::RUN_DEFAULT);
}

inline void NetCore::Poll(float32 /*frameDelta*/)
{
    loop->Run(IOLoop::RUN_NOWAIT);
}

inline bool NetCore::IsNetworkEnabled()
{
#if defined(DAVA_NETWORK_DISABLE)
    return false;
#else
    return true;
#endif
}

inline NetCore::TrackId NetCore::ObjectToTrackId(const IController* obj) const
{
    return reinterpret_cast<TrackId>(obj);
}

inline IController* NetCore::TrackIdToObject(TrackId id) const
{
    return reinterpret_cast<IController*>(id);
}

inline IOLoop* NetCore::Loop() const
{
    return loop;
}

} // namespace Net
} // namespace DAVA
