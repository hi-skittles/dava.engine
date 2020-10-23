#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_MACOS__)

#include "Notification/Private/LocalNotificationImpl.h"

#include "Base/Message.h"

namespace DAVA
{
struct NSUserNotificationWrapper;

class LocalNotificationMac : public LocalNotificationImpl
{
public:
    LocalNotificationMac(const String& _id);
    ~LocalNotificationMac() override;

    void SetAction(const String& action) override;
    void Hide() override;
    void ShowText(const String& title, const String& text, bool useSound) override;
    void ShowProgress(const String& title, const String& text, uint32 total, uint32 progress, bool useSound) override;
    void PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound) override;
    void RemoveAllDelayedNotifications() override;

public:
    NSUserNotificationWrapper* notification;
};
}

#endif // defined(__DAVAENGINE_MACOS__)
