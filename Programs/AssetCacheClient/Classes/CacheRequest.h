#ifndef __CACHE_REQUEST_H__
#define __CACHE_REQUEST_H__

#include <AssetCache/AssetCacheClient.h>
#include "CommandLine/ProgramOptions.h"

namespace DAVA
{
class AssetCacheClient;
}

class CacheRequest
{
public:
    CacheRequest(const DAVA::String& commandLineOptionName);
    virtual ~CacheRequest() = default;

    DAVA::AssetCache::Error Process(DAVA::AssetCacheClient& cacheClient);
    DAVA::AssetCache::Error CheckOptions() const;

protected:
    virtual DAVA::AssetCache::Error SendRequest(DAVA::AssetCacheClient& cacheClient) = 0;
    virtual DAVA::AssetCache::Error CheckOptionsInternal() const = 0;

public:
    DAVA::ProgramOptions options;
};

#endif //__CACHE_REQUEST_H__
