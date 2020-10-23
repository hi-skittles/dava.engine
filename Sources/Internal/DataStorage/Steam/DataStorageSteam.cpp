#include "DataStorageSteam.h"

#include "Platform/Steam.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/DynamicMemoryFile.h"
#include "Logger/Logger.h"

#if defined(__DAVAENGINE_STEAM__)

#include "steam/steam_api.h"
namespace DAVA
{
IDataStorage* DataStorage::Create()
{
    return new DataStorageSteam();
}

DataStorageSteam::DataStorageSteam()
    : values(new KeyedArchive)
{
    remoteStorage = Steam::CreateStorage();
}

ScopedPtr<KeyedArchive> DataStorageSteam::ReadArchFromStorage() const
{
    ScopedPtr<KeyedArchive> dataArchive(nullptr);

    if (!Steam::IsInited())
    {
        Logger::Error("Can't read from steam cloud storage - Steam is not inited yet.");
        return dataArchive;
    }

    ScopedPtr<DynamicMemoryFile> dataFile(nullptr);

    if (!remoteStorage->FileExists(storageFileName.c_str()))
    {
        return dataArchive;
    }

    int32 cubFileSize = remoteStorage->GetFileSize(storageFileName.c_str());
    if (0 >= cubFileSize)
    {
        return dataArchive;
    }

    uint8* buffer = new uint8[cubFileSize];

    int32 cubRead = remoteStorage->FileRead(storageFileName.c_str(), buffer, cubFileSize);

    dataFile = DynamicMemoryFile::Create(buffer, cubFileSize, File::CREATE | File::WRITE | File::READ);

    dataFile->Seek(0, File::SEEK_FROM_START);

    dataArchive = new KeyedArchive();
    bool isLoaded = dataArchive->Load(dataFile);
    DVASSERT(isLoaded, "Wrong SteamArchive Format.");

    return dataArchive;
}

void DataStorageSteam::WriteArchiveToStorage(const ScopedPtr<KeyedArchive> arch) const
{
    if (!Steam::IsInited())
    {
        Logger::Error("Can't write to steam cloud storage - Steam is not inited yet.");
        return;
    }

    ScopedPtr<DynamicMemoryFile> dataFile(DynamicMemoryFile::Create(File::CREATE | File::WRITE));
    arch->Save(dataFile);

    bool isWritten = remoteStorage->FileWrite(storageFileName.c_str(), dataFile->GetData(), static_cast<int32>(dataFile->GetSize()));

    if (!isWritten)
    {
        Logger::Error("Can't write to Cloud Steam Storage.");
    }
}

String DataStorageSteam::GetStringValue(const String& key)
{
    const String value = values->GetString(key);
    return value;
}

int64 DataStorageSteam::GetLongValue(const String& key)
{
    int64 value = values->GetInt64(key);
    return value;
}

void DataStorageSteam::SetStringValue(const String& key, const String& value)
{
    values->SetString(key, value);
    isValuesChanged = true;
};

void DataStorageSteam::SetLongValue(const String& key, int64 value)
{
    values->SetInt64(key, value);
    isValuesChanged = true;
}

void DataStorageSteam::RemoveEntry(const String& key)
{
    values->DeleteKey(key);
    isValuesChanged = true;
}

void DataStorageSteam::Clear()
{
    values->DeleteAllKeys();
    WriteArchiveToStorage(values);
    isValuesChanged = false;
}

void DataStorageSteam::Push()
{
    auto remoteArch = ReadArchFromStorage();
    if (!remoteArch)
    {
        Logger::Error("Can't sync steam cloud storage - Steam is not inited yet.");
        return;
    }

    auto remoteMap = remoteArch->GetArchieveData();

    // iterate over remote keys and merge new keys into local archive
    // use local values for same keys
    for (auto pair : remoteMap)
    {
        const String remoteKey = pair.first;
        VariantType* remoteValue = remoteArch->GetVariant(remoteKey);
        VariantType::eVariantType remoteValueType = VariantType::TYPE_NONE;
        if (nullptr != remoteValue)
        {
            remoteValueType = remoteValue->GetType();
        }

        VariantType* localValue = values->GetVariant(remoteKey);
        VariantType::eVariantType localValueType = VariantType::TYPE_NONE;
        if (nullptr != localValue)
        {
            localValueType = localValue->GetType();
        }

        // has no value
        if (nullptr != remoteValue && !isValuesChanged)
        {
            values->SetVariant(remoteKey, *remoteValue);
        }
    }

    WriteArchiveToStorage(values);
    isValuesChanged = false;
}
}
#endif
