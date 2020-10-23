#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/PlatformApiAndroid.h"
#include "Notification/Private/LocalNotificationImpl.h"
#include "Base/Message.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
class LocalNotificationAndroid : public LocalNotificationImpl
{
public:
    LocalNotificationAndroid(const String& _id);
    void SetAction(const String& action) override;
    void Hide() override;
    void ShowText(const String& title, const String& text, const bool useSound) override;
    void ShowProgress(const String& title, const String& text, const uint32 total, const uint32 progress, bool useSound) override;
    void PostDelayedNotification(String const& title, String const& text, int delaySeconds, bool useSound) override;
    void RemoveAllDelayedNotifications() override;

private:
    Mutex javaCallMutex;
    JNI::JavaClass notificationProvider;

    Function<void(jstring, jstring, jstring, jboolean)> setText;
    Function<void(jstring, jstring, jstring, jint, jint, jboolean)> setProgress;
    Function<void(jstring)> hideNotification;
    Function<void(jstring)> enableTapAction;
    Function<void(jstring, jstring, jstring, jint, jboolean)> notifyDelayed;
    Function<void()> removeAllDelayedNotifications;
};
} // namespace DAVA
#endif // defined(__DAVAENGINE_ANDROID__)
