#pragma once

#include "DataStorage/DataStorage.h"

#if defined(__DAVAENGINE_LINUX__) && !defined(__DAVAENGINE_STEAM__)

namespace DAVA
{
class DataStorageLinux : public IDataStorage
{
public:
    String GetStringValue(const String& key) override
    {
        return String();
    }
    int64 GetLongValue(const String& key) override
    {
        return 0;
    }
    void SetStringValue(const String& key, const String& value) override
    {
    }
    void SetLongValue(const String& key, int64 value) override
    {
    }
    void RemoveEntry(const String& key) override
    {
    }
    void Clear() override
    {
    }
    void Push() override
    {
    }
};

} // namespace DAVA

#endif // defined(__DAVAENGINE_LINUX__) && !defined(__DAVAENGINE_STEAM__)
