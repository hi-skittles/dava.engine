#include "ServerCacheEntry.h"

#include "FileSystem/KeyedArchive.h"

#include "Debug/DVAssert.h"

ServerCacheEntry::ServerCacheEntry()
{
}

ServerCacheEntry::ServerCacheEntry(const DAVA::AssetCache::CachedItemValue& _value)
    : value(_value)
{
}

ServerCacheEntry::ServerCacheEntry(ServerCacheEntry&& right)
    : value(std::move(right.value))
    , accessTimestamp(right.accessTimestamp)
{
}

ServerCacheEntry& ServerCacheEntry::operator=(ServerCacheEntry&& right)
{
    if (this != &right)
    {
        value = std::move(right.value);
        accessTimestamp = right.accessTimestamp;
    }

    return (*this);
}

bool ServerCacheEntry::operator==(const ServerCacheEntry& right) const
{
    return (accessTimestamp == right.accessTimestamp) && (value == right.value);
}

void ServerCacheEntry::Serialize(DAVA::KeyedArchive* archieve) const
{
    DVASSERT(nullptr != archieve);

    archieve->SetUInt64("accessID", accessTimestamp);

    DAVA::ScopedPtr<DAVA::KeyedArchive> valueArchieve(new DAVA::KeyedArchive());
    value.Serialize(valueArchieve, false);
    archieve->SetArchive("value", valueArchieve);
}

void ServerCacheEntry::Deserialize(DAVA::KeyedArchive* archieve)
{
    DVASSERT(nullptr != archieve);

    accessTimestamp = archieve->GetUInt64("accessID");

    DAVA::KeyedArchive* valueArchieve = archieve->GetArchive("value");
    DVASSERT(valueArchieve);
    value.Deserialize(valueArchieve);
}

bool ServerCacheEntry::Fetch(const DAVA::FilePath& folder)
{
    return value.Fetch(folder);
}

void ServerCacheEntry::Free()
{
    value.Free();
}
