#include "Network/NetCore.h"
#include "Network/NetConfig.h"
#include "Network/Private/NetController.h"
#include "Network/Private/Announcer.h"
#include "Network/Private/Discoverer.h"
#include "Functional/Function.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Base/BaseTypes.h"
#include "FileSystem/KeyedArchive.h"
#include "Concurrency/LockGuard.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Net
{
const char8 NetCore::defaultAnnounceMulticastGroup[] = "239.192.100.1";

NetCore::NetCore(Engine* e)
    : engine(e)
    , loopCreatedEvent(false)
{
    bool separateThreadDefaultValue = false;
    const KeyedArchive* options = e->GetOptions();
    useSeparateThread = options->GetBool("separate_net_thread", separateThreadDefaultValue);

    e->update.Connect(this, &NetCore::Update);

    netEventsDispatcher.reset(new Dispatcher<Function<void()>>([](const Function<void()>& fn) { fn(); }));
    netEventsDispatcher->LinkToCurrentThread();

    if (useSeparateThread)
    {
        netThread = Thread::Create([this]() { NetThreadHandler(); });
        netThread->Start();
        loopCreatedEvent.Wait();
    }
    else
    {
        loop = new IOLoop(true);
    }

#if defined(__DAVAENGINE_IPHONE__)
    // iOS silently kills sockets when device is locked so recreate sockets
    // when application is resumed
    e->resumed.Connect(this, &NetCore::RestartAllControllers);
#endif
}

NetCore::~NetCore()
{
    engine->update.Disconnect(this);
#if defined(__DAVAENGINE_IPHONE__)
    engine->resumed.Disconnect(this);
#endif

    Finish(true);

    DVASSERT(state == State::FINISHED);
    if (netThread)
    {
        DVASSERT(netThread->GetState() == Thread::eThreadState::STATE_ENDED);
    }

    DVASSERT(true == controllers.empty());

    if (!useSeparateThread)
    {
        SafeDelete(loop);
    }
}

void NetCore::NetThreadHandler()
{
    loop = new IOLoop(true);
    loopCreatedEvent.Signal();
    loop->Run();
    SafeDelete(loop);
}

void NetCore::Update(float32)
{
    ProcessPendingEvents();
    if (!useSeparateThread)
    {
        Poll();
    }
}

void NetCore::ProcessPendingEvents()
{
    if (netEventsDispatcher->HasEvents())
    {
        netEventsDispatcher->ProcessEvents();
    }
}

NetCore::TrackId NetCore::StartController(std::unique_ptr<IController> ctrl)
{
    IController* c = ctrl.get();
    TrackId id = ObjectToTrackId(c);
    {
        LockGuard<Mutex> lock(controllersMutex);
        ControllerContext& context = controllers[id];

        context.status = ControllerContext::STARTING;
        context.ctrl = std::move(ctrl);
    }
    loop->Post(Bind(&NetCore::DoStart, this, c));
    return id;
}

Dispatcher<Function<void()>>* NetCore::GetNetEventsDispatcher()
{
    return netEventsDispatcher.get();
}

NetCore::TrackId NetCore::CreateController(const NetConfig& config, void* context, uint32 readTimeout)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE && true == config.Validate());

    std::unique_ptr<NetController> ctrl = std::make_unique<NetController>(loop, registrar, context, readTimeout);

    if (true == ctrl->ApplyConfig(config))
    {
        return StartController(std::move(ctrl));
    }
    else
    {
        return INVALID_TRACK_ID;
    }
#else
    return INVALID_TRACK_ID;
#endif
}

IController::Status NetCore::GetControllerStatus(TrackId id) const
{
    IController* ctrl = TrackIdToObject(id);
    if (ctrl)
    {
        return ctrl->GetStatus();
    }
    else
    {
        DVASSERT(false, Format("Wrong track id: %u", id).c_str());
        return IController::START_FAILED;
    }
}

NetCore::TrackId NetCore::CreateAnnouncer(const Endpoint& endpoint, uint32 sendPeriod, Function<size_t(size_t, void*)> needDataCallback, const Endpoint& tcpEndpoint)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);
    std::unique_ptr<Announcer> ctrl = std::make_unique<Announcer>(loop, endpoint, sendPeriod, needDataCallback, tcpEndpoint);
    return StartController(std::move(ctrl));
