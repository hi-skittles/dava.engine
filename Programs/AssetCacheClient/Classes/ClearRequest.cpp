#include "ClearRequest.h"
#include <AssetCache/AssetCacheClient.h>

ClearRequest::ClearRequest()
    : CacheRequest("clear")
{
}

DAVA::AssetCache::Error ClearRequest::SendRequest(DAVA::AssetCacheClient& cacheClient)
{
    return cacheClient.ClearCacheSynchronously();
}

DAVA::AssetCache::Error ClearRequest::CheckOptionsInternal() const
{
    return DAVA::AssetCache::Error::NO_ERRORS;
}
