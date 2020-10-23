#include "DataStorage/DataStorage.h"

#include "Utils/Utils.h"
namespace DAVA
{

#if defined(__DAVAENGINE_WIN_UAP__)

class DataStorageWin : public IDataStorage
{
public:
    DataStorageWin();
    String GetStringValue(const String& key) override;
    int64 GetLongValue(const String& key) override;
    void SetStringValue(const String& key, const String& value) override;
    void SetLongValue(const String& key, int64 value) override;
    void RemoveEntry(const String& key) override;
    void Clear() override;
    void Push() override
    {
    }

private:
    Windows::Storage::ApplicationDataContainer ^ roamingSettings = nullptr;
};
#endif // Win UAP

#if defined(__DAVAENGINE_WIN32__) && !defined(__DAVAENGINE_STEAM__)

class DataStorageWin : public IDataStorage
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

#endif //windows
}