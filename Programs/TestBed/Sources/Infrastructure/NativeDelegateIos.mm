#include "Infrastructure/NativeDelegateIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <Engine/PlatformApiIos.h>
#import <Foundation/Foundation.h>

#include <Logger/Logger.h>
#include <Utils/NSStringUtils.h>

@interface NativeDelegateIos : NSObject<DVEApplicationListener>
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application;
- (void)applicationDidBecomeActive:(UIApplication*)application;
- (void)applicationWillResignActive:(UIApplication*)application;
- (void)applicationDidEnterBackground:(UIApplication*)application;
- (void)applicationWillEnterForeground:(UIApplication*)application;
- (void)applicationWillTerminate:(UIApplication*)application;
@end

@implementation NativeDelegateIos

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    using namespace DAVA;
    Logger::Debug("TestBed.NativeDelegateIos::didFinishLaunchingWithOptions: enter");
    Logger::Debug("    launch options:");
    for (NSString* key in launchOptions)
    {
        String k = StringFromNSString(key);
        String d = StringFromNSString([launchOptions[key] description]);
        Logger::Debug("        %s: %d", k.c_str(), d.c_str());
    }
    Logger::Debug("TestBed.NativeDelegateIos::didFinishLaunchingWithOptions: leave");

    return YES;
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationDidReceiveMemoryWarning");
}

- (void)applicationDidBecomeActive:(UIApplication*)application;
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationDidBecomeActive");
}

- (void)applicationWillResignActive:(UIApplication*)application;
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationWillResignActive");
}

- (void)applicationWillEnterForeground:(UIApplication*)application;
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationWillEnterForeground");
}

- (void)applicationDidEnterBackground:(UIApplication*)application;
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationDidEnterBackground");
}

- (void)applicationWillTerminate:(UIApplication*)application;
{
    DAVA::Logger::Debug("TestBed.NativeDelegateIos::applicationWillTerminate");
}

@end

void RegisterIosApplicationListener()
{
    // Will be retained inside of implementation and released when app exits
    DAVA::PlatformApi::Ios::RegisterDVEApplicationListener([[[NativeDelegateIos alloc] init] autorelease]);
}

#endif // __DAVAENGINE_IPHONE__
