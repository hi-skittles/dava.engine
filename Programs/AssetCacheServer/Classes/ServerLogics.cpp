#include "ServerLogics.h"
#include "ServerCacheEntry.h"
#include "PrintHelpers.h"

#include <AssetCache/ChunkSplitter.h>

#include <Concurrency/LockGuard.h>
#include <Logger/Logger.h>
#include <Utils/StringFormat.h>

void ServerLogics::Init(DAVA::AssetCache::ServerNetProxy* server_, const DAVA::String& serverName_, DAVA::AssetCache::ClientNetProxy* client_, CacheDB* dataBase_)
{
    serverProxy = server_;
    serverName = serverName_;
    clientProxy = client_;
    dataBase = dataBase_;
}

void ServerLogics::OnAddChunkToCache(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key, DAVA::uint64 dataSize, DAVA::uint32 numOfChunks, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunkData)
{
    hasIncomingRequestsRecently = true;

    using namespace DAVA;

    DAVA::List<DataAddTask>::iterator it = GetOrCreateAddTask(channel, key);
    DataAddTask& task = *it;

    auto DiscardTask = [&]()
    {
        DAVA::Logger::Debug("Sending 'add data chunk failed' response");
        serverProxy->SendAddedToCache(channel, key, false);
        dataAddTasks.erase(it);
    };

    auto Error = [&](const char* err)
    {
        Logger::Error("Wrong request: %s. Client %p, key %s chunk#%u", err, channel.get(), Brief(key).c_str(), chunkNumber);
        DiscardTask();
    };

    if (chunkNumber == 0)
    {
        if (dataSize == 0 || numOfChunks == 0)
        {
            Error("both data size and number of chunks are zero");
            return;
        }

        DAVA::Logger::Debug("Receiving add request: key %s, %u bytes, %u chunks", Brief(key).c_str(), dataSize, numOfChunks);

        if (task.chunksOverall != 0 || task.bytesOverall != 0)
        {
            Error("add data info was already received with given key and channel");
            return;
        }

        if (dataSize > dataBase->GetStorageSize())
        {
            DAVA::Logger::Warning("Inserted data size %u is bigger than max storage size %u", dataSize, dataBase->GetStorageSize());
            DiscardTask();
            return;
        }

        task.bytesOverall = dataSize;
        task.chunksOverall = numOfChunks;
    }

    Logger::Debug("Adding chunk #%u, %u bytes. Overall received %u, remaining %u", chunkNumber, chunkData.size(), task.bytesReceived, task.bytesOverall - task.bytesReceived);

    if (task.chunksReceived != chunkNumber)
    {
        Error(Format("chunk #%u was expected", task.chunksReceived).c_str());
        return;
    }

    uint32 chunkSize = static_cast<uint32>(chunkData.size());
    uint32 written = task.receivedData->Write(chunkData.data(), chunkSize);
    if (written != chunkSize)
    {
        Error(Format("can't append %u bytes", chunkSize).c_str());
        return;
    }

    task.bytesReceived += chunkSize;
    ++task.chunksReceived;

    if (task.chunksReceived == task.chunksOverall)
    {
        if (task.bytesReceived != task.bytesOverall)
        {
            Error(Format("unexpected final bytes count: %u (expected %u bytes)", task.bytesReceived, task.bytesOverall).c_str());
            return;
        }

        AssetCache::CachedItemValue value;
        task.receivedData->Seek(0, File::SEEK_FROM_START);
        value.Deserialize(task.receivedData);
        if (value.IsEmpty() || !value.IsValid())
        {
            Error("Received data is empty or invalid");
            return;
        }

        AssetCache::CachedItemValue::Description description = value.GetDescription();
        description.addingChain += "/" + serverName;
        value.SetDescription(description);

        if (value.GetSize() > dataBase->GetStorageSize())
        {
            Logger::Warning("Inserted data size %u is bigger than max storage size %u", value.GetSize(), dataBase->GetStorageSize());
            DiscardTask();
            return;
        }

        dataBase->Insert(key, value);
        dataAddTasks.erase(it);
        dataRemoteAddTasks.emplace(key, DataRemoteAddTask());
        DAVA::Logger::Debug("Adding remote add task. Tasks now: %u", dataRemoteAddTasks.size());
    }

    DAVA::Logger::Debug("Sending 'chunk successfully added' response");
    serverProxy->SendAddedToCache(channel, key, true);
}

