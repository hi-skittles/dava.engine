#pragma once

#include "Notification/LocalNotification.h"

namespace DAVA
{
class LocalNotificationText : public LocalNotification
{
private:
    void ImplShow() override;
};
} // namespace DAVA
