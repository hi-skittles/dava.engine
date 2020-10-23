#ifndef __ADD_REQUEST_H__
#define __ADD_REQUEST_H__

#include "CacheRequest.h"

namespace DAVA
{
class AssetCacheClient;
}

class AddRequest : public CacheRequest
{
public:
    AddRequest();

protected:
    DAVA::AssetCache::Error SendRequest(DAVA::AssetCacheClient& cacheClient) override;
    DAVA::AssetCache::Error CheckOptionsInternal() const override;
};

#endif //__ADD_REQUEST_H__