DAVA::List<ServerLogics::DataAddTask>::iterator ServerLogics::GetOrCreateAddTask(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key)
{
    using namespace DAVA;
    List<ServerLogics::DataAddTask>::iterator it = std::find_if(dataAddTasks.begin(), dataAddTasks.end(), [&](const DataAddTask& task)
                                                                {
                                                                    return (task.channel == channel && task.key == key);
                                                                });

    if (it == dataAddTasks.end())
    {
        it = dataAddTasks.emplace(dataAddTasks.end(), DataAddTask());
        it->channel = channel;
        it->key = key;
        it->receivedData = DynamicMemoryFile::Create(File::CREATE | File::WRITE | File::READ);
    }

    return it;
}

ServerLogics::DataGetMap::iterator ServerLogics::GetOrCreateGetTask(const DAVA::AssetCache::CacheItemKey& key)
{
    using namespace DAVA;

    DataGetMap::iterator taskIter = dataGetTasks.find(key);
    if (taskIter == dataGetTasks.end())
    {
        ServerCacheEntry* entry = dataBase->Get(key);
        if (nullptr != entry)
        { // Found in db.
            Logger::Debug("Creating get task using local data");
            taskIter = dataGetTasks.emplace(key, DataGetTask()).first;
            DataGetTask& task = taskIter->second;
            task.serializedData = DynamicMemoryFile::Create(File::CREATE | File::READ | File::WRITE);

            AssetCache::CachedItemValue& value = entry->GetValue();
            AssetCache::CachedItemValue::Description description = value.GetDescription();
            description.receivingChain += "/" + serverName;
            value.SetDescription(description);
            value.Serialize(task.serializedData);
            task.dataStatus = DataGetTask::READY;
            task.bytesOverall = task.bytesReady = task.serializedData->GetSize();
            task.chunksOverall = task.chunksReady = AssetCache::ChunkSplitter::GetNumberOfChunks(task.bytesOverall);
        }
        else if (IsRemoteServerConnected() && clientProxy->RequestGetNextChunk(key, 0))
        { // Not found in db. Ask from remote cache.
            Logger::Debug("Creating get task. Requesting data from remote");
            taskIter = dataGetTasks.emplace(key, DataGetTask()).first;
            DataGetTask& task = taskIter->second;
            task.serializedData = DynamicMemoryFile::Create(File::CREATE | File::READ | File::WRITE);
            task.dataStatus = DataGetTask::WAITING_NEXT_CHUNK;
        }
    }

    return taskIter;
}

void ServerLogics::OnChunkRequestedFromCache(const std::shared_ptr<DAVA::Net::IChannel>& clientChannel, const DAVA::AssetCache::CacheItemKey& key, DAVA::uint32 chunkNumber)
{
    hasIncomingRequestsRecently = true;

    using namespace DAVA;
    Logger::Debug("Requested chunk #%u, key %s", chunkNumber, Brief(key).c_str());

    auto Error = [&](const char* err)
    {
        Logger::Error("Wrong chunk request: %s. Client %p, key %s, chunk %u", err, clientChannel.get(), Brief(key).c_str(), chunkNumber);
        Vector<uint8> empty;
        serverProxy->SendChunk(clientChannel, key, 0, 0, 0, empty);
    };

    DataGetMap::iterator taskIter = GetOrCreateGetTask(key);
    if (taskIter != dataGetTasks.end())
    {
        DataGetTask& task = taskIter->second;
        DataGetTask::ClientStatus& client = task.clients[clientChannel];

        if (task.chunksReady > chunkNumber) // task has such chunk
        {
            Vector<uint8> chunk = AssetCache::ChunkSplitter::GetChunk(task.serializedData->GetDataVector(), chunkNumber);
            if (chunk.empty())
            {
                Error("can't get valid range for given chunk");
                return;
            }

            if (chunkNumber == 0)
            {
                DAVA::Logger::Debug("Requested data will be sent: %u chunks, %u bytes", task.chunksOverall, task.bytesOverall);
            }

            SendChunkToClient(taskIter, clientChannel, chunkNumber, chunk);
            RemoveTaskIfChunksAreSent(taskIter);
        }
        else // task hasn't such chunk yet
        {
            if (task.dataStatus == DataGetTask::READY)
            {
                Error("task status is READY yet not all chunks are available");
                return;
            }
            else
            {
                client.status = DataGetTask::WAITING_NEXT_CHUNK;
                client.waitingChunk = chunkNumber;
            }
        }
    }
    else
    { // Not found in db. Remote server isn't connected.
        Vector<uint8> empty;
        DAVA::Logger::Debug("Sending empty chunk");
        serverProxy->SendChunk(clientChannel, key, 0, 0, 0, empty);
    }
}

