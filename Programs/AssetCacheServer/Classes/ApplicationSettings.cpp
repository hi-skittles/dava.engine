#include "ApplicationSettings.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/KeyedArchive.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"

RemoteServerParams::RemoteServerParams(DAVA::String _ip, bool _enabled)
    : ip(_ip)
    , enabled(_enabled)
{
}

void RemoteServerParams::Clear()
{
    ip.clear();
}

bool RemoteServerParams::IsEmpty() const
{
    return ip.empty();
}

bool RemoteServerParams::operator==(const RemoteServerParams& right) const
{
    return (ip == right.ip);
}

namespace ApplicationSettingsDetails
{
const DAVA::String SETTINGS_PATH_OLD = "~doc:/AssetServer/ACS_settings.dat";
const DAVA::String SETTINGS_PATH = "~doc:/ACS_settings.dat";
}

const DAVA::String ApplicationSettings::DEFAULT_FOLDER = "~doc:/DefaultStorage";
const DAVA::float64 ApplicationSettings::DEFAULT_CACHE_SIZE_GB = 5.0;
const DAVA::uint32 ApplicationSettings::DEFAULT_FILES_COUNT = 5;
const DAVA::uint32 ApplicationSettings::DEFAULT_AUTO_SAVE_TIMEOUT_MIN = 1;
const DAVA::uint16 ApplicationSettings::DEFAULT_PORT = DAVA::AssetCache::ASSET_SERVER_PORT;
const DAVA::uint16 ApplicationSettings::DEFAULT_HTTP_PORT = DAVA::AssetCache::ASSET_SERVER_HTTP_PORT;
const bool ApplicationSettings::DEFAULT_AUTO_START = true;
const bool ApplicationSettings::DEFAULT_LAUNCH_ON_SYSTEM_STARTUP = true;
const bool ApplicationSettings::DEFAULT_RESTART_ON_CRASH = false;
const bool ApplicationSettings::DEFAULT_SHARED_FOR_OTHERS = false;

void ApplicationSettings::Save() const
{
    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(ApplicationSettingsDetails::SETTINGS_PATH, DAVA::File::CREATE | DAVA::File::WRITE));
    if (!file)
    {
        DAVA::Logger::Error("[ApplicationSettings::%s] Cannot create file %s", __FUNCTION__, ApplicationSettingsDetails::SETTINGS_PATH.c_str());
        return;
    }

    DAVA::ScopedPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
    Serialize(archive);
    archive->Save(file);

    emit SettingsUpdated(this);
}

void ApplicationSettings::Load()
{
    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(ApplicationSettingsDetails::SETTINGS_PATH, DAVA::File::OPEN | DAVA::File::READ));
    if (file)
    {
        DAVA::Logger::Debug("Loading settings");
        isFirstLaunch = false;
        DAVA::ScopedPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
        archive->Load(file);
        Deserialize(archive);
    }
    else
    {
        DAVA::Logger::Debug("Settings are not found. First launch");
        isFirstLaunch = true;
    }

    emit SettingsUpdated(this);
}

void ApplicationSettings::LoadFromOldPath()
{
    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(ApplicationSettingsDetails::SETTINGS_PATH_OLD, DAVA::File::OPEN | DAVA::File::READ));
    if (file)
    {
        DAVA::Logger::Debug("Loading settings from old path");
        isFirstLaunch = false;
        DAVA::ScopedPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
        archive->Load(file);
        Deserialize(archive);
    }
    else
    {
        DAVA::Logger::Debug("Settings from old path are not found");
        isFirstLaunch = true;
    }

    emit SettingsUpdated(this);
}

