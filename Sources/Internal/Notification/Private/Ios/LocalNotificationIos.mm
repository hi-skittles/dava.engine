#include "Notification/Private/Ios/LocalNotificationIos.h"
#include "Utils/StringFormat.h"

#if defined(__DAVAENGINE_IPHONE__)

#import "Notification/Private/LocalNotificationImpl.h"
#import <UIKit/UIApplication.h>
#import <UIKit/UILocalNotification.h>
#import <UIKit/UIUserNotificationSettings.h>
#import "Utils/NSStringUtils.h"

namespace DAVA
{
struct UILocalNotificationWrapper
{
    UILocalNotification* impl = NULL;
};

LocalNotificationIOS::LocalNotificationIOS(const String& _id)
    : notification(NULL)
{
    notificationId = _id;
}

LocalNotificationIOS::~LocalNotificationIOS()
{
    if (notification)
    {
        if (notification->impl)
        {
            [notification->impl release];
        }
        delete notification;
    }
}

void LocalNotificationIOS::SetAction(const String& _action)
{
}

void LocalNotificationIOS::Hide()
{
    if (NULL != notification)
    {
        NSString* uid = NSStringFromString(notificationId);
        bool scheduledNotificationFoundAndRemoved = false;
        for (UILocalNotification* n in [[UIApplication sharedApplication] scheduledLocalNotifications])
        {
            NSDictionary* userInfo = n.userInfo;
            if (userInfo && [userInfo[@"uid"] isEqual:uid])
            {
                //[UIApplication sharedApplication] cancel
                scheduledNotificationFoundAndRemoved = true;
            }
        }
        [[UIApplication sharedApplication] cancelLocalNotification:notification->impl];
    }
}

void LocalNotificationIOS::ShowText(const String& title, const String& text, bool useSound)
{
    if (NULL == notification)
    {
        notification = new UILocalNotificationWrapper();
        notification->impl = [[UILocalNotification alloc] init];
    }

    notification->impl.alertBody = NSStringFromString(text);

    notification->impl.userInfo = @{ @"uid" : NSStringFromString(notificationId),
                                     @"action" : @"test action" };

    [[UIApplication sharedApplication] cancelLocalNotification:notification->impl];

    if (useSound)
    {
        notification->impl.soundName = UILocalNotificationDefaultSoundName;
    }

    [[UIApplication sharedApplication] scheduleLocalNotification:notification->impl];
}

void LocalNotificationIOS::ShowProgress(const String& title, const String& text, uint32 total, uint32 progress, bool useSound)
{
    double percentage = (static_cast<double>(progress) / total) * 100.0;
    String titleText = title + Format(" %.02f%%", percentage);

    ShowText(titleText, text, useSound);
}

void LocalNotificationIOS::PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound)
{
    UILocalNotification* notification = [[[UILocalNotification alloc] init] autorelease];
    notification.alertBody = NSStringFromString(text);
    notification.timeZone = [NSTimeZone defaultTimeZone];
    notification.fireDate = [NSDate dateWithTimeIntervalSinceNow:delaySeconds];

    if (useSound)
    {
        notification.soundName = UILocalNotificationDefaultSoundName;
    }

    [[UIApplication sharedApplication] scheduleLocalNotification:notification];
}

void LocalNotificationIOS::RemoveAllDelayedNotifications()
{
    for (UILocalNotification* notification in [[UIApplication sharedApplication] scheduledLocalNotifications])
    {
        [[UIApplication sharedApplication] cancelLocalNotification:notification];
    }
}

LocalNotificationImpl* LocalNotificationImpl::Create(const String& _id)
{
    return new LocalNotificationIOS(_id);
}

void LocalNotificationImpl::RequestPermissions()
{
// https://developer.apple.com/reference/uikit/uiapplication/1622932-registerusernotificationsettings
// available 8.0 and later
    
#if defined(__IPHONE_8_0)
    NSString* version = [[UIDevice currentDevice] systemVersion];
    if ([version compare:@"8.0" options:NSNumericSearch] != NSOrderedAscending)
    {
        UIApplication* app = [UIApplication sharedApplication];
        UIUserNotificationSettings* currentSettings = [app currentUserNotificationSettings];
        UIUserNotificationType notifications = (UIUserNotificationTypeAlert | UIUserNotificationTypeBadge | UIUserNotificationTypeSound);

        if (currentSettings.types != notifications)
        {
            if ([UIApplication instancesRespondToSelector:@selector(registerUserNotificationSettings:)])
            {
                UIUserNotificationSettings* newSettings = [UIUserNotificationSettings settingsForTypes:notifications categories:nil];
                [app registerUserNotificationSettings:newSettings];
            }
        }
    }
#endif
}
}
#endif // defined(__DAVAENGINE_IPHONE__)