void ServerLogics::OnRemoveFromCache(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key)
{
    hasIncomingRequestsRecently = true;

    if ((nullptr != serverProxy) && (nullptr != dataBase) && (nullptr != channel))
    {
        DAVA::Logger::Debug("Receiving remove from cache: key %s, channel %p", Brief(key).c_str(), channel.get());
        bool removed = dataBase->Remove(key);
        DAVA::Logger::Debug("Sending data %s removed", (removed ? "is" : "is not"));
        serverProxy->SendRemovedFromCache(channel, key, removed);
    }
}

void ServerLogics::OnClearCache(const std::shared_ptr<DAVA::Net::IChannel>& channel)
{
    hasIncomingRequestsRecently = true;

    DAVA::Logger::Debug("Receiving clearing of cache from channel %p", channel.get());
    dataBase->ClearStorage();
    DAVA::Logger::Debug("Sending storage is cleared");
    serverProxy->SendCleared(channel, true);
}

void ServerLogics::OnWarmingUp(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::AssetCache::CacheItemKey& key)
{
    DAVA::Logger::Debug("Receiving warming up request from channel %p key %s", channel.get(), Brief(key).c_str());
    if (nullptr != dataBase)
    {
        dataBase->UpdateAccessTimestamp(key);
    }
}

void ServerLogics::OnStatusRequested(const std::shared_ptr<DAVA::Net::IChannel>& channel)
{
    hasIncomingRequestsRecently = true;

    DAVA::Logger::Debug("Received status request from channel %p", channel.get());
    serverProxy->SendStatus(channel);
}

void ServerLogics::OnChannelClosed(const std::shared_ptr<DAVA::Net::IChannel>& channel, const DAVA::char8*)
{
    DAVA::Logger::Debug("Channel %p is closed", channel.get());
    RemoveClientFromTasks(channel);
}

void ServerLogics::OnRemoteDisconnecting()
{
    DAVA::Logger::Debug("Remote server is disconnecting");
    CancelRemoteTasks();
}

void ServerLogics::OnClientProxyStateChanged()
{
    DVASSERT(clientProxy);
    if (!clientProxy->ChannelIsOpened())
    {
        OnRemoteDisconnecting();
    }
}

