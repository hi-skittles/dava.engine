#pragma once

#include "CacheRequest.h"

namespace DAVA
{
class AssetCacheClient;
}

class RemoveRequest : public CacheRequest
{
public:
    RemoveRequest();

protected:
    DAVA::AssetCache::Error SendRequest(DAVA::AssetCacheClient& cacheClient) override;
    DAVA::AssetCache::Error CheckOptionsInternal() const override;
};
