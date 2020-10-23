#pragma once

#include <AssetCache/CachedItemValue.h>
#include <Base/BaseTypes.h>
#include <chrono>

namespace DAVA
{
class KeyedArchive;
}

class ServerCacheEntry final
{
public:
    ServerCacheEntry();
    explicit ServerCacheEntry(const DAVA::AssetCache::CachedItemValue& value);

    ServerCacheEntry(const ServerCacheEntry& right) = delete;
    ServerCacheEntry(ServerCacheEntry&& right);

    ~ServerCacheEntry() = default;

    ServerCacheEntry& operator=(const ServerCacheEntry& right) = delete;
    ServerCacheEntry& operator=(ServerCacheEntry&& right);

    bool operator==(const ServerCacheEntry& right) const;

    void Serialize(DAVA::KeyedArchive* archieve) const;
    void Deserialize(DAVA::KeyedArchive* archieve);

    void UpdateAccessTimestamp();
    DAVA::uint64 GetTimestamp() const;

    DAVA::AssetCache::CachedItemValue& GetValue();

    bool Fetch(const DAVA::FilePath& folder);
    void Free();

private:
    DAVA::AssetCache::CachedItemValue value;

private:
    DAVA::uint64 accessTimestamp = 0;
};

inline void ServerCacheEntry::UpdateAccessTimestamp()
{
    accessTimestamp = std::chrono::steady_clock::now().time_since_epoch().count();
}

inline DAVA::uint64 ServerCacheEntry::GetTimestamp() const
{
    return accessTimestamp;
}

inline DAVA::AssetCache::CachedItemValue& ServerCacheEntry::GetValue()
{
    return value;
}
