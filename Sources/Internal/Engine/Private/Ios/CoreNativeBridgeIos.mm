#include "Engine/Private/Ios/CoreNativeBridgeIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Engine.h"
#include "Engine/PlatformApiIos.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Ios/PlatformCoreIos.h"
#include "Engine/Private/Ios/WindowImplIos.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

#include "Logger/Logger.h"
#include "Time/SystemTimer.h"
#include "Utils/NSStringUtils.h"

#import <UIKit/UIKit.h>

// Objective-C class used for interoperation between Objective-C and C++.
// CADisplayLink, NSNotificationCenter, etc expect Objective-C selectors to notify about events
// so this class installs Objective-C handlers which transfer control into C++ class.
@interface ObjectiveCInterop : NSObject
{
    DAVA::Private::CoreNativeBridge* bridge;
    CADisplayLink* displayLink;
    DAVA::int32 currentFPS;
}

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge;
- (void)setDisplayLinkPreferredFPS:(DAVA::int32)fps;
- (void)pauseDisplayLink;
- (void)resumeDisplayLink;
- (void)cancelDisplayLink;
- (void)enableGameControllerObserver:(BOOL)enable;

@end

@implementation ObjectiveCInterop

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nil)
    {
        bridge = nativeBridge;
        currentFPS = 60;
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkTimerFired:)];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [self setDisplayLinkPreferredFPS:currentFPS];
    }
    return self;
}

- (void)setDisplayLinkPreferredFPS:(DAVA::int32)fps
{
    if (currentFPS != fps)
    {
        NSString* currSysVer = [[UIDevice currentDevice] systemVersion];
        if ([currSysVer compare:@"10.0" options:NSNumericSearch] != NSOrderedAscending)
        {
            [displayLink setPreferredFramesPerSecond:fps];
        }
        else
        {
            DAVA::int32 interval = DAVA::Max(DAVA::int32(60.0 / fps + 0.5), 1);
            [displayLink setFrameInterval:interval];
        }

        currentFPS = fps;
    }
}

- (void)pauseDisplayLink
{
    displayLink.paused = YES;
}

- (void)resumeDisplayLink
{
    displayLink.paused = NO;
}

- (void)cancelDisplayLink
{
    [displayLink invalidate];
}

- (void)displayLinkTimerFired:(CADisplayLink*)dispLink
{
    bridge->OnFrameTimer();
}

- (void)enableGameControllerObserver:(BOOL)enable
{
    if (enable)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(gameControllerDidConnected)
                                                     name:@"GCControllerDidConnectNotification"
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(gameControllerDidDisconnected)
                                                     name:@"GCControllerDidDisconnectNotification"
                                                   object:nil];
    }
    else
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self];
    }
}

- (void)gameControllerDidConnected
{
    bridge->GameControllerDidConnected();
}

- (void)gameControllerDidDisconnected
{
    bridge->GameControllerDidDisconnected();
}

@end

extern NSAutoreleasePool* preMainLoopReleasePool;

