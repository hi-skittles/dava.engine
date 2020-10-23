#include "GetRequest.h"

#include <AssetCache/AssetCacheClient.h>

#include <FileSystem/FileSystem.h>
#include <Time/SystemTimer.h>
#include <Logger/Logger.h>

using namespace DAVA;

GetRequest::GetRequest()
    : CacheRequest("get")
{
    options.AddOption("-k", VariantType(String("")), "Key (hash string) of requested data");
    options.AddOption("-f", VariantType(String("")), "Folder to save files from server");
}

AssetCache::Error GetRequest::SendRequest(AssetCacheClient& cacheClient)
{
    AssetCache::CacheItemKey key;
    key.FromString(options.GetOption("-k").AsString());

    FilePath folder = options.GetOption("-f").AsString();
    folder.MakeDirectoryPathname();

    AssetCache::CachedItemValue data;
    AssetCache::Error resultOfRequest = cacheClient.RequestFromCacheSynchronously(key, &data);
    if (resultOfRequest == AssetCache::Error::NO_ERRORS)
    {
        data.ExportToFolder(folder);
    }
    return resultOfRequest;
}

AssetCache::Error GetRequest::CheckOptionsInternal() const
{
    const String hash = options.GetOption("-k").AsString();
    if (hash.length() != AssetCache::HASH_SIZE * 2)
    {
        Logger::Error("[CacheRequest::%s] Wrong hash argument (%s)", __FUNCTION__, hash.c_str());
        return AssetCache::Error::WRONG_COMMAND_LINE;
    }

    const String folderpath = options.GetOption("-f").AsString();
    if (folderpath.empty())
    {
        Logger::Error("[GetRequest::%s] Empty folderpath", __FUNCTION__);
        return AssetCache::Error::WRONG_COMMAND_LINE;
    }

    return AssetCache::Error::NO_ERRORS;
}
