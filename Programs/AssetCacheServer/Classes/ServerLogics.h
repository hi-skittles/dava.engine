#pragma once

#include "CacheDB.h"

#include <AssetCache/AssetCache.h>

#include <FileSystem/DynamicMemoryFile.h>

class ServerLogics : public DAVA::AssetCache::ServerNetProxyListener,
                     public DAVA::AssetCache::ClientNetProxyListener
{
public:
    void Init(DAVA::AssetCache::ServerNetProxy* server, const DAVA::String& serverName, DAVA::AssetCache::ClientNetProxy* client, CacheDB* dataBase);
    void Update();
    void LazyUpdate();

    void OnRemoteDisconnecting();

    //ServerNetProxyListener
    void OnAddChunkToCache(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key, DAVA::uint64 dataSize, DAVA::uint32 numOfChunks, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunkData) override;
    void OnChunkRequestedFromCache(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key, DAVA::uint32 chunkNumber) override;
    void OnRemoveFromCache(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key) override;
    void OnClearCache(const std::shared_ptr<DAVA::Net::IChannel>& channel) override;
    void OnWarmingUp(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key) override;
    void OnChannelClosed(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::char8* message) override;
    void OnStatusRequested(const std::shared_ptr<DAVA::Net::IChannel>& channel) override;

    //ClientNetProxyListener
    void OnClientProxyStateChanged() override;
    void OnAddedToCache(const DAVA::AssetCache::CacheItemKey& key, bool added) override;
    void OnReceivedFromCache(const DAVA::AssetCache::CacheItemKey& key, DAVA::uint64 dataSize, DAVA::uint32 numOfChunks, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunkData) override;

private:
    struct DataGetTask
    {
        enum DataRequestStatus
        {
            READY,
            WAITING_NEXT_CHUNK
        };

        struct ClientStatus
        {
            ClientStatus(DataRequestStatus status = DataRequestStatus::READY)
                : status(status)
            {
            }
            DataRequestStatus status = DataRequestStatus::READY;
            DAVA::uint32 waitingChunk = 0;
            bool lastChunkWasSent = false;
        };

        DAVA::UnorderedMap<std::shared_ptr<DAVA::Net::IChannel>, ClientStatus> clients;
        DAVA::ScopedPtr<DAVA::DynamicMemoryFile> serializedData;
        DataRequestStatus dataStatus = READY;

        DAVA::uint64 bytesReady = 0;
        DAVA::uint64 bytesOverall = 0;
        DAVA::uint32 chunksReady = 0;
        DAVA::uint32 chunksOverall = 0;
    };
    using DataGetMap = DAVA::UnorderedMap<DAVA::AssetCache::CacheItemKey, DataGetTask>;

    struct DataAddTask
    {
        DAVA::AssetCache::CacheItemKey key;
        std::shared_ptr<DAVA::Net::IChannel> channel;
        DAVA::ScopedPtr<DAVA::DynamicMemoryFile> receivedData;

        size_t bytesReceived = 0;
        size_t bytesOverall = 0;
        DAVA::uint32 chunksReceived = 0;
        DAVA::uint32 chunksOverall = 0;
    };

    struct DataRemoteAddTask
    {
        DAVA::ScopedPtr<DAVA::DynamicMemoryFile> serializedData;
        DAVA::uint32 chunksSent = 0;
        DAVA::uint32 chunksOverall = 0;
        DAVA::uint64 bytesOverall = 0;
    };
    using DataRemoteAddMap = DAVA::UnorderedMap<DAVA::AssetCache::CacheItemKey, DataRemoteAddTask>;

    struct DataWarmupTask
    {
        DataWarmupTask(const DAVA::AssetCache::CacheItemKey& key)
            : key(key)
        {
        }
        DAVA::AssetCache::CacheItemKey key;
    };

private:
    bool IsRemoteServerConnected() const;

    DAVA::List<DataAddTask>::iterator GetOrCreateAddTask(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key);
    DataGetMap::iterator GetOrCreateGetTask(const DAVA::AssetCache::CacheItemKey& key);
    void RequestNextChunk(DataGetMap::iterator it);
    void SendChunkToClient(DataGetMap::iterator taskIt, const std::shared_ptr<DAVA::Net::IChannel>& clientChannel, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunk);
    void SendChunkToClients(DataGetMap::iterator taskIt, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunk);
    bool SendFirstChunkToRemote(DataRemoteAddMap::iterator taskIt);
    bool SendChunkToRemote(DataRemoteAddMap::iterator taskIt);
    void CancelGetTask(DataGetMap::iterator it);
    void CancelRemoteTasks();
    void RemoveClientFromTasks(const std::shared_ptr<DAVA::Net::IChannel>& clientChannel);
    void RemoveTaskIfChunksAreSent(ServerLogics::DataGetMap::iterator taskIt);

    void ProcessLazyTasks();

    void ProcessFirstRemoteAddDataTask();

private:
    DAVA::AssetCache::ServerNetProxy* serverProxy = nullptr;
    DAVA::AssetCache::ClientNetProxy* clientProxy = nullptr;
    CacheDB* dataBase = nullptr;

    DataGetMap dataGetTasks;
    DAVA::List<DataAddTask> dataAddTasks;
    DAVA::List<DataWarmupTask> dataWarmupTasks;
    DataRemoteAddMap dataRemoteAddTasks;
    DAVA::String serverName;
    bool hasIncomingRequestsRecently = false; // any incoming request has been received after last lazy update
};