namespace DAVA
{
namespace Private
{
// UIApplicationMain instantiates UIApplicationDelegate-derived class and user cannot
// create UIApplicationDelegate-derived class, pass init parameters and set it to UIApplication.
// AppDelegate will receive pointer to CoreNativeBridge instance through nativeBridge global
// variable
CoreNativeBridge* coreNativeBridge = nullptr;

CoreNativeBridge::CoreNativeBridge(PlatformCore* core)
    : core(core)
    , engineBackend(core->engineBackend)
    , mainDispatcher(core->dispatcher)
{
    coreNativeBridge = this;
    appDelegateListeners = [[NSMutableArray alloc] init];
}

CoreNativeBridge::~CoreNativeBridge()
{
    [appDelegateListeners release];
}

void CoreNativeBridge::Run()
{
    [preMainLoopReleasePool drain];

    @autoreleasepool
    {
        // UIApplicationMain never returns
        ::UIApplicationMain(0, nil, nil, @"AppDelegate");
    }
}

void CoreNativeBridge::OnFrameTimer()
{
    if (!EngineBackend::showingModalMessageBox)
    {
        int32 fps = core->OnFrame();

        [objcInterop setDisplayLinkPreferredFPS:fps];
    }
}

BOOL CoreNativeBridge::ApplicationWillFinishLaunchingWithOptions(UIApplication* app, NSDictionary* launchOptions)
{
    Logger::FrameworkDebug("******** applicationWillFinishLaunchingWithOptions");

    return NotifyListeners(ON_WILL_FINISH_LAUNCHING, app, launchOptions);
}

BOOL CoreNativeBridge::ApplicationDidFinishLaunchingWithOptions(UIApplication* app, NSDictionary* launchOptions)
{
    Logger::FrameworkDebug("******** applicationDidFinishLaunchingWithOptions");

    NSURL* launchUrl = static_cast<NSURL*>(launchOptions[UIApplicationLaunchOptionsURLKey]);
    if (launchUrl != nil)
    {
        CollectActivationFilenames(launchUrl);
        ignoreOpenUrlJustAfterStartup = true;
    }

    engineBackend->OnGameLoopStarted();

    WindowImpl* primaryWindowImpl = EngineBackend::GetWindowImpl(engineBackend->GetPrimaryWindow());
    primaryWindowImpl->Create();

    objcInterop = [[ObjectiveCInterop alloc] init:this];
    [objcInterop enableGameControllerObserver:YES];

    return NotifyListeners(ON_DID_FINISH_LAUNCHING, app, launchOptions);
}

BOOL CoreNativeBridge::ApplicationOpenUrl(NSURL* url)
{
    if (!ignoreOpenUrlJustAfterStartup && CollectActivationFilenames(url))
    {
        engineBackend->OnFileActivated();
        return YES;
    }
    ignoreOpenUrlJustAfterStartup = false;
    return NO;
}

bool CoreNativeBridge::CollectActivationFilenames(NSURL* url)
{
    if ([[url scheme] isEqualToString:@"file"])
    {
        engineBackend->AddActivationFilename(StringFromNSString([url path]));
        return true;
    }
    return false;
}

void CoreNativeBridge::ApplicationDidBecomeActive(UIApplication* app)
{
    Logger::FrameworkDebug("******** applicationDidBecomeActive");

    core->didBecomeResignActive.Emit(true);
    NotifyListeners(ON_DID_BECOME_ACTIVE, app);
}

void CoreNativeBridge::ApplicationWillResignActive(UIApplication* app)
{
    Logger::FrameworkDebug("******** applicationWillResignActive");

    core->didBecomeResignActive.Emit(false);
    NotifyListeners(ON_WILL_RESIGN_ACTIVE, app);
}

void CoreNativeBridge::ApplicationDidEnterBackground(UIApplication* app)
{
    core->didEnterForegroundBackground.Emit(false);
    NotifyListeners(ON_DID_ENTER_BACKGROUND, app);

    mainDispatcher->SendEvent(MainDispatcherEvent(MainDispatcherEvent::APP_SUSPENDED)); // Blocking call !!!

    [objcInterop pauseDisplayLink];

    goBackgroundTimeRelativeToBoot = SystemTimer::GetSystemUptimeUs();
    goBackgroundTime = SystemTimer::GetUs();
}

void CoreNativeBridge::ApplicationWillEnterForeground(UIApplication* app)
{
    mainDispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::APP_RESUMED));
    core->didEnterForegroundBackground.Emit(true);

    NotifyListeners(ON_WILL_ENTER_FOREGROUND, app);

    [objcInterop resumeDisplayLink];

    int64 timeSpentInBackground1 = SystemTimer::GetSystemUptimeUs() - goBackgroundTimeRelativeToBoot;
    int64 timeSpentInBackground2 = SystemTimer::GetUs() - goBackgroundTime;

    Logger::Debug("Time spent in background %lld us (reported by SystemTimer %lld us)", timeSpentInBackground1, timeSpentInBackground2);
    // Do adjustment only if SystemTimer has stopped ticking
    if (timeSpentInBackground1 - timeSpentInBackground2 > 500000l)
    {
        EngineBackend::AdjustSystemTimer(timeSpentInBackground1 - timeSpentInBackground2);
    }
}

void CoreNativeBridge::ApplicationWillTerminate(UIApplication* app)
{
    Logger::FrameworkDebug("******** applicationWillTerminate");

    NotifyListeners(ON_WILL_TERMINATE, app);

    [objcInterop cancelDisplayLink];
    [objcInterop enableGameControllerObserver:NO];

    WindowImpl* primaryWindowImpl = EngineBackend::GetWindowImpl(engineBackend->GetPrimaryWindow());
    primaryWindowImpl->Close(true);
}

