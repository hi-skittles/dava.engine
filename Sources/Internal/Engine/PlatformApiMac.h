#pragma once

/**
    \defgroup engine_mac Engine facilities specific to macOS platform
*/

#ifndef __OBJC__
#error This file can only be included from .mm file
#endif

#if defined(__DAVAENGINE_QT__) 
#elif defined(__DAVAENGINE_MACOS__)

@protocol NSObject;
@class NSNotification;
@class NSUserNotification;
@class NSUserNotificationCenter;
@class NSApplication;
@class NSDictionary;
@class NSString;
@class NSError;
@class NSData;
@class NSView;

/**
    \ingroup engine_mac
    Protocol definition for callbacks to be invoked when `NSApplicationDelegate` event occurs (applicationDidFinishLaunching,
    applicationWillTerminate, etc).
    Only subset of NSApplicationDelegate methods are mapped to the interface definition, other methods are mapped as required.

    To receive callbacks from `NSApplicationDelegate` application should declare class conforming to `DVEApplicationListener` protocol,
    implement necessary methods and register it through `RegisterDVEApplicationListener` function.

    Methods of `DVEApplicationListener` are always called in the context of UI thread (for Mac UI thread and main thread are the same).
*/
@protocol DVEApplicationListener<NSObject>
@optional
- (void)applicationWillFinishLaunching:(NSNotification*)notification;
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
- (void)applicationWillTerminate:(NSNotification*)notification;
- (void)applicationDidBecomeActive:(NSNotification*)notification;
- (void)applicationDidResignActive:(NSNotification*)notification;
- (void)application:(NSApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo;
- (void)application:(NSApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken;
- (void)application:(NSApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error;
- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification;
@end

namespace DAVA
{
class Window;
namespace PlatformApi
{
namespace Mac
{
void AddNSView(Window* targetWindow, NSView* nsview);
void RemoveNSView(Window* targetWindow, NSView* nsview);

/**
    \ingroup engine_mac
    Register a callback to be invoked in response of `NSApplicationDelegate` lifecycle events.

    Application can register a callback from any thread, but callbacks are invoked in the context of UI thread.
    The best place to call this function is before calling `Engine::Run` or in `Engine::gameLoopStarted` signal handler.
    
    \note The `listener` will also be retained. It will be released either when `UnregisterDVEApplicationListener` is called or when app exits
    \pre `listener` should not be null pointer
    \pre Function shall not be called before `Engine::Init` or after `Engine::cleanup` signal.
*/
void RegisterDVEApplicationListener(id<DVEApplicationListener> listener);

/**
    \ingroup engine_mac
    Unregister a callback previously registered by `RegisterDVEApplicationListener` function.

    Application can unregister a callback from any thread, even during callback invocation.

    \pre `listener` should be previously registered
    \pre Function shall not be called after `Engine::cleanup` signal
*/
void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener);
} // namespace Mac
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
