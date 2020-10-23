#include "Engine/Private/Ios/AppDelegateIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/Ios/CoreNativeBridgeIos.h"

namespace DAVA
{
namespace Private
{
extern CoreNativeBridge* coreNativeBridge;
}
}

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application willFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    bridge = DAVA::Private::coreNativeBridge;
    return bridge->ApplicationWillFinishLaunchingWithOptions(application, launchOptions);
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    return bridge->ApplicationDidFinishLaunchingWithOptions(application, launchOptions);
}

// Apple recommends using `application:openURL:options:` selector to handle opening URLs but this selector
// was introduced starting from iOS 9. But engine's client now uses deprecated but still usable
// `application:openURL:sourceApplication:annotation:` selector and knows nothing about new selector.
// So comment new handler and use old handler.
// TODO: later make use of `application:openURL:options:`
//- (BOOL)application:(UIApplication*)app openURL:(NSURL*)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options
//{
//    return bridge->ApplicationOpenUrl(url);
//}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    bridge->ApplicationDidBecomeActive(application);
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    bridge->ApplicationWillResignActive(application);
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    bridge->ApplicationDidEnterBackground(application);
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    bridge->ApplicationWillEnterForeground(application);
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    bridge->ApplicationWillTerminate(application);
}

- (UIInterfaceOrientationMask)application:(UIApplication*)application supportedInterfaceOrientationsForWindow:(UIWindow*)window
{
    // This method returns the total set of interface orientations supported by the app.
    // To determine whether to rotate screen system intersects orientations reported by this method
    // and UIViewController's supportedInterfaceOrientations.
    // For now use hardcoded landscape orientation here and in [RenderViewController supportedInterfaceOrientations].
    // https://developer.apple.com/reference/uikit/uiapplicationdelegate/1623107-application?language=objc
    // https://developer.apple.com/reference/uikit/uiviewcontroller/1621435-supportedinterfaceorientations?language=objc
    // TODO: provide public methods to set orientations desired by client application
    return UIInterfaceOrientationMaskLandscape;
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    bridge->ApplicationDidReceiveMemoryWarning(application);
}

- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo
{
    bridge->DidReceiveRemoteNotification(application, userInfo);
}

- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo fetchCompletionHandler:(void (^)(UIBackgroundFetchResult result))completionHandler
{
    bridge->DidReceiveRemoteNotificationFetchCompletionHandler(application, userInfo, completionHandler);
}

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
    bridge->DidRegisterForRemoteNotificationsWithDeviceToken(application, deviceToken);
}

- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error
{
    bridge->DidFailToRegisterForRemoteNotificationsWithError(application, error);
}

- (void)application:(UIApplication*)application didReceiveLocalNotification:(UILocalNotification*)notification
{
    bridge->ApplicationDidReceiveLocalNotification(application, notification);
}

- (void)application:(UIApplication*)application handleActionWithIdentifier:(NSString*)identifier
     forRemoteNotification:(NSDictionary*)userInfo
         completionHandler:(void (^)())completionHandler
{
    bridge->HandleActionWithIdentifier(application, identifier, userInfo, completionHandler);
}

- (BOOL)application:(UIApplication*)application openURL:(NSURL*)url sourceApplication:(NSString*)sourceApplication annotation:(id)annotation
{
    return bridge->OpenURL(application, url, sourceApplication, annotation);
}

- (BOOL)application:(UIApplication*)application continueUserActivity:(NSUserActivity*)userActivity restorationHandler:(void (^)(NSArray* restorableObjects))restorationHandler
{
    return bridge->ContinueUserActivity(application, userActivity, restorationHandler);
}

@end

#endif // __DAVAENGINE_IPHONE__
