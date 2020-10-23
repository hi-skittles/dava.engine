#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Base/Message.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
class LocalNotification;
class LocalNotificationText;
class LocalNotificationProgress;

namespace Private
{
struct LocalNotificationListener;
}

class LocalNotificationController : public Singleton<LocalNotificationController>
{
    friend class LocalNotification;

public:
    LocalNotificationController();
    virtual ~LocalNotificationController();
    LocalNotificationProgress* const CreateNotificationProgress(const String& title = "", const String& text = "", uint32 max = 0, uint32 current = 0, bool useSound = false);
    LocalNotificationText* const CreateNotificationText(const String& title = "", const String& text = "", bool useSound = false);
    void PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound = false);
    void RemoveAllDelayedNotifications();
    bool Remove(LocalNotification* const notification);
    bool RemoveById(const String& notificationId);
    void Clear();
    void Update(float32);

    void RequestPermissions();

    LocalNotification* const GetNotificationById(const String& id);
    void OnNotificationPressed(const String& id);

private:
    Mutex notificationsListMutex;
    List<LocalNotification*> notificationsList;
    std::unique_ptr<Private::LocalNotificationListener> localListener;
};
} // namespace DAVA
