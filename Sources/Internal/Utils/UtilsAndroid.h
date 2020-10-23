#pragma once

#include "Base/BaseTypes.h"
#if defined(__DAVAENGINE_ANDROID__)
#include "Engine/PlatformApiAndroid.h"

namespace DAVA
{
class JniUtils
{
public:
    JniUtils();
    bool DisableSleepTimer();
    bool EnableSleepTimer();
    void OpenURL(const String& url);
    String GenerateGUID();

private:
    JNI::JavaClass jniUtils;
    Function<void()> disableSleepTimer;
    Function<void()> enableSleepTimer;
    Function<void(jstring)> openURL;
    Function<jstring()> generateGUID;
};
}

#endif //__DAVAENGINE_ANDROID__
