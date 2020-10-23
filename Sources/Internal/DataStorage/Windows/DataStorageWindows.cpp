#include "DataStorage/DataStorage.h"
#include "DataStorageWindows.h"

namespace DAVA
{

#if defined(__DAVAENGINE_WINDOWS__) && !defined(__DAVAENGINE_STEAM__)

IDataStorage* DataStorage::Create()
{
    return new DataStorageWin();
}

#endif

#if defined(__DAVAENGINE_WIN_UAP__)

DataStorageWin::DataStorageWin()
{
    using ::Windows::Storage::ApplicationData;

    roamingSettings = ApplicationData::Current->RoamingSettings;
    DVASSERT(nullptr != roamingSettings); // something goes wrong
}

String DataStorageWin::GetStringValue(const String& key)
{
    using ::Windows::Foundation::IPropertyValue;
    using ::Windows::Foundation::PropertyType;

    auto values = roamingSettings->Values;
    IPropertyValue ^ value = safe_cast<IPropertyValue ^>(values->Lookup(StringToRTString(key)));

    if (nullptr != value && PropertyType::String == value->Type)
    {
        return RTStringToString(value->GetString());
    }
    else
    {
        return String();
    }
}

int64 DataStorageWin::GetLongValue(const String& key)
{
    using ::Windows::Foundation::IPropertyValue;
    using ::Windows::Foundation::PropertyType;

    auto values = roamingSettings->Values;
    IPropertyValue ^ value = safe_cast<IPropertyValue ^>(values->Lookup(StringToRTString(key)));
    if (nullptr != value && PropertyType::Int64 == value->Type)
    {
        return value->GetInt64();
    }
    else
    {
        return 0;
    }
}

void DataStorageWin::SetStringValue(const String& key, const String& value)
{
    using ::Windows::Foundation::PropertyValue;

    auto values = roamingSettings->Values;
    values->Insert(StringToRTString(key), dynamic_cast<PropertyValue ^>(PropertyValue::CreateString(StringToRTString(value))));
}

void DataStorageWin::SetLongValue(const String& key, int64 value)
{
    using ::Windows::Foundation::PropertyValue;

    auto values = roamingSettings->Values;
    values->Insert(StringToRTString(key), dynamic_cast<PropertyValue ^>(PropertyValue::CreateInt64(value)));
}

void DataStorageWin::RemoveEntry(const String& key)
{
    auto values = roamingSettings->Values;
    values->Remove(StringToRTString(key));
}

void DataStorageWin::Clear()
{
    auto values = roamingSettings->Values;
    values->Clear();
}
#endif
}