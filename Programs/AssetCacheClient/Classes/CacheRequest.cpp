#include "CacheRequest.h"

#include <Engine/Engine.h>
#include <AssetCache/AssetCacheConstants.h>

#include <Time/SystemTimer.h>
#include <Concurrency/Thread.h>
#include <Logger/Logger.h>

using namespace DAVA;

CacheRequest::CacheRequest(const String& commandLineOptionName)
    : options(commandLineOptionName)
{
    options.AddOption("-ip", VariantType(AssetCache::GetLocalHost()), "Set ip adress of Asset Cache Server.");
    options.AddOption("-p", VariantType(static_cast<uint32>(AssetCache::ASSET_SERVER_PORT)), "Set port of Asset Cache Server.");
    options.AddOption("-t", VariantType(static_cast<uint64>(5)), "Connection timeout seconds.");
    options.AddOption("-v", VariantType(false), "Verbose output.");
}

AssetCache::Error CacheRequest::Process(AssetCacheClient& cacheClient)
{
    if (options.GetOption("-v").AsBool())
    {
        GetEngineContext()->logger->SetLogLevel(Logger::LEVEL_FRAMEWORK);
    }

    AssetCacheClient::ConnectionParams params;
    params.ip = options.GetOption("-ip").AsString();
    params.port = static_cast<uint16>(options.GetOption("-p").AsUInt32());
    params.timeoutms = options.GetOption("-t").AsUInt64() * 1000; // convert to ms

    AssetCache::Error exitCode = cacheClient.ConnectSynchronously(params);
    if (AssetCache::Error::NO_ERRORS == exitCode)
    {
        exitCode = SendRequest(cacheClient);
    }
    cacheClient.Disconnect();

    return exitCode;
}

AssetCache::Error CacheRequest::CheckOptions() const
{
    return CheckOptionsInternal();
}
