#include "Infrastructure/NativeDelegateMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <Foundation/Foundation.h>

#include <Engine/PlatformApiMac.h>
#include <Logger/Logger.h>
#include <Utils/NSStringUtils.h>

@interface NativeDelegateMac : NSObject<DVEApplicationListener>
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
- (void)applicationWillTerminate:(NSNotification*)notification;
- (void)applicationDidBecomeActive:(NSNotification*)notification;
- (void)applicationDidResignActive:(NSNotification*)notification;
- (void)application:(NSApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo;
- (void)application:(NSApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken;
- (void)application:(NSApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error;
@end

@implementation NativeDelegateMac

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    using namespace DAVA;
    String name = StringFromNSString([notification name]);
    Logger::Debug("TestBed.NativeDelegateMac::applicationDidFinishLaunching: enter");
    Logger::Debug("    notification name=%s", name.c_str());
    Logger::Debug("    notification dictionary:");
    NSDictionary* dict = [notification userInfo];
    for (NSString* key in dict)
    {
        String k = StringFromNSString(key);
        String d = StringFromNSString([dict[key] description]);
        Logger::Debug("        %s: %d", k.c_str(), d.c_str());
    }
    Logger::Debug("TestBed.NativeDelegateMac::applicationDidFinishLaunching: leave");
}

- (void)applicationDidBecomeActive:(NSNotification*)notification
{
    DAVA::Logger::Debug("TestBed.NativeDelegateMac::applicationDidBecomeActive");
}

- (void)applicationDidResignActive:(NSNotification*)notification
{
    DAVA::Logger::Debug("TestBed.NativeDelegateMac::applicationDidResignActive");
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    DAVA::Logger::Debug("TestBed.NativeDelegateMac::applicationWillTerminate");
}

- (void)application:(NSApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo;
{
    using namespace DAVA;
    Logger::Debug("TestBed.NativeDelegateMac::didReceiveRemoteNotification: enter");
    Logger::Debug("    dictionary:");
    for (NSString* key in userInfo)
    {
        String k = StringFromNSString(key);
        String d = StringFromNSString([userInfo[key] description]);
        Logger::Debug("        %s: %d", k.c_str(), d.c_str());
    }
    Logger::Debug("TestBed.NativeDelegateMac::didReceiveRemoteNotification: leave");
}

- (void)application:(NSApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
    DAVA::Logger::Debug("TestBed.NativeDelegateMac::didRegisterForRemoteNotificationsWithDeviceToken");
}

- (void)application:(NSApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error
{
    using namespace DAVA;
    String descr = StringFromNSString([error localizedDescription]);
    DAVA::Logger::Debug("TestBed.NativeDelegateMac::didFailToRegisterForRemoteNotificationsWithError: %s", descr.c_str());
}

@end

void RegisterMacApplicationListener()
{
    // Will be retained inside of implementation and released when app exits
    DAVA::PlatformApi::Mac::RegisterDVEApplicationListener([[[NativeDelegateMac alloc] init] autorelease]);
}

#endif // __DAVAENGINE_MACOS__
