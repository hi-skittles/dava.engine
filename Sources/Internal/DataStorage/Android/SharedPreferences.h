#pragma once

#include "DataStorage/DataStorage.h"
#include "Engine/PlatformApiAndroid.h"

namespace DAVA
{

#if defined(__DAVAENGINE_ANDROID__)

class SharedPreferences : public IDataStorage
{
public:
    SharedPreferences();
    ~SharedPreferences();

public: // IDataStorage implementation
    String GetStringValue(const String& key) override;
    int64 GetLongValue(const String& key) override;
    void SetStringValue(const String& key, const String& value) override;
    void SetLongValue(const String& key, int64 value) override;
    void RemoveEntry(const String& key) override;
    void Clear() override;
    void Push() override;

private:
    JNI::JavaClass jniSharedPreferences;
    Function<jobject(void)> getSharedPreferences;

    jobject preferencesObject;

    Function<void(jstring, jstring)> putString;
    Function<jstring(jstring, jstring)> getString;
    Function<void(jstring, jlong)> putLong;
    Function<jlong(jstring, jlong)> getLong;
    Function<void(jstring)> remove;
    Function<void(void)> clear;
    Function<void(void)> push;
};

#endif

} //namespace DAVA
