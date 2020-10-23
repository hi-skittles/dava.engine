#include "ServerCore.h"
#include "Platform/DeviceInfo.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"

#include <QTimer>

ServerCore::ServerCore()
    : serverProxy(DAVA::Net::NetCore::Instance()->GetNetEventsDispatcher())
    , clientProxy(DAVA::Net::NetCore::Instance()->GetNetEventsDispatcher())
    , dataBase(*this)
    , state(State::STOPPED)
    , remoteState(RemoteState::STOPPED)
{
    QObject::connect(&settings, &ApplicationSettings::SettingsUpdated, this, &ServerCore::OnSettingsUpdated);

    serverProxy.SetListener(&serverLogics);
    clientProxy.AddListener(&serverLogics);
    clientProxy.AddListener(this);

    DAVA::String serverName = DAVA::UTF8Utils::EncodeToUTF8(DAVA::DeviceInfo::GetName());
    serverLogics.Init(&serverProxy, serverName, &clientProxy, &dataBase);

    updateTimer = new QTimer(this);
    QObject::connect(updateTimer, &QTimer::timeout, this, &ServerCore::OnRefreshTimer);

    lazyUpdateTimer = new QTimer(this);
    lazyUpdateTimer->setInterval(LAZY_UPDATE_INTERVAL_MS);
    QObject::connect(lazyUpdateTimer, &QTimer::timeout, this, &ServerCore::OnLazyUpdateTimer);

    connectTimer = new QTimer(this);
    connectTimer->setInterval(CONNECT_TIMEOUT_SEC * 1000);
    connectTimer->setSingleShot(true);
    QObject::connect(connectTimer, &QTimer::timeout, this, &ServerCore::OnConnectTimeout);

    reconnectWaitTimer = new QTimer(this);
    reconnectWaitTimer->setInterval(CONNECT_REATTEMPT_WAIT_SEC * 1000);
    reconnectWaitTimer->setSingleShot(true);
    QObject::connect(reconnectWaitTimer, &QTimer::timeout, this, &ServerCore::OnReattemptTimer);

    sharedDataUpdateTimer = new QTimer(this);
    sharedDataUpdateTimer->setInterval(SHARED_UPDATE_INTERVAL_SEC * 1000);
    sharedDataUpdateTimer->setSingleShot(false);
    sharedDataUpdateTimer->start();
    QObject::connect(sharedDataUpdateTimer, &QTimer::timeout, this, &ServerCore::OnSharedDataUpdateTimer);
    QObject::connect(&sharedDataRequester, &SharedDataRequester::SharedDataReceived, this, &ServerCore::OnSharedDataReceived);
    QObject::connect(&sharedDataRequester, &SharedDataRequester::ServerShared, this, &ServerCore::OnServerShared);
    QObject::connect(&sharedDataRequester, &SharedDataRequester::ServerUnshared, this, &ServerCore::OnServerUnshared);

    settings.Load();

    settings.DumpStorageSettings();
    settings.DumpSharingSettings();

    ResetRemotesList();
    UseNextRemote();
}

ServerCore::~ServerCore()
{
    Stop();
}

void ServerCore::Start()
{
    if (state != State::STARTED)
    {
        DAVA::Logger::Debug("Server is starting");

        StartListening();

        ConnectRemote();

        updateTimer->start(UPDATE_INTERVAL_MS);
        lazyUpdateTimer->start(LAZY_UPDATE_INTERVAL_MS);

        state = State::STARTED;
        emit ServerStateChanged(this);
        DAVA::Logger::Info("Server is started");
    }
}

void ServerCore::Stop()
{
    if (state != State::STOPPED)
    {
        DAVA::Logger::Debug("Server is stopping");

        updateTimer->stop();
        lazyUpdateTimer->stop();

        StopListening();
        DisconnectRemote();

        emit ServerStateChanged(this);
        DAVA::Logger::Info("Server is stopped");
    }
}

void ServerCore::StartListening()
{
    DVASSERT(state == State::STOPPED);

    serverProxy.Listen(settings.GetPort());
    state = State::STARTED;
}

void ServerCore::StopListening()
{
    state = State::STOPPED;
    serverProxy.Disconnect();
}

