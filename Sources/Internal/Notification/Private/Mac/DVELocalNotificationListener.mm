#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#include "Engine/Engine.h"
#include "Engine/Window.h"
#include "Utils/NSStringUtils.h"

#import "Engine/PlatformApiMac.h"
#import "Notification/Private/Mac/DVELocalNotificationListener.h"
#import "Notification/LocalNotificationController.h"

#import <Foundation/Foundation.h>

@implementation DVELocalNotificationListener
{
    DAVA::LocalNotificationController* notificationController;
}

- (instancetype)initWithController:(DAVA::LocalNotificationController&)controller
{
    if (self = [super init])
    {
        notificationController = &controller;
    }

    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    using namespace DAVA;

    NSUserNotification* userNotification = [notification userInfo][(id) @"NSApplicationLaunchUserNotificationKey"];
    if (userNotification && (userNotification.userInfo != nil))
    {
        NSString* uid = [[userNotification userInfo] valueForKey:@"uid"];
        if (uid != nil && [uid length] != 0)
        {
            String uidStr = StringFromNSString(uid);
            auto func = [self, uidStr]() {
                notificationController->OnNotificationPressed(uidStr);
            };
            RunOnMainThreadAsync(func);
        }
    }
}

- (void)applicationDidBecomeActive:(NSNotification*)notification
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] removeAllDeliveredNotifications];
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification
{
    using namespace DAVA;

    NSString* uid = [[notification userInfo] valueForKey:@"uid"];
    if (uid != nil && [uid length] != 0)
    {
        String uidStr = StringFromNSString(uid);
        auto func = [self, uidStr]() {
            notificationController->OnNotificationPressed(uidStr);
        };
        RunOnMainThreadAsync(func);

        [[NSUserNotificationCenter defaultUserNotificationCenter] removeAllDeliveredNotifications];

        Window* w = GetPrimaryWindow();
        if (w != nullptr)
        {
            w->ActivateAsync();
        }
    }
}

@end

#endif // __DAVAENGINE_MACOS__