void ServerLogics::OnReceivedFromCache(const DAVA::AssetCache::CacheItemKey& key, DAVA::uint64 dataSize, DAVA::uint32 numOfChunks, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunkData)
{
    hasIncomingRequestsRecently = true;

    using namespace DAVA;

    auto Error = [&](const char* err, DataGetMap::iterator taskIter)
    {
        Logger::Error("Wrong chunk response: %s. Key %s, chunk %u", err, Brief(key).c_str(), chunkNumber);
        CancelGetTask(taskIter);
    };

    auto taskIter = dataGetTasks.find(key);
    if (taskIter == dataGetTasks.end())
    {
        Error("data was not requested", taskIter);
        return;
    }

    DataGetTask& task = taskIter->second;

    if (chunkNumber == 0)
    {
        if (task.dataStatus != DataGetTask::DataRequestStatus::WAITING_NEXT_CHUNK)
        {
            Error("data status is not WAITING_NEXT_CHUNK", taskIter);
            return;
        }

        DVASSERT(task.bytesReady == 0 && task.chunksReady == 0 && task.serializedData->GetSize() == 0);

        if (dataSize == 0 || numOfChunks == 0)
        {
            CancelGetTask(taskIter);
            return;
        }
        else if (dataSize > dataBase->GetStorageSize())
        {
            DAVA::Logger::Warning("Inserted data size %u is bigger than max storage size %u", dataSize, dataBase->GetStorageSize());
            CancelGetTask(taskIter);
            return;
        }
        else
        {
            task.bytesOverall = dataSize;
            task.chunksOverall = numOfChunks;
        }
    }

    if (task.dataStatus != DataGetTask::DataRequestStatus::WAITING_NEXT_CHUNK)
    {
        Error("data status is not WAITING_NEXT_CHUNK", taskIter);
        return;
    }

    Logger::Debug("Receiving chunk #%u: %u bytes. Overall received %u, remaining %u", chunkNumber, chunkData.size(), task.bytesReady, task.bytesOverall - task.bytesReady);

    if (chunkData.empty())
    {
        Logger::Debug("Empty chunk is received. GetData task will be canceled for all clients");
        CancelGetTask(taskIter);
        return;
    }

    if (chunkNumber != task.chunksReady)
    {
        Error(Format("chunk #%u was expected", task.chunksReady).c_str(), taskIter);
        return;
    }

    uint32 chunkSize = static_cast<uint32>(chunkData.size());
    uint32 written = task.serializedData->Write(chunkData.data(), chunkSize);
    if (written != chunkSize)
    {
        Error(Format("can't append %u bytes", chunkSize).c_str(), taskIter);
        return;
    }

    task.bytesReady += chunkSize;
    ++task.chunksReady;

    if (task.chunksReady == task.chunksOverall)
    {
        if (task.bytesReady != task.bytesOverall)
        {
            Error(Format("unexpected final bytes count: %u (expected %u bytes)", task.bytesReady, task.bytesOverall).c_str(), taskIter);
            return;
        }

        task.dataStatus = DataGetTask::READY;

        AssetCache::CachedItemValue value;
        task.serializedData->Seek(0, File::SEEK_FROM_START);
        value.Deserialize(task.serializedData);
        if (value.IsEmpty() || !value.IsValid())
        {
            Logger::Debug("Received data is empty or invalid");
            CancelGetTask(taskIter);
            return;
        }

        dataBase->Insert(key, value);
    }
    else
    {
        RequestNextChunk(taskIter);
    }

    SendChunkToClients(taskIter, chunkNumber, chunkData);
}

void ServerLogics::OnAddedToCache(const DAVA::AssetCache::CacheItemKey& key, bool received)
{
    DAVA::Logger::Debug("Receiving response: info/chunk was %s by the remote cache", (received ? "received" : "not received"));
    DataRemoteAddMap::iterator itTask = dataRemoteAddTasks.find(key);
    if (itTask != dataRemoteAddTasks.end())
    {
        DataRemoteAddTask& task = itTask->second;

        if (received)
        {
            if (task.chunksSent == task.chunksOverall)
            {
                DAVA::Logger::Debug("All chunks are sent. Removing remote add task. Tasks remaining: %u", dataRemoteAddTasks.size() - 1);
                dataRemoteAddTasks.erase(itTask);
                ProcessFirstRemoteAddDataTask();
                return;
            }
            else
            {
                bool sentOk = SendChunkToRemote(itTask);
                if (!sentOk)
                {
                    dataRemoteAddTasks.erase(itTask);
                    ProcessFirstRemoteAddDataTask();
                    return;
                }
            }
        }
        else
        {
            DAVA::Logger::Debug("Chunk/info was not added to remote cache. Removing task");
            dataRemoteAddTasks.erase(itTask);
            ProcessFirstRemoteAddDataTask();
            return;
        }
    }
    else
    {
        DAVA::Logger::Error("Answer for unknown remote add task is received. Key %s", Brief(key).c_str());
    }
}

