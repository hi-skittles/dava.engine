#include "AddRequest.h"

#include <AssetCache/AssetCacheClient.h>

#include "Logger/Logger.h"
#include "FileSystem/File.h"
#include "Platform/DeviceInfo.h"
#include "Time/DateTime.h"
#include "Utils/UTF8Utils.h"

using namespace DAVA;

AddRequest::AddRequest()
    : CacheRequest("add")
{
    options.AddOption("-k", VariantType(String("")), "Key (hash string) of requested data");
    options.AddOption("-f", VariantType(String("")), "Files list to send files to server", true);
}

DAVA::AssetCache::Error AddRequest::SendRequest(AssetCacheClient& cacheClient)
{
    AssetCache::CacheItemKey key;
    key.FromString(options.GetOption("-k").AsString());

    AssetCache::CachedItemValue value;

    uint32 filesCount = options.GetOptionValuesCount("-f");
    for (uint32 i = 0; i < filesCount; ++i)
    {
        const FilePath path = options.GetOption("-f", i).AsString();
        ScopedPtr<File> file(File::Create(path, File::OPEN | File::READ));
        if (file)
        {
            std::shared_ptr<Vector<uint8>> data = std::make_shared<Vector<uint8>>();

            uint64 dataSize64 = file->GetSize();
            if (dataSize64 > std::numeric_limits<uint32>::max())
            {
                Logger::Error("%s File (%s) size is bigger than 2^32", __FUNCTION__, path.GetStringValue().c_str());
                return AssetCache::Error::READ_FILES_ERROR;
            }

            uint32 dataSize = static_cast<uint32>(dataSize64);

            data.get()->resize(dataSize);

            auto read = file->Read(data.get()->data(), dataSize);
            DVASSERT(read == dataSize);

            value.Add(path.GetFilename(), data);
        }
        else
        {
            Logger::Error("[AddRequest::%s] Cannot read file(%s)", __FUNCTION__, path.GetStringValue().c_str());
            return AssetCache::Error::READ_FILES_ERROR;
        }
    }

    AssetCache::CachedItemValue::Description description;
    description.machineName = UTF8Utils::EncodeToUTF8(DeviceInfo::GetName());

    DateTime timeNow = DateTime::Now();
    description.creationDate = UTF8Utils::EncodeToUTF8(timeNow.GetLocalizedDate()) + "_" + UTF8Utils::EncodeToUTF8(timeNow.GetLocalizedTime());
    description.comment = "Asset Cache Client";

    value.SetDescription(description);
    value.UpdateValidationData();
    return cacheClient.AddToCacheSynchronously(key, value);
}

DAVA::AssetCache::Error AddRequest::CheckOptionsInternal() const
{
    const String hash = options.GetOption("-k").AsString();
    if (hash.length() != AssetCache::HASH_SIZE * 2)
    {
        Logger::Error("[CacheRequest::%s] Wrong hash argument (%s)", __FUNCTION__, hash.c_str());
        return AssetCache::Error::WRONG_COMMAND_LINE;
    }

    const String filepath = options.GetOption("-f").AsString();
    if (filepath.empty())
    {
        Logger::Error("[AddRequest::%s] Empty file list", __FUNCTION__);
        return AssetCache::Error::WRONG_COMMAND_LINE;
    }

    return AssetCache::Error::NO_ERRORS;
}
