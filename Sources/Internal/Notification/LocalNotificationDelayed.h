#pragma once

#include "Notification/LocalNotification.h"

namespace DAVA
{
class LocalNotificationDelayed : public LocalNotification
{
public:
    void SetDelaySeconds(int value)
    {
        delaySeconds = value;
    }
    int GetDelaySeconds()
    {
        return delaySeconds;
    }
    void RemoveAllDelayedNotifications();
    void Post();

private:
    void ImplShow() override;

private:
    int delaySeconds;
};
} // namespace DAVA