void ApplicationSettings::Serialize(DAVA::KeyedArchive* archive) const
{
    DVASSERT(nullptr != archive);

    archive->SetString("FolderPath", folder.GetStringValue());
    archive->SetFloat64("FolderSize", cacheSizeGb);
    archive->SetUInt32("NumberOfFiles", filesCount);
    archive->SetUInt32("AutoSaveTimeout", autoSaveTimeoutMin);
    archive->SetBool("AutoStart", autoStart);
    archive->SetBool("SystemStartup", launchOnSystemStartup);
    archive->SetBool("Restart", restartOnCrash);

    DAVA::uint32 size = static_cast<DAVA::uint32>(customServers.size());
    archive->SetUInt32("ServersSize", size);

    DAVA::uint32 index = 0;
    for (const RemoteServerParams& pool : customServers)
    {
        archive->SetString(DAVA::Format("Server_%u_ip", index), pool.ip);
        archive->SetBool(DAVA::Format("Server_%u_enabled", index), pool.enabled);
        ++index;
    }

    DAVA::uint32 poolsSize = static_cast<DAVA::uint32>(sharedPools.size());
    archive->SetUInt32("PoolsSize", poolsSize);

    DAVA::uint32 poolIndex = 0;
    for (auto& poolEntry : sharedPools)
    {
        const SharedPool& pool = poolEntry.second;
        archive->SetUInt64(DAVA::Format("Pool_%u_ID", poolIndex), pool.poolID);
        archive->SetString(DAVA::Format("Pool_%u_name", poolIndex), pool.poolName);
        archive->SetString(DAVA::Format("Pool_%u_description", poolIndex), pool.poolDescription);
        archive->SetBool(DAVA::Format("Pool_%u_enabled", poolIndex), pool.enabled);

        DAVA::uint32 serversSize = static_cast<DAVA::uint32>(pool.servers.size());
        archive->SetUInt32(DAVA::Format("Pool_%u_ServersSize", poolIndex), serversSize);

        DAVA::uint32 serverIndex = 0;
        for (auto& serverEntry : pool.servers)
        {
            const SharedServer& server = serverEntry.second;
            archive->SetUInt64(DAVA::Format("Pool_%u_Server_%u_ID", poolIndex, serverIndex), server.serverID);
            archive->SetString(DAVA::Format("Pool_%u_Server_%u_name", poolIndex, serverIndex), server.serverName);
            archive->SetString(DAVA::Format("Pool_%u_Server_%u_ip", poolIndex, serverIndex), server.remoteParams.ip);
            archive->SetBool(DAVA::Format("Pool_%u_Server_%u_enabled", poolIndex, serverIndex), server.remoteParams.enabled);
            ++serverIndex;
        }

        ++poolIndex;
    }

    archive->SetBool("SharedForOthers", sharedForOthers);
    archive->SetUInt64("OwnPoolID", ownPoolID);
    archive->SetUInt64("OwnID", ownID);
    archive->SetString("OwnName", ownName);
}

