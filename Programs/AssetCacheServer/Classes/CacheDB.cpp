#include "CacheDB.h"
#include "ServerCacheEntry.h"
#include "PrintHelpers.h"

#include <AssetCache/CachedItemValue.h>

#include <FileSystem/File.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/KeyedArchive.h>
#include <Debug/DVAssert.h>
#include <Time/SystemTimer.h>
#include <Utils/StringFormat.h>
#include <Logger/Logger.h>

const DAVA::String CacheDB::DB_FILE_NAME = "cache.dat";
const DAVA::uint32 CacheDB::VERSION = 1;

CacheDB::CacheDB(CacheDBOwner& _owner)
    : owner(_owner)
    , dbStateChanged(false)
{
}

CacheDB::~CacheDB()
{
    Unload();
}

void CacheDB::NotifySizeChanged()
{
    owner.OnStorageSizeChanged(occupiedSize, maxStorageSize);
}

void CacheDB::UpdateSettings(const DAVA::FilePath& folderPath, const DAVA::uint64 size, const DAVA::uint32 newMaxItemsInMemory, const DAVA::uint64 _autoSaveTimeout)
{
    bool fullCacheChanged = false;

    DAVA::FilePath newCacheRootFolder = folderPath;
    newCacheRootFolder.MakeDirectoryPathname();

    if (cacheRootFolder != newCacheRootFolder)
    {
        if (!cacheRootFolder.IsEmpty())
        {
            DAVA::Logger::Info("Cache folder is changed: was '%s', now '%s'", cacheRootFolder.GetAbsolutePathname().c_str(), newCacheRootFolder.GetAbsolutePathname().c_str());
            Unload();
        }

        cacheRootFolder = newCacheRootFolder;
        cacheSettings = cacheRootFolder + DB_FILE_NAME;

        Load();
        fullCacheChanged = true;
    }

    if (maxStorageSize != size)
    {
        ReduceFullCacheToSize(size);
        fullCacheChanged = true;

        maxStorageSize = size;
        NotifySizeChanged();
    }

    if (maxItemsInMemory != newMaxItemsInMemory)
    {
        DAVA::int32 reducedByCount = maxItemsInMemory - newMaxItemsInMemory;
        if (reducedByCount > 0)
        {
            ReduceFastCacheByCount(reducedByCount);
        }

        maxItemsInMemory = newMaxItemsInMemory;
        fastCache.reserve(maxItemsInMemory);
    }

    autoSaveTimeout = _autoSaveTimeout;

    if (fullCacheChanged)
    {
        Save();
    }
}

void CacheDB::Load()
{
    DVASSERT(fastCache.empty());
    DVASSERT(fullCache.empty());

    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(cacheSettings, DAVA::File::OPEN | DAVA::File::READ));
    if (!file)
    {
        return;
    }

    DAVA::ScopedPtr<DAVA::KeyedArchive> header(new DAVA::KeyedArchive());
    header->Load(file);

    if (header->GetString("signature") != "cache")
    {
        DAVA::Logger::Error("[CacheDB::%s] Wrong signature %s", __FUNCTION__, header->GetString("signature").c_str());
        return;
    }

    if (header->GetUInt32("version") != VERSION)
    {
        DVASSERT(false, "cachedb file version is changed. Versions load functions should be implemented");
        return;
    }

    DAVA::uint64 cacheSize = header->GetUInt64("itemsCount");
    fullCache.reserve(static_cast<size_t>(cacheSize));

    DAVA::ScopedPtr<DAVA::KeyedArchive> cache(new DAVA::KeyedArchive());
    if (!cache->Load(file))
    {
        DAVA::Logger::Error("[%s] Can't load cache file", __FUNCTION__);
        return;
    }

    occupiedSize = 0;
    for (DAVA::uint64 index = 0; index < cacheSize; ++index)
    {
        DAVA::KeyedArchive* itemArchieve = cache->GetArchive(DAVA::Format("item_%d", index));
        DVASSERT(nullptr != itemArchieve);

        DAVA::AssetCache::CacheItemKey key;
        key.Deserialize(itemArchieve);

        ServerCacheEntry entry;
        entry.Deserialize(itemArchieve);

        occupiedSize += entry.GetValue().GetSize();
        fullCache[key] = std::move(entry);
    }

    NotifySizeChanged();
    dbStateChanged = false;
}

void CacheDB::Unload()
{
    Save();

    for (auto& entry : fastCache)
    {
        entry.second->Free();
    }

    fastCache.clear();
    fullCache.clear();
    occupiedSize = 0;
    NotifySizeChanged();
}