void ServerLogics::RequestNextChunk(ServerLogics::DataGetMap::iterator it)
{
    DVASSERT(it != dataGetTasks.end());

    const DAVA::AssetCache::CacheItemKey& key = it->first;
    DataGetTask& task = it->second;
    DVASSERT(task.dataStatus != DataGetTask::READY);
    DVASSERT(task.chunksReady < task.chunksOverall);

    DAVA::Logger::Debug("Sending request for chunk #%u", task.chunksReady);
    clientProxy->RequestGetNextChunk(key, task.chunksReady);
    task.dataStatus = DataGetTask::WAITING_NEXT_CHUNK;
}

void ServerLogics::SendChunkToClient(DataGetMap::iterator taskIt, const std::shared_ptr<DAVA::Net::IChannel>& clientChannel, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunk)
{
    DataGetTask& task = taskIt->second;
    DataGetTask::ClientStatus& client = task.clients[clientChannel];

    DAVA::Logger::Debug("Sending chunk #%u: %u bytes", chunkNumber, chunk.size());
    serverProxy->SendChunk(clientChannel, taskIt->first, task.bytesOverall, task.chunksOverall, chunkNumber, chunk);
    client.status = DataGetTask::READY;

    if (chunkNumber + 1 == task.chunksOverall)
    {
        client.lastChunkWasSent = true;
    }
}

void ServerLogics::SendChunkToClients(ServerLogics::DataGetMap::iterator taskIt, DAVA::uint32 chunkNumber, const DAVA::Vector<DAVA::uint8>& chunk)
{
    DVASSERT(taskIt != dataGetTasks.end());

    const DAVA::AssetCache::CacheItemKey& key = taskIt->first;
    DataGetTask& task = taskIt->second;

    for (std::pair<std::shared_ptr<DAVA::Net::IChannel> const, DataGetTask::ClientStatus>& client : task.clients)
    {
        if (client.second.status == DataGetTask::WAITING_NEXT_CHUNK && client.second.waitingChunk == chunkNumber)
        {
            SendChunkToClient(taskIt, client.first, chunkNumber, chunk);
        }
    }

    RemoveTaskIfChunksAreSent(taskIt);
}

void ServerLogics::RemoveTaskIfChunksAreSent(ServerLogics::DataGetMap::iterator taskIt)
{
    DataGetTask& task = taskIt->second;
    bool allChunksAreSent = std::all_of(task.clients.begin(), task.clients.end(), [](std::pair<std::shared_ptr<DAVA::Net::IChannel> const, DataGetTask::ClientStatus>& client)
                                        {
                                            return client.second.lastChunkWasSent == true;
                                        });

    if (allChunksAreSent)
    {
        DAVA::Logger::Debug("Removing get task for key %s", Brief(taskIt->first).c_str());
        dataGetTasks.erase(taskIt);
    }
}

bool ServerLogics::SendFirstChunkToRemote(DataRemoteAddMap::iterator taskIt)
{
    using namespace DAVA;

    DVASSERT(taskIt != dataRemoteAddTasks.end());

    const AssetCache::CacheItemKey& key = taskIt->first;
    DataRemoteAddTask& task = taskIt->second;

    ServerCacheEntry* entry = dataBase->Get(key);
    if (entry)
    {
        task.serializedData = DynamicMemoryFile::Create(File::CREATE | File::READ | File::WRITE);
        AssetCache::CachedItemValue& value = entry->GetValue();
        value.Serialize(task.serializedData);
        task.bytesOverall = task.serializedData->GetSize();
        task.chunksOverall = AssetCache::ChunkSplitter::GetNumberOfChunks(task.bytesOverall);
        task.chunksSent = 0;
        return SendChunkToRemote(taskIt);
    }
    else
    {
        Logger::Warning("Data with key %s is not found in DB", Brief(key).c_str());
        return false;
    }
}

bool ServerLogics::SendChunkToRemote(DataRemoteAddMap::iterator taskIt)
{
    using namespace DAVA;

    DVASSERT(taskIt != dataRemoteAddTasks.end());

    const AssetCache::CacheItemKey& key = taskIt->first;
    DataRemoteAddTask& task = taskIt->second;

    Vector<uint8> chunk = AssetCache::ChunkSplitter::GetChunk(task.serializedData->GetDataVector(), task.chunksSent);
    DAVA::Logger::Debug("Sending add chunk %u/%u to remote, key %s", task.chunksSent, task.chunksOverall, Brief(key).c_str());
    return clientProxy->RequestAddNextChunk(key, task.bytesOverall, task.chunksOverall, task.chunksSent++, chunk);
}