void ApplicationSettings::Deserialize(DAVA::KeyedArchive* archive)
{
    DVASSERT(nullptr != archive);

    DVASSERT(customServers.empty());

    folder = archive->GetString("FolderPath", DEFAULT_FOLDER);
    cacheSizeGb = archive->GetFloat64("FolderSize", DEFAULT_CACHE_SIZE_GB);
    filesCount = archive->GetUInt32("NumberOfFiles", DEFAULT_FILES_COUNT);
    autoSaveTimeoutMin = archive->GetUInt32("AutoSaveTimeout", DEFAULT_AUTO_SAVE_TIMEOUT_MIN);
    autoStart = archive->GetBool("AutoStart", DEFAULT_AUTO_START);
    launchOnSystemStartup = archive->GetBool("SystemStartup", DEFAULT_LAUNCH_ON_SYSTEM_STARTUP);
    restartOnCrash = archive->GetBool("Restart", DEFAULT_RESTART_ON_CRASH);

    auto count = archive->GetUInt32("ServersSize");
    for (DAVA::uint32 poolIndex = 0; poolIndex < count; ++poolIndex)
    {
        RemoteServerParams sd;
        sd.ip = archive->GetString(DAVA::Format("Server_%u_ip", poolIndex));
        sd.enabled = archive->GetBool(DAVA::Format("Server_%u_enabled", poolIndex), false);

        customServers.push_back(sd);
    }

    DAVA::uint32 poolsSize = archive->GetUInt32("PoolsSize");

    for (DAVA::uint32 poolIndex = 0; poolIndex < poolsSize; ++poolIndex)
    {
        PoolID poolID = archive->GetUInt64(DAVA::Format("Pool_%u_ID", poolIndex), NullPoolID);
        SharedPool& pool = sharedPools[poolID];
        pool.poolID = poolID;
        pool.poolName = archive->GetString(DAVA::Format("Pool_%u_name", poolIndex));
        pool.poolDescription = archive->GetString(DAVA::Format("Pool_%u_description", poolIndex));
        pool.enabled = archive->GetBool(DAVA::Format("Pool_%u_enabled", poolIndex));

        DAVA::uint32 serversSize = archive->GetUInt32(DAVA::Format("Pool_%u_ServersSize", poolIndex));

        for (DAVA::uint32 serverIndex = 0; serverIndex < serversSize; ++serverIndex)
        {
            ServerID serverID = archive->GetUInt64(DAVA::Format("Pool_%u_Server_%u_ID", poolIndex, serverIndex), NullServerID);
            SharedServer& server = pool.servers[serverID];
            server.serverID = serverID;
            server.poolID = poolID;
            server.serverName = archive->GetString(DAVA::Format("Pool_%u_Server_%u_name", poolIndex, serverIndex));
            server.remoteParams.ip = archive->GetString(DAVA::Format("Pool_%u_Server_%u_ip", poolIndex, serverIndex));
            server.remoteParams.enabled = archive->GetBool(DAVA::Format("Pool_%u_Server_%u_enabled", poolIndex, serverIndex));
        }
    }

    sharedForOthers = archive->GetBool("SharedForOthers", DEFAULT_SHARED_FOR_OTHERS);
    ownPoolID = archive->GetUInt64("OwnPoolID", NullPoolID);
    ownID = archive->GetUInt64("OwnID", NullServerID);
    ownName = archive->GetString("OwnName");
}

DAVA::FilePath ApplicationSettings::GetDefaultFolder()
{
    return DEFAULT_FOLDER;
}

const DAVA::FilePath& ApplicationSettings::GetFolder() const
{
    return folder;
}

void ApplicationSettings::SetFolder(const DAVA::FilePath& _folder)
{
    folder = _folder;
}

const DAVA::float64 ApplicationSettings::GetCacheSizeGb() const
{
    return cacheSizeGb;
}

void ApplicationSettings::SetCacheSizeGb(const DAVA::float64 size)
{
    cacheSizeGb = size;
}

const DAVA::uint32 ApplicationSettings::GetFilesCount() const
{
    return filesCount;
}

void ApplicationSettings::SetFilesCount(const DAVA::uint32 count)
{
    filesCount = count;
}

const DAVA::uint64 ApplicationSettings::GetAutoSaveTimeoutMin() const
{
    return autoSaveTimeoutMin;
}

void ApplicationSettings::SetAutoSaveTimeoutMin(const DAVA::uint64 timeout)
{
    autoSaveTimeoutMin = timeout;
}

const DAVA::uint16 ApplicationSettings::GetPort() const
{
    return listenPort;
}

void ApplicationSettings::SetPort(const DAVA::uint16 val)
{
    listenPort = val;
}

const DAVA::uint16 ApplicationSettings::GetHttpPort() const
{
    return listenHttpPort;
}

void ApplicationSettings::SetHttpPort(const DAVA::uint16 port)
{
    listenHttpPort = port;
}

const bool ApplicationSettings::IsAutoStart() const
{
    return autoStart;
}

void ApplicationSettings::SetAutoStart(bool val)
{
    autoStart = val;
}

const bool ApplicationSettings::IsLaunchOnSystemStartup() const
{
    return launchOnSystemStartup;
}