void CacheDB::Save()
{
    DAVA::FileSystem::Instance()->CreateDirectory(cacheRootFolder, true);

    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(cacheSettings, DAVA::File::CREATE | DAVA::File::WRITE));
    if (!file)
    {
        DAVA::Logger::Error("[CacheDB::%s] Cannot create file %s", __FUNCTION__, cacheSettings.GetStringValue().c_str());
        return;
    }

    DAVA::ScopedPtr<DAVA::KeyedArchive> header(new DAVA::KeyedArchive());
    header->SetString("signature", "cache");
    header->SetUInt32("version", VERSION);
    header->SetUInt64("itemsCount", fullCache.size());
    header->Save(file);

    DAVA::ScopedPtr<DAVA::KeyedArchive> cache(new DAVA::KeyedArchive());
    DAVA::uint64 index = 0;
    for (auto& item : fullCache)
    {
        DAVA::ScopedPtr<DAVA::KeyedArchive> itemArchieve(new DAVA::KeyedArchive());
        item.first.Serialize(itemArchieve);
        item.second.Serialize(itemArchieve);

        cache->SetArchive(DAVA::Format("item_%d", index++), itemArchieve);
    }
    cache->Save(file);

    dbStateChanged = false;
    lastSaveTime = DAVA::SystemTimer::GetMs();
}

void CacheDB::ReduceFullCacheToSize(DAVA::uint64 toSize)
{
    while (occupiedSize > toSize)
    {
        const auto& found = std::min_element(fullCache.begin(), fullCache.end(), [](const CacheMap::value_type& left, const CacheMap::value_type& right) -> bool
                                             {
                                                 return left.second.GetTimestamp() < right.second.GetTimestamp();
                                             });
        if (found != fullCache.end())
        {
            Remove(found);
        }
        else
        {
            if (occupiedSize > 0)
            {
                DAVA::Logger::Warning("Occupied size is %u, should be 0", occupiedSize);
                occupiedSize = 0;
                NotifySizeChanged();
                break;
            }
        }
    }
}

void CacheDB::ReduceFastCacheByCount(DAVA::uint32 countToRemove)
{
    for (; countToRemove > 0; --countToRemove)
    {
        const auto& oldestFound = std::min_element(fastCache.begin(), fastCache.end(), [](const FastCacheMap::value_type& left, const FastCacheMap::value_type& right) -> bool
                                                   {
                                                       return left.second->GetTimestamp() < right.second->GetTimestamp();
                                                   });

        if (oldestFound != fastCache.end())
        {
            RemoveFromFastCache(oldestFound);
        }
        else
        {
            return;
        }
    }
}

ServerCacheEntry* CacheDB::Get(const DAVA::AssetCache::CacheItemKey& key)
{
    ServerCacheEntry* entry = FindInFastCache(key);

    if (nullptr == entry)
    {
        entry = FindInFullCache(key);
        if (nullptr != entry)
        {
            const DAVA::FilePath path = CreateFolderPath(key);

            if (true == entry->Fetch(path))
            {
                InsertInFastCache(key, entry);
            }
            else
            {
                DAVA::Logger::Error("[CacheDB::%s] Fetch error. Entry '%s' will be removed from cache", __FUNCTION__, Brief(key).c_str());
                if (false == Remove(key))
                {
                    DAVA::Logger::Error("[CacheDB::%s] Cannot find item in cache");
                }
                entry = nullptr;
            }
        }
    }

    UpdateAccessTimestamp(entry);

    return entry;
}

ServerCacheEntry* CacheDB::FindInFastCache(const DAVA::AssetCache::CacheItemKey& key) const
{
    auto found = fastCache.find(key);
    if (found != fastCache.cend())
    {
        return found->second;
    }

    return nullptr;
}

ServerCacheEntry* CacheDB::FindInFullCache(const DAVA::AssetCache::CacheItemKey& key)
{
    auto found = fullCache.find(key);
    if (found != fullCache.cend())
    {
        return &found->second;
    }

    return nullptr;
}

const ServerCacheEntry* CacheDB::FindInFullCache(const DAVA::AssetCache::CacheItemKey& key) const
{
    auto found = fullCache.find(key);
    if (found != fullCache.cend())
    {
        return &found->second;
    }

    return nullptr;
}

void CacheDB::ClearStorage()
{
    ReduceFullCacheToSize(0);
}

void CacheDB::Insert(const DAVA::AssetCache::CacheItemKey& key, const DAVA::AssetCache::CachedItemValue& value)
{
    ServerCacheEntry entry(value);
    Insert(key, std::forward<ServerCacheEntry>(entry));
}

