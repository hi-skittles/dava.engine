#pragma once

/**
    \defgroup engine_ios Engine facilities specific to iOS platform
*/

#ifndef __OBJC__
#error This file can only be included from .mm file
#endif

#if defined(__DAVAENGINE_IPHONE__)

#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>

@protocol NSObject;
@class UIApplication;
@class NSDictionary;
@class NSString;
@class NSError;
@class NSData;
@class NSURL;
@class UIView;
@class UIImage;
@class UILocalNotification;

/**
    \ingroup engine_ios
    Protocol definition for callbacks to be invoked when `UIApplicationDelegate` event occurs (applicationDidFinishLaunching,
    applicationWillTerminate, etc).
    Only subset of UIApplicationDelegate methods are mapped to the interface definition, other methods are mapped as required.

    To receive callbacks from `UIApplicationDelegate` application should declare class conforming to `DVEApplicationListener` protocol,
    implement necessary methods and register it through `RegisterDVEApplicationListener` function.

    Methods of `DVEApplicationListener` are always called in the context of UI thread (for IOS UI thread and main thread are the same).
*/
@protocol DVEApplicationListener<NSObject>
@optional
- (BOOL)application:(UIApplication*)application willFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (void)applicationDidBecomeActive:(UIApplication*)application;
- (void)applicationWillResignActive:(UIApplication*)application;
- (void)applicationDidEnterBackground:(UIApplication*)application;
- (void)applicationWillEnterForeground:(UIApplication*)application;
- (void)applicationWillTerminate:(UIApplication*)application;
- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application;

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken;
- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error;
- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo;
- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;
- (void)application:(UIApplication*)application didReceiveLocalNotification:(UILocalNotification*)notification;

- (void)application:(UIApplication*)application handleActionWithIdentifier:(NSString*)identifier
     forRemoteNotification:(NSDictionary*)userInfo
         completionHandler:(void (^)())completionHandler;
- (BOOL)application:(UIApplication*)application openURL:(NSURL*)url sourceApplication:(NSString*)sourceApplication annotation:(id)annotation;
- (BOOL)application:(UIApplication*)application continueUserActivity:(NSUserActivity*)userActivity restorationHandler:(void (^)(NSArray* restorableObjects))restorationHandler;

/*
- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification;
*/
@end

namespace DAVA
{
class Image;
class Window;
namespace PlatformApi
{
namespace Ios
{
void AddUIView(Window* targetWindow, UIView* uiview);
void RemoveUIView(Window* targetWindow, UIView* uiview);

UIView* GetUIViewFromPool(Window* targetWindow, const char* className);
void ReturnUIViewToPool(Window* targetWindow, UIView* view);

UIImage* RenderUIViewToUIImage(UIView* view);
Image* ConvertUIImageToImage(UIImage* nativeImage);

/**
    \ingroup engine_ios
    Register a callback to be invoked in response of `UIApplicationDelegate` lifecycle events.

    Application can register a callback from any thread, but callbacks are invoked in the context of UI thread.
    The best place to call this function is before calling `Engine::Run` or in `Engine::gameLoopStarted` signal handler.

    \pre `listener` should not be null pointer
    \pre Function shall not be called before `Engine::Init` or after `Engine::cleanup` signal.
*/
void RegisterDVEApplicationListener(id<DVEApplicationListener> listener);

/**
    \ingroup engine_ios
    Unregister a callback previously registered by `RegisterUIApplicationDelegateListener` function.

    Application can unregister a callback from any thread, even during callback invocation.

    \pre `listener` should be previously registered
    \pre Function shall not be called after `Engine::cleanup` signal
*/
void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener);

} // namespace Ios
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
