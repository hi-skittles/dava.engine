#include "LocalNotificationListenerBridge.h"
#include "DVELocalNotificationListener.h"

#if defined(__DAVAENGINE_IPHONE__)

namespace DAVA
{
namespace Private
{
LocalNotificationListener::LocalNotificationListener(DAVA::LocalNotificationController& controller)
{
    if (nativeListener == nullptr)
    {
        DVELocalNotificationListener* listener = [[DVELocalNotificationListener alloc] autorelease];
        DAVA::PlatformApi::Ios::RegisterDVEApplicationListener(listener);

        nativeListener = listener;
    }
}

LocalNotificationListener::~LocalNotificationListener()
{
    if (nativeListener != nullptr)
    {
        DVELocalNotificationListener* listener = static_cast<DVELocalNotificationListener*>(nativeListener);
        DAVA::PlatformApi::Ios::UnregisterDVEApplicationListener(listener);

        nativeListener = nullptr;
    }
}
} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IOS__