void ApplicationSettings::SetLaunchOnSystemStartup(bool val)
{
    launchOnSystemStartup = val;
}

const bool ApplicationSettings::IsRestartOnCrash() const
{
    return restartOnCrash;
}

void ApplicationSettings::SetRestartOnCrash(bool val)
{
    restartOnCrash = val;
}

bool ApplicationSettings::IsSharedForOthers() const
{
    return sharedForOthers;
}

void ApplicationSettings::SetSharedForOthers(bool val)
{
    sharedForOthers = val;
}

ServerID ApplicationSettings::GetOwnID() const
{
    return ownID;
}

void ApplicationSettings::SetOwnID(ServerID val)
{
    ownID = val;
}

void ApplicationSettings::ResetOwnID()
{
    ownID = NullServerID;
}

void ApplicationSettings::SetOwnPoolID(PoolID val)
{
    ownPoolID = val;
}

PoolID ApplicationSettings::GetOwnPoolID() const
{
    return ownPoolID;
}

void ApplicationSettings::SetOwnName(DAVA::String val)
{
    ownName = val;
}

const DAVA::String& ApplicationSettings::GetOwnName() const
{
    return ownName;
}

const DAVA::Map<PoolID, SharedPool>& ApplicationSettings::GetSharedPools() const
{
    return sharedPools;
}

void ApplicationSettings::ClearCustomServers()
{
    customServers.clear();
}

const DAVA::List<RemoteServerParams>& ApplicationSettings::GetCustomServers() const
{
    return customServers;
}

void ApplicationSettings::AddCustomServer(const RemoteServerParams& server)
{
    customServers.push_back(server);
}

void ApplicationSettings::UpdateSharedPools(const DAVA::List<SharedPoolParams>& pools, const DAVA::List<SharedServerParams>& servers)
{
    DAVA::Map<PoolID, SharedPool> updatedPools;

    for (const SharedPoolParams& pool : pools)
    {
        SharedPool& updatedPool = updatedPools[pool.poolID];
        updatedPool.poolID = pool.poolID;
        updatedPool.poolName = pool.name;
        updatedPool.poolDescription = pool.description;
    }

    for (const SharedServerParams& server : servers)
    {
        auto poolIter = updatedPools.find(server.poolID);
        if (poolIter == updatedPools.end())
        {
            if (server.poolID == NullPoolID)
            {
                poolIter = updatedPools.insert(std::make_pair(NullPoolID, SharedPool())).first;
            }
            else
            {
                DAVA::Logger::Error("Can't find pool with id %u referenced by server id %u", server.poolID, server.serverID);
                continue;
            }
        }

        SharedServer& updatedServer = poolIter->second.servers[server.serverID];
        updatedServer.serverID = server.serverID;
        updatedServer.poolID = server.poolID;
        updatedServer.serverName = server.name;
        updatedServer.remoteParams.ip = server.ip;
    }

    EnabledRemote currentEnabledRemote = GetEnabledRemote();
    if (currentEnabledRemote.type == EnabledRemote::POOL)
    {
        auto poolIter = updatedPools.find(currentEnabledRemote.pool->poolID);
        if (poolIter != updatedPools.end())
        {
            SharedPool& pool = poolIter->second;
            pool.enabled = true;
        }
    }
    else if (currentEnabledRemote.type == EnabledRemote::POOL_SERVER)
    {
        auto poolIter = updatedPools.find(currentEnabledRemote.server->poolID);
        if (poolIter != updatedPools.end())
        {
            SharedPool& pool = poolIter->second;
            auto serverIter = pool.servers.find(currentEnabledRemote.server->serverID);
            if (serverIter != pool.servers.end())
            {
                SharedServer& server = serverIter->second;
                server.remoteParams.enabled = true;
            }
        }
    }

    sharedPools.swap(updatedPools);
    emit SettingsUpdated(this);
}

