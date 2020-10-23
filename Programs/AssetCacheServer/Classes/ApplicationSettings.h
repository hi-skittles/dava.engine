#pragma once

#include <AssetCache/AssetCacheConstants.h>

#include "FileSystem/FilePath.h"
#include "Network/Base/Endpoint.h"
#include <QObject>

namespace DAVA
{
class KeyedArchive;
};

struct RemoteServerParams
{
    RemoteServerParams() = default;
    RemoteServerParams(DAVA::String _ip, bool _enabled);

    void Clear();
    bool IsEmpty() const;

    bool operator==(const RemoteServerParams& right) const;

    DAVA::String ip = "";
    bool enabled = false;
};

using ServerID = DAVA::uint64;
using PoolID = DAVA::uint64;

const ServerID NullServerID = 0;
const PoolID NullPoolID = 0;

struct SharedPoolParams
{
    PoolID poolID;
    DAVA::String name;
    DAVA::String description;
};

struct SharedServerParams
{
    ServerID serverID;
    PoolID poolID;
    DAVA::String ip;
    DAVA::uint16 port;
    DAVA::String name;
};

struct SharedServer
{
    ServerID serverID = NullServerID;
    PoolID poolID = NullPoolID;
    DAVA::String serverName;
    RemoteServerParams remoteParams;
};

struct SharedPool
{
    PoolID poolID = NullPoolID;
    DAVA::String poolName;
    DAVA::String poolDescription;
    bool enabled = false;
    DAVA::Map<ServerID, SharedServer> servers;
};

struct EnabledRemote
{
    enum Type
    {
        POOL,
        POOL_SERVER,
        CUSTOM_SERVER,
        NONE
    } type = NONE;
    union
    {
        SharedPool* pool = nullptr;
        SharedServer* server;
        RemoteServerParams* customServer;
    };

    EnabledRemote() = default;
    EnabledRemote(SharedPool* pool)
        : type(POOL)
        , pool(pool)
    {
    }
    EnabledRemote(SharedServer* server)
        : type(POOL_SERVER)
        , server(server)
    {
    }
    EnabledRemote(RemoteServerParams* server)
        : type(CUSTOM_SERVER)
        , customServer(server)
    {
    }
};

//TODO: we need one code for settings in different projects
//need to use introspection instead of hardcoding of values

class ApplicationSettings : public QObject
{
    Q_OBJECT

private:
    static const DAVA::String DEFAULT_FOLDER;
    static const DAVA::float64 DEFAULT_CACHE_SIZE_GB;
    static const DAVA::uint32 DEFAULT_FILES_COUNT;
    static const DAVA::uint32 DEFAULT_AUTO_SAVE_TIMEOUT_MIN;
    static const DAVA::uint16 DEFAULT_PORT;
    static const DAVA::uint16 DEFAULT_HTTP_PORT;
    static const bool DEFAULT_AUTO_START;
    static const bool DEFAULT_LAUNCH_ON_SYSTEM_STARTUP;
    static const bool DEFAULT_RESTART_ON_CRASH;
    static const bool DEFAULT_SHARED_FOR_OTHERS;

public:
    void Save() const;
    void Load();

    void LoadFromOldPath(); // todo: remove some versions later

    bool IsFirstLaunch() const;

    static DAVA::FilePath GetDefaultFolder();

    const DAVA::FilePath& GetFolder() const;
    void SetFolder(const DAVA::FilePath& folder);

    const DAVA::float64 GetCacheSizeGb() const;
    void SetCacheSizeGb(const DAVA::float64 size);

    const DAVA::uint32 GetFilesCount() const;
    void SetFilesCount(const DAVA::uint32 count);

    const DAVA::uint64 GetAutoSaveTimeoutMin() const;
    void SetAutoSaveTimeoutMin(const DAVA::uint64 timeout);

    const DAVA::uint16 GetPort() const;
    void SetPort(const DAVA::uint16 port);

    const DAVA::uint16 GetHttpPort() const;
    void SetHttpPort(const DAVA::uint16 port);

    const bool IsAutoStart() const;
    void SetAutoStart(bool);

    const bool IsLaunchOnSystemStartup() const;
    void SetLaunchOnSystemStartup(bool);

    const bool IsRestartOnCrash() const;
    void SetRestartOnCrash(bool);

    bool IsSharedForOthers() const;
    void SetSharedForOthers(bool);

    ServerID GetOwnID() const;
    void SetOwnID(ServerID);
    void ResetOwnID();

    void SetOwnPoolID(PoolID);
    PoolID GetOwnPoolID() const;

    void SetOwnName(DAVA::String);
    const DAVA::String& GetOwnName() const;

    void UpdateSharedPools(const DAVA::List<SharedPoolParams>& pools, const DAVA::List<SharedServerParams>& servers);
    const DAVA::Map<PoolID, SharedPool>& GetSharedPools() const;
    void EnableSharedPool(PoolID poolID);
    void EnableSharedServer(PoolID poolID, ServerID serverID);

    const DAVA::List<RemoteServerParams>& GetCustomServers() const;
    void AddCustomServer(const RemoteServerParams& server);
    void RemoveCustomServer(const RemoteServerParams& server);
    void ClearCustomServers();

    DAVA::List<RemoteServerParams> GetEnabledRemoteServers();
    EnabledRemote GetEnabledRemote();
    void DisableRemote();

    void DumpStorageSettings();
    void DumpSharingSettings();

signals:
    void SettingsUpdated(const ApplicationSettings* settings) const;

private:
    void Serialize(DAVA::KeyedArchive* archieve) const;
    void Deserialize(DAVA::KeyedArchive* archieve);

public:
    DAVA::FilePath folder = DEFAULT_FOLDER;
    DAVA::float64 cacheSizeGb = DEFAULT_CACHE_SIZE_GB;
    DAVA::uint32 filesCount = DEFAULT_FILES_COUNT;
    DAVA::uint32 autoSaveTimeoutMin = DEFAULT_AUTO_SAVE_TIMEOUT_MIN;
    DAVA::uint16 listenPort = DEFAULT_PORT;
    DAVA::uint16 listenHttpPort = DEFAULT_HTTP_PORT;
    bool autoStart = DEFAULT_AUTO_START;
    bool launchOnSystemStartup = DEFAULT_LAUNCH_ON_SYSTEM_STARTUP;
    bool restartOnCrash = DEFAULT_RESTART_ON_CRASH;
    bool sharedForOthers = DEFAULT_SHARED_FOR_OTHERS;
    ServerID ownID = 0;
    PoolID ownPoolID = 0;
    DAVA::String ownName;

    DAVA::Map<PoolID, SharedPool> sharedPools;
    DAVA::List<RemoteServerParams> customServers;

    bool isFirstLaunch = true;
};

inline bool ApplicationSettings::IsFirstLaunch() const
{
    return isFirstLaunch;
}
