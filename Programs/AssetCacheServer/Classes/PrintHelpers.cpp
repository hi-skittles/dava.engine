#include "PrintHelpers.h"

DAVA::String Brief(const DAVA::AssetCache::CacheItemKey& key)
{
    static const size_t key_brief_length = 7;
    DAVA::String brief = key.ToString();
    brief.resize(key_brief_length);
    return brief;
}