#include "Notification/LocalNotificationController.h"
#include "Notification/LocalNotificationProgress.h"
#include "Notification/LocalNotificationText.h"
#include "Notification/LocalNotificationDelayed.h"

#include "Notification/Private/Android/LocalNotificationAndroid.h"
#include "Notification/Private/LocalNotificationStub.h"
#include "Notification/Private/LocalNotificationImpl.h"

#include "Notification/Private/Mac/LocalNotificationListenerBridge.h"
#include "Notification/Private/Ios/LocalNotificationListenerBridge.h"
#include "Notification/Private/Win10/LocalNotificationListenerWin10.h"
#include "Notification/Private/Android/LocalNotificationListenerAndroid.h"
#include "Notification/Private/LocalNotificationListenerStub.h"

#include "Engine/Engine.h"

#include "Base/BaseTypes.h"
#include "Concurrency/LockGuard.h"

namespace DAVA
{
LocalNotificationController::LocalNotificationController()
    : localListener(std::make_unique<Private::LocalNotificationListener>(*this))
{
    Engine* engine = Engine::Instance();
    engine->update.Connect(this, &LocalNotificationController::Update);
    engine->backgroundUpdate.Connect(this, &LocalNotificationController::Update);
}

LocalNotificationController::~LocalNotificationController()
{
    Engine* engine = Engine::Instance();
    engine->update.Disconnect(this);
    engine->backgroundUpdate.Disconnect(this);

    LockGuard<Mutex> guard(notificationsListMutex);
    if (!notificationsList.empty())
    {
        for (List<LocalNotification*>::iterator it = notificationsList.begin(); it != notificationsList.end();)
        {
            LocalNotification* notification = (*it);
            it = notificationsList.erase(it);
            SafeRelease(notification);
        }
    }
}

LocalNotificationProgress* const LocalNotificationController::CreateNotificationProgress(const String& title, const String& text, uint32 maximum, uint32 current, bool useSound)
{
    LocalNotificationProgress* note = new LocalNotificationProgress();

    if (NULL != note)
    {
        note->SetText(text);
        note->SetTitle(title);
        note->SetProgressCurrent(current);
        note->SetProgressTotal(maximum);
        note->SetUseSound(useSound);
        note->SetAction(Message());

        LockGuard<Mutex> guard(notificationsListMutex);
        notificationsList.push_back(note);
    }

    return note;
}

LocalNotificationText* const LocalNotificationController::CreateNotificationText(const String& title, const String& text, bool useSound)
{
    LocalNotificationText* note = new LocalNotificationText();

    if (NULL != note)
    {
        note->SetText(text);
        note->SetTitle(title);
        note->SetAction(Message());
        note->SetUseSound(useSound);

        LockGuard<Mutex> guard(notificationsListMutex);
        notificationsList.push_back(note);
    }

    return note;
}

bool LocalNotificationController::Remove(LocalNotification* notification)
{
    LockGuard<Mutex> guard(notificationsListMutex);

    auto endIt = end(notificationsList);
    auto it = find(begin(notificationsList), endIt, notification);
    if (endIt != it)
    {
        (*it)->Release();
        notificationsList.erase(it);
        return true;
    }
    return false;
}

bool LocalNotificationController::RemoveById(const String& notificationId)
{
    LockGuard<Mutex> guard(notificationsListMutex);

    auto endIt = end(notificationsList);
    auto it = find_if(begin(notificationsList), endIt, [&notificationId](LocalNotification* note) -> bool
                      {
                          return 0 == note->GetId().compare(notificationId);
                      });

    if (it != endIt)
    {
        (*it)->Release();
        notificationsList.erase(it);
        return true;
    }

    return false;
}

void LocalNotificationController::Clear()
{
    LockGuard<Mutex> guard(notificationsListMutex);
    for (auto note : notificationsList)
    {
        note->Release();
    }
    notificationsList.clear();
}

void LocalNotificationController::Update(float32)
{
    LockGuard<Mutex> guard(notificationsListMutex);

    for (auto notification : notificationsList)
    {
        notification->Update();
    }
}

void LocalNotificationController::RequestPermissions()
{
    LocalNotificationImpl::RequestPermissions();
}

LocalNotification* const LocalNotificationController::GetNotificationById(const String& id)
{
    LockGuard<Mutex> guard(notificationsListMutex);
    for (auto notification : notificationsList)
    {
        if (notification->GetId().compare(id) == 0)
        {
            return notification;
        }
    }

    return NULL;
}

void LocalNotificationController::OnNotificationPressed(const String& id)
{
    LocalNotification* const notification = GetNotificationById(id);
    if (NULL != notification)
    {
        notification->RunAction();
    }
}

void LocalNotificationController::PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound)
{
    LocalNotificationDelayed* notification = new LocalNotificationDelayed();
    notification->SetTitle(title);
    notification->SetText(text);
    notification->SetDelaySeconds(delaySeconds);
    notification->SetUseSound(useSound);
    notification->Post();
    SafeRelease(notification);
}

void LocalNotificationController::RemoveAllDelayedNotifications()
{
    LocalNotificationDelayed* notification = new LocalNotificationDelayed();
    notification->RemoveAllDelayedNotifications();
    SafeRelease(notification);
}
} // namespace DAVA