void CacheDB::Insert(const DAVA::AssetCache::CacheItemKey& key, ServerCacheEntry&& entry)
{
    if (entry.GetValue().GetSize() > maxStorageSize)
    {
        if (maxStorageSize > 0)
        {
            DAVA::Logger::Warning("Inserted data size %llu is bigger than max storage size %llu", entry.GetValue().GetSize(), maxStorageSize);
        }
        return;
    }

    auto found = fullCache.find(key);
    if (found != fullCache.end())
    {
        Remove(found);
    }

    DAVA::Logger::Debug("Inserting into cache: key %s", Brief(key).c_str());
    fullCache[key] = std::move(entry);
    ServerCacheEntry* insertedEntry = &fullCache[key];
    DAVA::FilePath savedPath = CreateFolderPath(key);
    insertedEntry->GetValue().ExportToFolder(savedPath);
    insertedEntry->UpdateAccessTimestamp();
    occupiedSize += insertedEntry->GetValue().GetSize();
    NotifySizeChanged();

    InsertInFastCache(key, insertedEntry);

    if (occupiedSize > maxStorageSize)
    {
        ReduceFullCacheToSize(maxStorageSize);
        DVASSERT(fullCache.find(key) != fullCache.end());
    }

    dbStateChanged = true;
}

void CacheDB::InsertInFastCache(const DAVA::AssetCache::CacheItemKey& key, ServerCacheEntry* entry)
{
    if (fastCache.count(key) != 0)
    {
        return;
    }

    if (maxItemsInMemory == fastCache.size() && maxItemsInMemory > 0)
    {
        ReduceFastCacheByCount(1);
    }

    DVASSERT(entry->GetValue().IsFetched() == true);

    fastCache[key] = entry;
}

void CacheDB::UpdateAccessTimestamp(const DAVA::AssetCache::CacheItemKey& key)
{
    ServerCacheEntry* entry = FindInFastCache(key);
    if (nullptr == entry)
    {
        entry = FindInFullCache(key);
    }

    UpdateAccessTimestamp(entry);
}

void CacheDB::UpdateAccessTimestamp(ServerCacheEntry* entry)
{
    if (nullptr != entry)
    {
        entry->UpdateAccessTimestamp();
        dbStateChanged = true;
    }
}

bool CacheDB::Remove(const DAVA::AssetCache::CacheItemKey& key)
{
    auto found = fullCache.find(key);
    if (found != fullCache.end())
    {
        Remove(found);
        return true;
    }

    return false;
}

void CacheDB::Remove(const CacheMap::iterator& it)
{
    auto found = fastCache.find(it->first);
    if (found != fastCache.end())
    {
        RemoveFromFastCache(found);
    }

    RemoveFromFullCache(it);
    dbStateChanged = true;
}

void CacheDB::RemoveFromFullCache(const CacheMap::iterator& it)
{
    DVASSERT(it != fullCache.end());

    DAVA::FilePath dataPath = CreateFolderPath(it->first);
    DAVA::FileSystem::Instance()->DeleteDirectory(dataPath);

    DAVA::uint64 itemSize = it->second.GetValue().GetSize();
    DVASSERT(itemSize <= occupiedSize);
    occupiedSize -= itemSize;
    DAVA::Logger::Debug("Removing from full cache: key %s", Brief(it->first).c_str());
    fullCache.erase(it);
    NotifySizeChanged();
}

void CacheDB::RemoveFromFastCache(const FastCacheMap::iterator& it)
{
    DVASSERT(it != fastCache.end());

    DVASSERT(it->second->GetValue().IsFetched() == true);
    it->second->Free();
    fastCache.erase(it);
}

DAVA::FilePath CacheDB::CreateFolderPath(const DAVA::AssetCache::CacheItemKey& key) const
{
    DAVA::String keyString = key.ToString();
    DVASSERT(keyString.length() > 2);

    return (cacheRootFolder + (keyString.substr(0, 2) + "/" + keyString.substr(2) + "/"));
}

void CacheDB::Update()
{
    if (dbStateChanged && (autoSaveTimeout != 0))
    {
        auto curTime = DAVA::SystemTimer::GetMs();
        if (curTime - lastSaveTime > autoSaveTimeout)
        {
            Save();
            lastSaveTime = curTime;
        }
    }
}

const DAVA::uint64 CacheDB::GetAvailableSize() const
{
    DVASSERT(GetStorageSize() > GetOccupiedSize());
    return (GetStorageSize() - GetOccupiedSize());
}
