#include "Notification/LocalNotificationProgress.h"
#include "Notification/Private/LocalNotificationImpl.h"

namespace DAVA
{
LocalNotificationProgress::LocalNotificationProgress()
    : LocalNotification()
    , total(0)
    , progress(0)
{
}

LocalNotificationProgress::~LocalNotificationProgress()
{
    impl->Hide();
}

void LocalNotificationProgress::SetProgressCurrent(const uint32 _currentProgress)
{
    if (isVisible && (progress != _currentProgress))
    {
        isChanged = true;
        progress = _currentProgress;
    }
}

void LocalNotificationProgress::SetProgressTotal(const uint32 _total)
{
    if (total != _total)
    {
        isChanged = true;
        total = _total;
    }
}

void LocalNotificationProgress::ImplShow()
{
    impl->ShowProgress(title, text, total, progress, useSound);
}
} // namespace DAVA
