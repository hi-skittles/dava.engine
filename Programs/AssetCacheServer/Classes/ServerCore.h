#pragma once

#include <AssetCache/ServerNetProxy.h>
#include <AssetCache/ClientNetProxy.h>

#include "ServerLogics.h"
#include "ApplicationSettings.h"
#include "SharedDataRequester.h"

#include <atomic>
#include <QObject>

class QTimer;

class ServerCore : public QObject,
                   public DAVA::AssetCache::ClientNetProxyListener,
                   public CacheDBOwner
{
    Q_OBJECT

    static const DAVA::uint32 UPDATE_INTERVAL_MS = 16;
    static const DAVA::uint32 LAZY_UPDATE_INTERVAL_MS = 500;
    static const DAVA::uint32 CONNECT_TIMEOUT_SEC = 1;
    static const DAVA::uint32 CONNECT_REATTEMPT_WAIT_SEC = 5;
    static const DAVA::uint32 SHARED_UPDATE_INTERVAL_SEC = 3;

public:
    enum class State
    {
        STARTED,
        STOPPED
    };
    enum class RemoteState
    {
        STARTED,
        STOPPED,
        CONNECTING,
        VERIFYING,
        WAITING_REATTEMPT
    };

    ServerCore();
    ~ServerCore() override;

    ApplicationSettings& Settings();

    void Start();
    void Stop();

    State GetState() const;
    RemoteState GetRemoteState() const;

    void InitiateShareRequest(PoolID poolID, const DAVA::String& serverName);
    void InitiateUnshareRequest();

    void SetApplicationPath(const DAVA::String& path);

    void ClearStorage();
    void GetStorageSpaceUsage(DAVA::uint64& occupied, DAVA::uint64& overall) const;

    // ClientNetProxyListener
    void OnClientProxyStateChanged() override;
    void OnServerStatusReceived() override;
    void OnIncorrectPacketReceived(DAVA::AssetCache::IncorrectPacketType) override;

    // CacheDBOwner
    void OnStorageSizeChanged(DAVA::uint64 occupied, DAVA::uint64 overall) override;

signals:
    void ServerStateChanged(const ServerCore* serverCore) const;
    void StorageSizeChanged(DAVA::uint64 occupied, DAVA::uint64 overall) const;
    void ServerShared();
    void ServerUnshared();
    void SharedDataUpdated();

private slots:
    void OnServerShared(PoolID poolID, ServerID serverID, const DAVA::String& serverName);
    void OnServerUnshared();
    void OnSharedDataReceived(const DAVA::List<SharedPoolParams>& pools, const DAVA::List<SharedServerParams>& servers);
    void OnSettingsUpdated(const ApplicationSettings* settings);
    void OnRefreshTimer();
    void OnLazyUpdateTimer();
    void OnConnectTimeout();
    void OnReattemptTimer();
    void ReconnectAsynchronously();
    void ReconnectNow();
    void OnSharedDataUpdateTimer();

private:
    void StartListening();
    void StopListening();

    bool ConnectRemote();
    bool VerifyRemote();
    void DisconnectRemote();
    void ReconnectRemoteLater();
    void UseNextRemote();
    void ResetRemotesList();

    struct CompareResult
    {
        bool listsAreTotallyEqual = false;
        bool listsAreEqualAtLeastTillCurrentIndex = false;
    };
    CompareResult CompareWithRemoteList(const DAVA::List<RemoteServerParams>& updatedRemotesList);

private:
    DAVA::AssetCache::ServerNetProxy serverProxy;
    DAVA::AssetCache::ClientNetProxy clientProxy;
    CacheDB dataBase;

    ServerLogics serverLogics;
    ApplicationSettings settings;

    std::atomic<State> state;
    std::atomic<RemoteState> remoteState;

    DAVA::String appPath;

    DAVA::Vector<RemoteServerParams> remoteServers;
    size_t remoteServerIndex = 0;
    bool remoteServerIndexIsValid = false;
    RemoteServerParams currentRemoteServer;

    SharedDataRequester sharedDataRequester;

    QTimer* updateTimer = nullptr;
    QTimer* lazyUpdateTimer = nullptr;
    QTimer* connectTimer = nullptr;
    QTimer* reconnectWaitTimer = nullptr;
    QTimer* sharedDataUpdateTimer = nullptr;
};

inline ServerCore::State ServerCore::GetState() const
{
    return state;
}

inline ServerCore::RemoteState ServerCore::GetRemoteState() const
{
    return remoteState;
}

inline ApplicationSettings& ServerCore::Settings()
{
    return settings;
}