void CoreNativeBridge::ApplicationDidReceiveMemoryWarning(UIApplication* app)
{
    Logger::FrameworkDebug("******** applicationDidReceiveMemoryWarning");

    mainDispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::LOW_MEMORY));

    NotifyListeners(ON_DID_RECEIVE_MEMORY_WARNING, app);
}

void CoreNativeBridge::ApplicationDidReceiveLocalNotification(UIApplication* app, UILocalNotification* notification)
{
    NotifyListeners(ON_DID_RECEIVE_LOCAL_NOTIFICATION, app, notification);
}

void CoreNativeBridge::DidReceiveRemoteNotification(UIApplication* app, NSDictionary* userInfo)
{
    NotifyListeners(ON_DID_RECEIVE_REMOTE_NOTIFICATION, app, userInfo);
}

void CoreNativeBridge::DidReceiveRemoteNotificationFetchCompletionHandler(UIApplication* app, NSDictionary* userInfo, void (^completionHandler)(UIBackgroundFetchResult))
{
    NotifyListeners(ON_DID_RECEIVE_REMOTE_NOTIFICATION_FETCH_COMPLETION_HANDLER, app, userInfo, nil, completionHandler);
}

void CoreNativeBridge::DidRegisterForRemoteNotificationsWithDeviceToken(UIApplication* app, NSData* deviceToken)
{
    NotifyListeners(ON_DID_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_TOKEN, app, deviceToken);
}

void CoreNativeBridge::DidFailToRegisterForRemoteNotificationsWithError(UIApplication* app, NSError* error)
{
    NotifyListeners(ON_DID_FAIL_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_ERROR, app, error);
}

void CoreNativeBridge::HandleActionWithIdentifier(UIApplication* app, NSString* identifier, NSDictionary* userInfo, id completionHandler)
{
    NotifyListeners(ON_HANDLE_ACTION_WITH_IDENTIFIER, app, identifier, userInfo, completionHandler);
}

BOOL CoreNativeBridge::OpenURL(UIApplication* app, NSURL* url, NSString* sourceApplication, id annotation)
{
    BOOL r = ApplicationOpenUrl(url);
    r |= NotifyListeners(ON_OPEN_URL, app, url, sourceApplication, annotation);
    return r;
}

BOOL CoreNativeBridge::ContinueUserActivity(UIApplication* app, NSUserActivity* userActivity, id restorationHandler)
{
    return NotifyListeners(ON_CONTINUE_USER_ACTIVITY, app, userActivity, nil, restorationHandler);
}

void CoreNativeBridge::GameControllerDidConnected()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadAddedEvent(0));
}

void CoreNativeBridge::GameControllerDidDisconnected()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadRemovedEvent(0));
}

void CoreNativeBridge::RegisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    DVASSERT(listener != nullptr);

    LockGuard<Mutex> lock(listenersMutex);
    if ([appDelegateListeners indexOfObject:listener] == NSNotFound)
    {
        [appDelegateListeners addObject:listener];
    }
}

void CoreNativeBridge::UnregisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    LockGuard<Mutex> lock(listenersMutex);
    [appDelegateListeners removeObject:listener];
}