bool ServerCore::ConnectRemote()
{
    DVASSERT(remoteState == RemoteState::STOPPED || remoteState == RemoteState::WAITING_REATTEMPT);

    if (!currentRemoteServer.IsEmpty())
    {
        DAVA::Logger::Debug("Connecting to remote %s:%u", currentRemoteServer.ip.c_str(), DAVA::AssetCache::ASSET_SERVER_PORT);
        clientProxy.Connect(currentRemoteServer.ip, DAVA::AssetCache::ASSET_SERVER_PORT);
        connectTimer->start();
        remoteState = RemoteState::CONNECTING;
        return true;
    }

    return false;
}

bool ServerCore::VerifyRemote()
{
    if (clientProxy.RequestServerStatus())
    {
        connectTimer->start();
        remoteState = RemoteState::VERIFYING;
        return true;
    }

    return false;
}

void ServerCore::DisconnectRemote()
{
    connectTimer->stop();
    reconnectWaitTimer->stop();
    remoteState = RemoteState::STOPPED;
    serverLogics.OnRemoteDisconnecting();
    clientProxy.Disconnect();
}

void ServerCore::ReconnectRemoteLater()
{
    DisconnectRemote();
    UseNextRemote();
    remoteState = RemoteState::WAITING_REATTEMPT;
    reconnectWaitTimer->start();
}

void ServerCore::UseNextRemote()
{
    if (!remoteServerIndexIsValid)
    {
        if (remoteServers.empty() == false)
        {
            remoteServerIndexIsValid = true;
            remoteServerIndex = 0;
            currentRemoteServer = remoteServers.front();
        }
        else
        {
            currentRemoteServer.Clear();
        }
    }
    else
    {
        DVASSERT(remoteServerIndex < remoteServers.size());
        if (++remoteServerIndex == remoteServers.size())
            remoteServerIndex = 0;
        currentRemoteServer = remoteServers[remoteServerIndex];
    }
}

void ServerCore::ResetRemotesList()
{
    DAVA::List<RemoteServerParams> updatedRemotesList = settings.GetEnabledRemoteServers();

    CompareResult compareResult = CompareWithRemoteList(updatedRemotesList);
    if (!compareResult.listsAreTotallyEqual)
    {
        remoteServers.assign(std::make_move_iterator(std::begin(updatedRemotesList)), std::make_move_iterator(std::end(updatedRemotesList)));
        if (!compareResult.listsAreEqualAtLeastTillCurrentIndex)
            remoteServerIndexIsValid = false;
    }
}

ServerCore::CompareResult ServerCore::CompareWithRemoteList(const DAVA::List<RemoteServerParams>& updatedRemotesList)
{
    CompareResult result;

    auto ourIter = remoteServers.begin();
    auto ourEnd = remoteServers.end();
    auto updatedIter = updatedRemotesList.begin();
    auto updatedEnd = updatedRemotesList.end();
    DAVA::uint32 index = 0;

    for (; ourIter != ourEnd && updatedIter != updatedEnd; ++ourIter, ++updatedIter, ++index)
    {
        if (*ourIter == *updatedIter)
        {
            if (index == remoteServerIndex)
            {
                result.listsAreEqualAtLeastTillCurrentIndex = true;
            }
        }
        else
        {
            break;
        }
    }

    result.listsAreTotallyEqual = (ourIter == ourEnd && updatedIter == updatedEnd);
    return result;
}

void ServerCore::OnRefreshTimer()
{
    serverLogics.Update();
    DAVA::Net::NetCore::Instance()->Update();
}

void ServerCore::OnLazyUpdateTimer()
{
    serverLogics.LazyUpdate();
}

void ServerCore::OnConnectTimeout()
{
    ReconnectRemoteLater();
    emit ServerStateChanged(this);
}

void ServerCore::OnReattemptTimer()
{
    ConnectRemote();
    emit ServerStateChanged(this);
}

void ServerCore::OnSharedDataUpdateTimer()
{
    sharedDataRequester.RequestSharedData(settings.GetOwnID());
}

void ServerCore::OnClientProxyStateChanged()
{
    if (clientProxy.ChannelIsOpened())
    {
        switch (remoteState)
        {
        case RemoteState::STOPPED:
        case RemoteState::CONNECTING:
        {
            connectTimer->stop();
            reconnectWaitTimer->stop();
            VerifyRemote();
            break;
        }
        default:
        {
            DVASSERT(false, DAVA::Format("Remote is opened, but remoteState = %u", remoteState.load()).c_str());
            break;
        }
        }
    }
    else
    {
        DVASSERT(remoteState != RemoteState::STOPPED);
        ReconnectNow();
    }

    emit ServerStateChanged(this);
}

void ServerCore::ReconnectAsynchronously()
{
    QTimer::singleShot(0, this, &ServerCore::ReconnectNow);
}