void ServerLogics::CancelGetTask(ServerLogics::DataGetMap::iterator it)
{
    using namespace DAVA;

    DAVA::Logger::Debug("Canceling get task");

    if (it != dataGetTasks.end())
    {
        const DAVA::AssetCache::CacheItemKey& key = it->first;
        DataGetTask& task = it->second;

        for (const std::pair<std::shared_ptr<DAVA::Net::IChannel>, DataGetTask::ClientStatus>& client : task.clients)
        {
            switch (client.second.status)
            {
            case DataGetTask::READY:
                break;
            case DataGetTask::WAITING_NEXT_CHUNK:
                DAVA::Logger::Debug("Sending empty chunk");
                serverProxy->SendChunk(client.first, key, 0, 0, 0, Vector<uint8>());
                break;
            default:
                DVASSERT(false, Format("Incorrect data status: %u", task.dataStatus).c_str());
                break;
            }
        }

        dataGetTasks.erase(it);
    }
}

void ServerLogics::RemoveClientFromTasks(const std::shared_ptr<DAVA::Net::IChannel>& clientChannel)
{
    for (auto it = dataGetTasks.begin(); it != dataGetTasks.end();)
    {
        DataGetTask& task = it->second;
        task.clients.erase(clientChannel);
        if (task.clients.empty() && task.dataStatus == DataGetTask::READY)
        {
            auto itDel = it++;
            DAVA::Logger::Debug("removing get task, no one more needs it");
            dataGetTasks.erase(itDel);
        }
        else
        {
            ++it;
        }
    }

    dataAddTasks.remove_if([clientChannel](const DataAddTask& task)
                           {
                               return task.channel == clientChannel;
                           });
}

void ServerLogics::CancelRemoteTasks()
{
    for (auto it = dataGetTasks.begin(); it != dataGetTasks.end();)
    {
        DataGetTask& task = it->second;
        if (task.dataStatus != DataGetTask::READY)
        {
            DAVA::Logger::Debug("Cancel remote get task, key %s", Brief(it->first).c_str());
            auto itDel = it++;
            CancelGetTask(itDel);
        }
        else
        {
            ++it;
        }
    }

    dataRemoteAddTasks.clear();
}

void ServerLogics::ProcessLazyTasks()
{
    if (IsRemoteServerConnected())
    {
        if (!hasIncomingRequestsRecently)
        {
            for (DataWarmupTask& task : dataWarmupTasks)
            {
                DAVA::Logger::Debug("Sending warm-up request, key %s", Brief(task.key).c_str());
                clientProxy->RequestWarmingUp(task.key);
            }
            dataWarmupTasks.clear();

            ProcessFirstRemoteAddDataTask();
        }
    }
    else
    {
        dataWarmupTasks.clear();
        dataRemoteAddTasks.clear();
    }

    hasIncomingRequestsRecently = false;
}

void ServerLogics::ProcessFirstRemoteAddDataTask()
{
    if (!hasIncomingRequestsRecently)
    {
        auto firstRemoteDataTask = dataRemoteAddTasks.begin();
        if (firstRemoteDataTask != dataRemoteAddTasks.end())
        {
            DataRemoteAddTask& task = firstRemoteDataTask->second;
            if (!task.chunksSent)
            {
                bool sentOk = SendFirstChunkToRemote(firstRemoteDataTask);
                if (!sentOk)
                {
                    dataRemoteAddTasks.erase(firstRemoteDataTask);
                    ProcessFirstRemoteAddDataTask();
                }
            }
        }
    }
}

void ServerLogics::Update()
{
    if (dataBase)
    {
        dataBase->Update();
    }
}

void ServerLogics::LazyUpdate()
{
    ProcessLazyTasks();
}

bool ServerLogics::IsRemoteServerConnected() const
{
    return (clientProxy && clientProxy->ChannelIsOpened());
}
