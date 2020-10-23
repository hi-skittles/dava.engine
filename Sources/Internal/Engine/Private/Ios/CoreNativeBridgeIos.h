#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <Foundation/Foundation.h>
#import <UIKit/UIApplication.h>

#include "Concurrency/Mutex.h"
#include "Engine/Private/EnginePrivateFwd.h"

@class NSObject;
@class NSData;
@class NSDictionary;
@class NSError;
@class NSMutableArray;
@class NSString;
@class UIApplication;
@class UILocalNotification;
@class NSURL;

@class ObjectiveCInterop;
@class NotificationBridge;

@protocol DVEApplicationListener;

namespace DAVA
{
namespace Private
{
// Bridge between C++ and Objective-C for iOS's PlatformCore class
// Responsibilities:
//  - holds neccesary Objective-C objects
//
// CoreNativeBridge is friend of iOS's PlatformCore
struct CoreNativeBridge final
{
    CoreNativeBridge(PlatformCore* core);
    ~CoreNativeBridge();

    void Run();
    void OnFrameTimer();

    // Callbacks from AppDelegate
    BOOL ApplicationWillFinishLaunchingWithOptions(UIApplication* app, NSDictionary* launchOptions);
    BOOL ApplicationDidFinishLaunchingWithOptions(UIApplication* app, NSDictionary* launchOptions);
    BOOL ApplicationOpenUrl(NSURL* url);
    void ApplicationDidBecomeActive(UIApplication* app);
    void ApplicationWillResignActive(UIApplication* app);
    void ApplicationDidEnterBackground(UIApplication* app);
    void ApplicationWillEnterForeground(UIApplication* app);
    void ApplicationWillTerminate(UIApplication* app);
    void ApplicationDidReceiveMemoryWarning(UIApplication* app);
    void ApplicationDidReceiveLocalNotification(UIApplication* app, UILocalNotification* notification);
    void DidReceiveRemoteNotification(UIApplication* app, NSDictionary* userInfo);
    void DidReceiveRemoteNotificationFetchCompletionHandler(UIApplication* app, NSDictionary* userInfo, void (^completionHandler)(UIBackgroundFetchResult));
    void DidRegisterForRemoteNotificationsWithDeviceToken(UIApplication* app, NSData* deviceToken);
    void DidFailToRegisterForRemoteNotificationsWithError(UIApplication* app, NSError* error);
    void DidReceiveLocalNotification(UIApplication* app, UILocalNotification* notification);
    void HandleActionWithIdentifier(UIApplication* app, NSString* identifier, NSDictionary* userInfo, id completionHandler);
    BOOL OpenURL(UIApplication* app, NSURL* url, NSString* sourceApplication, id annotation);
    BOOL ContinueUserActivity(UIApplication* application, NSUserActivity* userActivity, id restorationHandler);

    void GameControllerDidConnected();
    void GameControllerDidDisconnected();

    void RegisterDVEApplicationListener(id<DVEApplicationListener> listener);
    void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener);

    enum eNotificationType
    {
        ON_DID_FINISH_LAUNCHING,
        ON_WILL_FINISH_LAUNCHING,
        ON_DID_BECOME_ACTIVE,
        ON_WILL_RESIGN_ACTIVE,
        ON_DID_ENTER_BACKGROUND,
        ON_WILL_ENTER_FOREGROUND,
        ON_WILL_TERMINATE,
        ON_DID_RECEIVE_MEMORY_WARNING,
        ON_DID_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_TOKEN,
        ON_DID_FAIL_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_ERROR,
        ON_DID_RECEIVE_REMOTE_NOTIFICATION,
        ON_DID_RECEIVE_REMOTE_NOTIFICATION_FETCH_COMPLETION_HANDLER,
        ON_DID_RECEIVE_LOCAL_NOTIFICATION,
        ON_HANDLE_ACTION_WITH_IDENTIFIER,
        ON_OPEN_URL,
        ON_CONTINUE_USER_ACTIVITY,
    };

    BOOL NotifyListeners(eNotificationType type, NSObject* arg1 = nullptr, NSObject* arg2 = nullptr, NSObject* arg3 = nullptr, id arg4 = nullptr);

    bool CollectActivationFilenames(NSURL* url);

    PlatformCore* core = nullptr;
    EngineBackend* engineBackend = nullptr;
    MainDispatcher* mainDispatcher = nullptr;
    ObjectiveCInterop* objcInterop = nullptr;

    // Even if request to open URL comes with didFinishLaunchingWithOptions system calls
    // openURL of UIApplicationDelegate implementation. This flags prevents collecting
    // startup file twice.
    bool ignoreOpenUrlJustAfterStartup = false;

    Mutex listenersMutex;
    NSMutableArray* appDelegateListeners;

    int64 goBackgroundTimeRelativeToBoot = 0;
    int64 goBackgroundTime = 0;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