void ServerCore::ReconnectNow()
{
    DisconnectRemote();
    UseNextRemote();
    ConnectRemote();
}

void ServerCore::OnServerStatusReceived()
{
    DVASSERT(remoteState == RemoteState::VERIFYING);

    connectTimer->stop();
    reconnectWaitTimer->stop();
    remoteState = RemoteState::STARTED;

    emit ServerStateChanged(this);
}

void ServerCore::OnIncorrectPacketReceived(DAVA::AssetCache::IncorrectPacketType)
{
    DVASSERT(remoteState != RemoteState::STOPPED);
    ReconnectRemoteLater();
    emit ServerStateChanged(this);
}

void ServerCore::OnSettingsUpdated(const ApplicationSettings* _settings)
{
    DVASSERT(&settings == _settings);

    auto& folder = settings.GetFolder();
    auto sizeGb = settings.GetCacheSizeGb();
    auto count = settings.GetFilesCount();
    auto autoSaveTimeoutMin = settings.GetAutoSaveTimeoutMin();

    bool remoteChanged = false;

    ResetRemotesList();
    auto found = std::find(remoteServers.begin(), remoteServers.end(), currentRemoteServer);
    if (found == remoteServers.end())
    {
        remoteChanged = true;
        UseNextRemote();
    }

    bool needServerRestart = false;
    bool needClientRestart = false;

    if (state == State::STARTED)
    { // disconnect network if settings changed
        if (serverProxy.GetListenPort() != settings.GetPort())
        {
            needServerRestart = true;
            StopListening();
        }

        if ((remoteState != RemoteState::STOPPED && remoteChanged) ||
            (remoteState == RemoteState::STOPPED && !currentRemoteServer.IsEmpty()))
        {
            needClientRestart = true;
            DisconnectRemote();
        }
    }

    //updated DB settings
    if (sizeGb && !folder.IsEmpty())
    {
        dataBase.UpdateSettings(folder, sizeGb * 1024 * 1024 * 1024, count, autoSaveTimeoutMin * 60 * 1000);
    }
    else
    {
        DAVA::Logger::Warning("[ServerCore::%s] Empty settings", __FUNCTION__);
    }

    // restart network connections after changing of settings
    if (needServerRestart)
    {
        StartListening();
    }

    if (state == State::STARTED && needClientRestart)
    {
        ConnectRemote();
    }

    if (needServerRestart || needClientRestart)
    {
        emit ServerStateChanged(this);
        return;
    }
}

void ServerCore::InitiateShareRequest(PoolID poolID, const DAVA::String& serverName)
{
    SharedServerParams serverParams;
    serverParams.poolID = poolID;
    serverParams.name = serverName;
    serverParams.port = settings.GetPort();
    sharedDataRequester.AddSharedServer(serverParams, appPath);
}

void ServerCore::InitiateUnshareRequest()
{
    sharedDataRequester.RemoveSharedServer(settings.GetOwnID());
}

void ServerCore::OnServerShared(PoolID poolID, ServerID serverID, const DAVA::String& serverName)
{
    settings.SetOwnPoolID(poolID);
    settings.SetOwnID(serverID);
    settings.SetOwnName(serverName);
    settings.SetSharedForOthers(true);
    settings.Save();
    settings.DumpSharingSettings();
    emit ServerShared();
}

void ServerCore::OnServerUnshared()
{
    settings.SetOwnID(NullServerID);
    settings.SetSharedForOthers(false);
    settings.Save();
    settings.DumpSharingSettings();
    emit ServerUnshared();
}

void ServerCore::OnSharedDataReceived(const DAVA::List<SharedPoolParams>& pools, const DAVA::List<SharedServerParams>& servers)
{
    settings.UpdateSharedPools(pools, servers);
    settings.Save();
    emit SharedDataUpdated();
}

void ServerCore::ClearStorage()
{
    DAVA::Logger::Info("Manual storage clearing");
    dataBase.ClearStorage();
}

void ServerCore::GetStorageSpaceUsage(DAVA::uint64& occupied, DAVA::uint64& overall) const
{
    occupied = dataBase.GetOccupiedSize();
    overall = dataBase.GetStorageSize();
}

void ServerCore::OnStorageSizeChanged(DAVA::uint64 occupied, DAVA::uint64 overall)
{
    emit StorageSizeChanged(occupied, overall);
}

void ServerCore::SetApplicationPath(const DAVA::String& path)
{
    appPath = path;
}
