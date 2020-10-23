#include "Notification/LocalNotificationText.h"
#include "Notification/Private/LocalNotificationImpl.h"

namespace DAVA
{
void LocalNotificationText::ImplShow()
{
    impl->ShowText(title, text, useSound);
}
} // namespace DAVA