void ApplicationSettings::RemoveCustomServer(const RemoteServerParams& server)
{
    customServers.remove(server);
}

EnabledRemote ApplicationSettings::GetEnabledRemote()
{
    for (auto& poolIter : sharedPools)
    {
        SharedPool& pool = poolIter.second;
        if (pool.enabled)
            return EnabledRemote(&pool);

        for (auto& serverIter : pool.servers)
        {
            SharedServer& server = serverIter.second;
            if (server.remoteParams.enabled)
                return EnabledRemote(&server);
        }
    }

    for (RemoteServerParams& customServer : customServers)
    {
        if (customServer.enabled)
            return EnabledRemote(&customServer);
    }

    return EnabledRemote();
}

DAVA::List<RemoteServerParams> ApplicationSettings::GetEnabledRemoteServers()
{
    DAVA::List<RemoteServerParams> enabledRemotesParams;

    EnabledRemote enabledRemote = GetEnabledRemote();
    switch (enabledRemote.type)
    {
    case EnabledRemote::POOL:
    {
        for (auto& serverIter : enabledRemote.pool->servers)
        {
            SharedServer& server = serverIter.second;
            enabledRemotesParams.push_back(server.remoteParams);
        }
        break;
    }
    case EnabledRemote::POOL_SERVER:
    {
        enabledRemotesParams.push_back(enabledRemote.server->remoteParams);
        break;
    }
    case EnabledRemote::CUSTOM_SERVER:
    {
        enabledRemotesParams.push_back(*(enabledRemote.customServer));
        break;
    }
    case EnabledRemote::NONE:
    default:
        break;
    }

    return enabledRemotesParams;
}

void ApplicationSettings::DisableRemote()
{
    EnabledRemote enabledRemote = GetEnabledRemote();
    switch (enabledRemote.type)
    {
    case EnabledRemote::POOL:
        enabledRemote.pool->enabled = false;
        break;
    case EnabledRemote::POOL_SERVER:
        enabledRemote.server->remoteParams.enabled = false;
        break;
    case EnabledRemote::CUSTOM_SERVER:
        enabledRemote.customServer->enabled = false;
        break;
    default:
        break;
    }
}

void ApplicationSettings::DumpStorageSettings()
{
    DAVA::Logger::Info("Storage folder: %s", GetFolder().GetAbsolutePathname().c_str());
}

void ApplicationSettings::DumpSharingSettings()
{
    DAVA::Logger::Info("Shared for others: %s, ownID: %u, poolID: %u, name: %s", IsSharedForOthers() ? "YES" : "NO", GetOwnID(), GetOwnPoolID(), GetOwnName().c_str());
}

void ApplicationSettings::EnableSharedPool(PoolID poolID)
{
    DVASSERT(GetEnabledRemote().type == EnabledRemote::NONE);

    auto pairFound = sharedPools.find(poolID);
    if (pairFound != sharedPools.end())
    {
        SharedPool& pool = pairFound->second;
        pool.enabled = true;
    }
    else
    {
        DVASSERT(false, DAVA::Format("Can't find pool with id %u", poolID).c_str());
    }
}

void ApplicationSettings::EnableSharedServer(PoolID poolID, ServerID serverID)
{
    DVASSERT(GetEnabledRemote().type == EnabledRemote::NONE);

    auto pairFound = sharedPools.find(poolID);
    if (pairFound != sharedPools.end())
    {
        SharedPool& pool = pairFound->second;
        auto serverPairFound = pool.servers.find(serverID);
        if (serverPairFound != pool.servers.end())
        {
            SharedServer& server = serverPairFound->second;
            server.remoteParams.enabled = true;
        }
        else
        {
            DVASSERT(false, DAVA::Format("Can't find server with id %u inside of pool %u", serverID, poolID).c_str());
        }
    }
    else
    {
        DVASSERT(false, DAVA::Format("Can't find pool with id %u", poolID).c_str());
    }
}
