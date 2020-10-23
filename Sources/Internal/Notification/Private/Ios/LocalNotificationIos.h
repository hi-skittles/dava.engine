#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Notification/Private/LocalNotificationImpl.h"

#include "Base/Message.h"

namespace DAVA
{
struct UILocalNotificationWrapper;

class LocalNotificationIOS : public LocalNotificationImpl
{
public:
    LocalNotificationIOS(const String& _id);
    ~LocalNotificationIOS() override;

    void SetAction(const String& action) override;
    void Hide() override;
    void ShowText(const String& title, const String& text, bool useSound) override;
    void ShowProgress(const String& title, const String& text, uint32 total, uint32 progress, bool useSound) override;
    void PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound) override;
    void RemoveAllDelayedNotifications() override;

public:
    UILocalNotificationWrapper* notification;
};
}

#endif // defined(__DAVAENGINE_IPHONE__)