#else
    return INVALID_TRACK_ID;
#endif
}

NetCore::TrackId NetCore::CreateDiscoverer(const Endpoint& endpoint, Function<void(size_t, const void*, const Endpoint&)> dataReadyCallback)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);
    std::unique_ptr<Discoverer> ctrl = std::make_unique<Discoverer>(loop, endpoint, dataReadyCallback);
    discovererId = StartController(std::move(ctrl));
    return discovererId;
#else
    return INVALID_TRACK_ID;
#endif
}

void NetCore::DestroyController(TrackId id, Function<void()> callback)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);

    if (id == discovererId)
    {
        discovererId = INVALID_TRACK_ID;
    }

    bool isStartingController = false;

    {
        LockGuard<Mutex> lock(controllersMutex);
        auto it = controllers.find(id);
        if (it != controllers.end())
        {
            ControllerContext& context = it->second;
            if (context.status == ControllerContext::STARTING)
            {
                isStartingController = true;
            }
            else if (context.status == ControllerContext::RUNNING)
            {
                context.status = ControllerContext::DESTROYING;
                context.controllerStoppedCallback = callback;
                loop->Post(Bind(&NetCore::DoDestroy, this, context.ctrl.get()));
            }
        }
        else
        {
            DVASSERT(false, "passed controller is not found");
        }
    }

    if (isStartingController)
    {
        if (callback)
        {
            callback();
        }

        LockGuard<Mutex> lock(controllersMutex);
        controllers.erase(id);
    }
#endif
}

void NetCore::DestroyControllerBlocked(TrackId id)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);

    bool toWait = false;
    {
        LockGuard<Mutex> lock(controllersMutex);
        auto it = controllers.find(id);
        if (it != controllers.end())
        {
            ControllerContext& context = it->second;
            if (context.status == ControllerContext::STARTING)
            {
                controllers.erase(it);
            }
            else
            {
                toWait = true;
                if (context.status == ControllerContext::RUNNING)
                {
                    context.status = ControllerContext::DESTROYING;
                    loop->Post(Bind(&NetCore::DoDestroy, this, context.ctrl.get()));
                }
            }
        }
        else
        {
            DVASSERT(false, "passed controller is not found");
            return;
        }
    }

    if (toWait)
    {
        WaitForDestroyed(id);
    }
#endif
}

void NetCore::WaitForDestroyed(TrackId id)
{
    while (true)
    {
        {
            LockGuard<Mutex> lock(controllersMutex);
            if (controllers.find(id) == controllers.end())
            {
                break;
            }
        }

        Update();
    }
}

bool NetCore::PostAllToDestroy()
{
    LockGuard<Mutex> lock(controllersMutex);

    bool hasControllersToDestroy = false;
    for (auto it = controllers.begin(); it != controllers.end();)
    {
        ControllerContext& context = it->second;
        if (context.status == ControllerContext::STARTING)
        {
            auto itDel = it++;
            controllers.erase(it);
        }
        else
        {
            ++it;
            if (context.status == ControllerContext::RUNNING)
            {
                context.status = ControllerContext::DESTROYING;
                loop->Post(Bind(&NetCore::DoDestroy, this, context.ctrl.get()));
            }
            hasControllersToDestroy = true;
        }
    }

    return hasControllersToDestroy;
}

void NetCore::DestroyAllControllers(Function<void()> callback)
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);
    DVASSERT(allControllersStoppedCallback == nullptr);

    allControllersStoppedCallback = callback;
    PostAllToDestroy();
#endif
}

void NetCore::DestroyAllControllersBlocked()
{
#if !defined(DAVA_NETWORK_DISABLE)
    DVASSERT(state == State::ACTIVE);
    DVASSERT(allControllersStoppedCallback == nullptr);

    PostAllToDestroy();
    WaitForAllDestroyed();
#endif
}

void NetCore::WaitForAllDestroyed()
{
    while (true)
    {
        {
            LockGuard<Mutex> lock(controllersMutex);
            if (controllers.empty())
                break;
        }

        Update();
    }
}

void NetCore::RestartAllControllers()
{
#if !defined(DAVA_NETWORK_DISABLE)
    // Restart controllers on mobile devices
    loop->Post(MakeFunction(this, &NetCore::DoRestart));
#endif
}

