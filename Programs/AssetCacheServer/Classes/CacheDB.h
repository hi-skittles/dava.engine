#pragma once

#include <AssetCache/CacheItemKey.h>

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>

#include <atomic>

namespace DAVA
{
namespace AssetCache
{
class CachedItemValue;
}
}

class ServerCacheEntry;

struct CacheDBOwner
{
    virtual void OnStorageSizeChanged(DAVA::uint64 occupied, DAVA::uint64 overall) = 0;
};

class CacheDB final
{
    static const DAVA::String DB_FILE_NAME;
    static const DAVA::uint32 VERSION;

    using CacheMap = DAVA::UnorderedMap<DAVA::AssetCache::CacheItemKey, ServerCacheEntry>;
    using FastCacheMap = DAVA::UnorderedMap<DAVA::AssetCache::CacheItemKey, ServerCacheEntry*>;

public:
    CacheDB(CacheDBOwner& owner);
    ~CacheDB();

    void UpdateSettings(const DAVA::FilePath& folderPath, const DAVA::uint64 size, const DAVA::uint32 itemsInMemory, const DAVA::uint64 autoSaveTimeout);

    void Save();
    void Load();

    ServerCacheEntry* Get(const DAVA::AssetCache::CacheItemKey& key);

    void Insert(const DAVA::AssetCache::CacheItemKey& key, const DAVA::AssetCache::CachedItemValue& value);
    bool Remove(const DAVA::AssetCache::CacheItemKey& key);
    void ClearStorage();
    void UpdateAccessTimestamp(const DAVA::AssetCache::CacheItemKey& key);

    const DAVA::FilePath& GetPath() const;
    const DAVA::uint64 GetStorageSize() const;
    const DAVA::uint64 GetAvailableSize() const;
    const DAVA::uint64 GetOccupiedSize() const;

    void Update();

private:
    void Insert(const DAVA::AssetCache::CacheItemKey& key, ServerCacheEntry&& entry);

    DAVA::FilePath CreateFolderPath(const DAVA::AssetCache::CacheItemKey& key) const;

    void Unload();

    ServerCacheEntry* FindInFastCache(const DAVA::AssetCache::CacheItemKey& key) const;
    ServerCacheEntry* FindInFullCache(const DAVA::AssetCache::CacheItemKey& key);
    const ServerCacheEntry* FindInFullCache(const DAVA::AssetCache::CacheItemKey& key) const;

    void InsertInFastCache(const DAVA::AssetCache::CacheItemKey& key, ServerCacheEntry* entry);

    void UpdateAccessTimestamp(ServerCacheEntry* entry);

    void ReduceFullCacheToSize(DAVA::uint64 toSize);
    void ReduceFastCacheByCount(DAVA::uint32 countToRemove);

    void RemoveFromFastCache(const FastCacheMap::iterator& it);
    void RemoveFromFullCache(const CacheMap::iterator& it);

    void Remove(const CacheMap::iterator& it);

    void NotifySizeChanged();

private:
    CacheDBOwner& owner;

    DAVA::FilePath cacheRootFolder; //path to folder with settings and cache of files
    DAVA::FilePath cacheSettings; //path to settings

    DAVA::uint64 maxStorageSize = 0; //maximum cache size
    DAVA::uint32 maxItemsInMemory = 0; //count of items in memory, to use for fast access

    DAVA::uint64 occupiedSize = 0; //used by CacheItemValues
    DAVA::uint64 nextItemID = 0; //item counter, used as last access time token

    DAVA::uint64 autoSaveTimeout = 0;
    DAVA::uint64 lastSaveTime = 0;

    FastCacheMap fastCache; //runtime, week storage
    CacheMap fullCache; //stored on disk, strong storage

    std::atomic<bool> dbStateChanged; //flag about changes in db
};

inline const DAVA::FilePath& CacheDB::GetPath() const
{
    return cacheRootFolder;
}

inline const DAVA::uint64 CacheDB::GetStorageSize() const
{
    return maxStorageSize;
}

inline const DAVA::uint64 CacheDB::GetOccupiedSize() const
{
    return occupiedSize;
}
