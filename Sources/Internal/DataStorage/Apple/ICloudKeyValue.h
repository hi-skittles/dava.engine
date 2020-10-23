#ifndef __ICLOUD_KEYVALUE_H__
#define __ICLOUD_KEYVALUE_H__

#include "DataStorage/DataStorage.h"

namespace DAVA
{

#if defined(__DAVAENGINE_APPLE__) && !defined(__DAVAENGINE_STEAM__)

class ICloudKeyValue : public IDataStorage
{
public:
    ICloudKeyValue();

public: // IDataStorage implementation
    String GetStringValue(const String& key) override;
    int64 GetLongValue(const String& key) override;
    void SetStringValue(const String& key, const String& value) override;
    void SetLongValue(const String& key, int64 value) override;
    void RemoveEntry(const String& key) override;
    void Clear() override;
    void Push() override;

private:
    void Sync();
};

#endif

} //namespace DAVA

#endif // __DATA_VAULT_H__