void NetCore::Finish(bool waitForFinished)
{
#if !defined(DAVA_NETWORK_DISABLE)

    if (state == State::ACTIVE)
    {
        state = State::FINISHING;
        bool hasControllersToDestroy = PostAllToDestroy();

        if (!hasControllersToDestroy)
        {
            AllDestroyed();
        }

        if (waitForFinished)
        {
            if (hasControllersToDestroy)
            {
                WaitForAllDestroyed();
            }

            if (useSeparateThread)
            {
                netThread->Join();
            }
            else
            {
                loop->Run(IOLoop::RUN_DEFAULT);
            }
        }
    }
#endif
}

NetCore::DiscoverStartResult NetCore::TryDiscoverDevice(const Endpoint& endpoint)
{
#if !defined(DAVA_NETWORK_DISABLE)
    if (discovererId != INVALID_TRACK_ID)
    {
        LockGuard<Mutex> lock(controllersMutex);
        auto it = controllers.find(discovererId);
        if (it != controllers.end() && it->second.status == ControllerContext::RUNNING)
        {
            IController* ctrl = it->second.ctrl.get();
            // Variable is named in honor of big fan and donater of tanks - Sergey Demidov
            // And this man assures that cast below is valid, so do not worry, guys
            Discoverer* SergeyDemidov = static_cast<Discoverer*>(ctrl);
            return (SergeyDemidov->TryDiscoverDevice(endpoint) ? DISCOVER_STARTED : CLOSING_PREVIOUS_DISCOVER);
        }
        else
        {
            return CONTROLLER_NOT_STARTED_YET;
        }
    }
    else
    {
        return CONTROLLER_NOT_CREATED;
    }    
#else
    return DISCOVER_STARTED;
#endif
}

void NetCore::DoStart(IController* ctrl)
{
    bool toStart = false;
    {
        LockGuard<Mutex> lock(controllersMutex);
        auto it = controllers.find(ObjectToTrackId(ctrl));
        if (it != controllers.end())
        {
            if (it->second.status == ControllerContext::STARTING)
            {
                toStart = true;
                it->second.status = ControllerContext::RUNNING;
            }
            else
            {
                DVASSERT("Controller status is not STARTING");
                return;
            }
        }
    }

    if (toStart)
    {
        ctrl->Start();
    }
}

void NetCore::DoRestart()
{
    LockGuard<Mutex> lock(controllersMutex);
    for (auto& entry : controllers)
    {
        if (entry.second.status == ControllerContext::RUNNING)
            entry.second.ctrl->Restart();
    }
}

void NetCore::DoDestroy(IController* ctrl)
{
    DVASSERT(ctrl != nullptr);
    ctrl->Stop(Bind(&NetCore::TrackedObjectStopped, this, _1));
}

void NetCore::AllDestroyed()
{
    if (allControllersStoppedCallback != nullptr)
    {
        allControllersStoppedCallback();
        allControllersStoppedCallback = nullptr;
    }
    if (state == State::FINISHING)
    {
        state = State::FINISHED;
        loop->PostQuit();
    }
}

void NetCore::TrackedObjectStopped(IController* obj)
{
    Function<void()> callbackOnStopped;
    bool allDestroyed = false;

    {
        LockGuard<Mutex> lock(controllersMutex);
        auto it = controllers.find(ObjectToTrackId(obj));
        if (it != controllers.end())
        {
            DVASSERT(it->second.status == ControllerContext::DESTROYING);
            callbackOnStopped = it->second.controllerStoppedCallback;

            TrackId id = it->first;
            if (id == discovererId)
            {
                discovererId = INVALID_TRACK_ID;
            }

            controllers.erase(it);
        }
        else
        {
            DVASSERT(false && "stopped controller is not found in map");
        }

        allDestroyed = (true == controllers.empty());
    }

    if (callbackOnStopped)
    {
        callbackOnStopped();
    }

    if (allDestroyed)
    {
        AllDestroyed();
    }
}

size_t NetCore::ControllersCount() const
{
    LockGuard<Mutex> lock(controllersMutex);
    return controllers.size();
}

} // namespace Net
} // namespace DAVA