BOOL CoreNativeBridge::NotifyListeners(eNotificationType type, NSObject* arg1, NSObject* arg2, NSObject* arg3, id arg4)
{
    BOOL ret = YES;

    NSArray* listenersCopy = nil;
    {
        // Make copy to allow listeners unregistering inside a callback
        LockGuard<Mutex> lock(listenersMutex);
        listenersCopy = [appDelegateListeners copy];
    }

    // some notification types require ret value
    // to be initialized with "NO"
    switch (type)
    {
    case ON_OPEN_URL:
        ret = NO;
        break;
    default:
        break;
    }

    for (id<DVEApplicationListener> listener in listenersCopy)
    {
        switch (type)
        {
        case ON_WILL_FINISH_LAUNCHING:
            if ([listener respondsToSelector:@selector(application:willFinishLaunchingWithOptions:)])
            {
                ret &= [listener application:static_cast<UIApplication*>(arg1) willFinishLaunchingWithOptions:static_cast<NSDictionary*>(arg2)];
            }
            break;

        case ON_DID_FINISH_LAUNCHING:
            if ([listener respondsToSelector:@selector(application:didFinishLaunchingWithOptions:)])
            {
                ret &= [listener application:static_cast<UIApplication*>(arg1) didFinishLaunchingWithOptions:static_cast<NSDictionary*>(arg2)];
            }
            break;
        case ON_DID_BECOME_ACTIVE:
            if ([listener respondsToSelector:@selector(applicationDidBecomeActive:)])
            {
                [listener applicationDidBecomeActive:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_WILL_RESIGN_ACTIVE:
            if ([listener respondsToSelector:@selector(applicationWillResignActive:)])
            {
                [listener applicationWillResignActive:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_DID_ENTER_BACKGROUND:
            if ([listener respondsToSelector:@selector(applicationDidEnterBackground:)])
            {
                [listener applicationDidEnterBackground:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_WILL_ENTER_FOREGROUND:
            if ([listener respondsToSelector:@selector(applicationWillEnterForeground:)])
            {
                [listener applicationWillEnterForeground:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_WILL_TERMINATE:
            if ([listener respondsToSelector:@selector(applicationWillTerminate:)])
            {
                [listener applicationWillTerminate:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_DID_RECEIVE_MEMORY_WARNING:
            if ([listener respondsToSelector:@selector(applicationDidReceiveMemoryWarning:)])
            {
                [listener applicationDidReceiveMemoryWarning:static_cast<UIApplication*>(arg1)];
            }
            break;
        case ON_DID_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_TOKEN:
            if ([listener respondsToSelector:@selector(application:didRegisterForRemoteNotificationsWithDeviceToken:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) didRegisterForRemoteNotificationsWithDeviceToken:static_cast<NSData*>(arg2)];
            }
            break;
        case ON_DID_FAIL_REGISTER_FOR_REMOTE_NOTIFICATION_WITH_ERROR:
            if ([listener respondsToSelector:@selector(application:didFailToRegisterForRemoteNotificationsWithError:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) didFailToRegisterForRemoteNotificationsWithError:static_cast<NSError*>(arg2)];
            }
            break;
        case ON_DID_RECEIVE_REMOTE_NOTIFICATION:
            if ([listener respondsToSelector:@selector(application:didReceiveRemoteNotification:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) didReceiveRemoteNotification:static_cast<NSDictionary*>(arg2)];
            }
            break;
        case ON_DID_RECEIVE_REMOTE_NOTIFICATION_FETCH_COMPLETION_HANDLER:
            if ([listener respondsToSelector:@selector(application:didReceiveRemoteNotification:fetchCompletionHandler:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) didReceiveRemoteNotification:static_cast<NSDictionary*>(arg2) fetchCompletionHandler:static_cast<void (^)(UIBackgroundFetchResult)>(arg4)];
            }
            break;
        case ON_DID_RECEIVE_LOCAL_NOTIFICATION:
            if ([listener respondsToSelector:@selector(application:didReceiveLocalNotification:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) didReceiveLocalNotification:static_cast<UILocalNotification*>(arg2)];
            }
            break;
        case ON_HANDLE_ACTION_WITH_IDENTIFIER:
            if ([listener respondsToSelector:@selector(application:handleActionWithIdentifier:forRemoteNotification:completionHandler:)])
            {
                [listener application:static_cast<UIApplication*>(arg1) handleActionWithIdentifier:static_cast<NSString*>(arg2) forRemoteNotification:static_cast<NSDictionary*>(arg3) completionHandler:arg4];
            }
            break;
        case ON_OPEN_URL:
            if ([listener respondsToSelector:@selector(application:openURL:sourceApplication:annotation:)])
            {
                ret |= [listener application:static_cast<UIApplication*>(arg1) openURL:static_cast<NSURL*>(arg2) sourceApplication:static_cast<NSString*>(arg3) annotation:arg4];
            }
            break;
        case ON_CONTINUE_USER_ACTIVITY:
            if ([listener respondsToSelector:@selector(application:continueUserActivity:restorationHandler:)])
            {
                ret |= [listener application:static_cast<UIApplication*>(arg1) continueUserActivity:static_cast<NSUserActivity*>(arg2) restorationHandler:arg4];
            }
            break;

        default:
            DVASSERT(false);
            break;
        }
    }

    [listenersCopy release];

    return ret;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
