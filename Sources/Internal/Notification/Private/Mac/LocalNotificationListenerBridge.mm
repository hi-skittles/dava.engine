#include "LocalNotificationListenerBridge.h"
#include "DVELocalNotificationListener.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

namespace DAVA
{
namespace Private
{
LocalNotificationListener::LocalNotificationListener(DAVA::LocalNotificationController& controller)
{
    if (nativeListener == nullptr)
    {
        DVELocalNotificationListener* listener = [[[DVELocalNotificationListener alloc] initWithController:controller] autorelease];
        DAVA::PlatformApi::Mac::RegisterDVEApplicationListener(listener);

        nativeListener = listener;
    }
}

LocalNotificationListener::~LocalNotificationListener()
{
    if (nativeListener != nullptr)
    {
        DVELocalNotificationListener* listener = static_cast<DVELocalNotificationListener*>(nativeListener);
        DAVA::PlatformApi::Mac::UnregisterDVEApplicationListener(listener);

        nativeListener = nullptr;
    }
}
} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
