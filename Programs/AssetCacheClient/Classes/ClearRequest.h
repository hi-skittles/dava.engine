#pragma once

#include "CacheRequest.h"

namespace DAVA
{
class AssetCacheClient;
}

class ClearRequest : public CacheRequest
{
public:
    ClearRequest();

protected:
    DAVA::AssetCache::Error SendRequest(DAVA::AssetCacheClient& cacheClient) override;
    DAVA::AssetCache::Error CheckOptionsInternal() const override;
};
