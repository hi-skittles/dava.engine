#ifndef __GET_REQUEST_H__
#define __GET_REQUEST_H__

#include "CacheRequest.h"

namespace DAVA
{
class AssetCacheClient;
}

class GetRequest : public CacheRequest
{
public:
    GetRequest();

protected:
    DAVA::AssetCache::Error SendRequest(DAVA::AssetCacheClient& cacheClient) override;
    DAVA::AssetCache::Error CheckOptionsInternal() const override;
};

#endif //__GET_REQUEST_H__