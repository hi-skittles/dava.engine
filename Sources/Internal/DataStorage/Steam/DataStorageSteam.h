#ifndef DATA_STORAGE_STEAM_H
#define DATA_STORAGE_STEAM_H

#if defined(__DAVAENGINE_STEAM__)
#include "DataStorage/DataStorage.h"
#include "Utils/Utils.h"
#include "Platform/Steam.h"

class ISteamRemoteStorage;
namespace DAVA
{
class DynamicMemoryFile;
class DataStorageSteam : public IDataStorage
{
private:
    const String storageFileName = "CloudArchive";

public:
    DataStorageSteam();
    String GetStringValue(const String& key) override;
    int64 GetLongValue(const String& key) override;
    void SetStringValue(const String& key, const String& value) override;
    void SetLongValue(const String& key, int64 value) override;
    void RemoveEntry(const String& key) override;
    void Clear() override;
    void Push() override;

private:
    ScopedPtr<KeyedArchive> ReadArchFromStorage() const;
    void WriteArchiveToStorage(const ScopedPtr<KeyedArchive> arch) const;

    ISteamRemoteStorage* remoteStorage = nullptr;
    ScopedPtr<KeyedArchive> values;
    bool isValuesChanged = false;
};
}

#endif //__DAVAENGINE_STEAM__
#endif
