#include "Engine/Private/Mac/CoreNativeBridgeMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <Foundation/Foundation.h>

#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Mac/PlatformCoreMac.h"
#include "Engine/Private/Mac/WindowImplMac.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

#import "Engine/Private/Mac/DVApplication.h"
#import "Engine/PlatformApiMac.h"
#import "Engine/Private/Mac/AppDelegateMac.h"

#include "Concurrency/LockGuard.h"
#include "Logger/Logger.h"
#include "Time/SystemTimer.h"
#include "Utils/NSStringUtils.h"

// Wrapper over NSTimer to connect Objective-C NSTimer object to
// C++ class CoreNativeBridge
@interface FrameTimer : NSObject
{
    DAVA::Private::CoreNativeBridge* bridge;
    double curInterval;

    NSTimer* timer;
}

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge;
- (void)set:(double)interval;
- (void)cancel;
- (void)timerFired:(NSTimer*)timer;

@end

@implementation FrameTimer

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nil)
    {
        bridge = nativeBridge;
        curInterval = -1.0;
    }
    return self;
}

- (void)set:(double)interval
{
    const double delta = 0.000001;
    if (std::abs(interval - curInterval) > delta)
    {
        [self cancel];
        timer = [NSTimer scheduledTimerWithTimeInterval:interval
                                                 target:self
                                               selector:@selector(timerFired:)
                                               userInfo:nil
                                                repeats:YES];
        curInterval = interval;
    }
}

- (void)cancel
{
    [timer invalidate];
    timer = nullptr;
}

- (void)timerFired:(NSTimer*)timer
{
    bridge->OnFrameTimer();
}

@end

//////////////////////////////////////////////////////////////////

// Defined in EntryApple.mm
extern NSAutoreleasePool* preMainLoopReleasePool;

namespace DAVA
{
namespace Private
{
CoreNativeBridge::CoreNativeBridge(PlatformCore* core)
    : core(core)
{
    appDelegateListeners = [[NSMutableArray alloc] init];

    // Force init NSApplication
    [DVApplication sharedApplication];

    // On macOS, AppKit will catch exceptions thrown on the main thread, preventing the application
    // from crashing, but also preventing Crashlytics from reporting them.
    // See more here: https://docs.fabric.io/apple/crashlytics/os-x.html
    // Turn of feature to crash on uncaught objective-c exception.
    [[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"NSApplicationCrashOnExceptions" : @YES }];
}

CoreNativeBridge::~CoreNativeBridge()
{
    [appDelegateListeners release];
}

void CoreNativeBridge::Run()
{
    appDelegate = [[AppDelegate alloc] initWithBridge:this];
    [[DVApplication sharedApplication] setDelegate:(id<NSApplicationDelegate>)appDelegate];

    [preMainLoopReleasePool drain];

    // NSApplicationMain never returns
    // NSApplicationMain itself ignores the argc and argv arguments. Instead, Cocoa gets its arguments indirectly via _NSGetArgv, _NSGetArgc, and _NSGetEnviron.
    // See https://developer.apple.com/library/mac/documentation/Cocoa/Reference/ApplicationKit/Miscellaneous/AppKit_Functions/#//apple_ref/c/func/NSApplicationMain
    ::NSApplicationMain(0, nullptr);
}

void CoreNativeBridge::Quit()
{
    closeRequestByApp = true;
    if (!quitSent)
    {
        quitSent = true;
        [[DVApplication sharedApplication] terminate:nil];
    }
}

void CoreNativeBridge::OnFrameTimer()
{
    int32 fps = core->OnFrame();
    if (fps <= 0)
    {
        // To prevent division by zero
        fps = std::numeric_limits<int32>::max();
    }

    if (curFps != fps)
    {
        double interval = 1.0 / fps;
        [frameTimer set:interval];
    }
}

void CoreNativeBridge::ApplicationWillFinishLaunching(NSNotification* notification)
{
    NotifyListeners(ON_WILL_FINISH_LAUNCHING, notification, nullptr, nullptr);
}

void CoreNativeBridge::ApplicationDidFinishLaunching(NSNotification* notification)
{
    core->engineBackend->OnGameLoopStarted();

    WindowImpl* primaryWindowImpl = EngineBackend::GetWindowImpl(core->engineBackend->GetPrimaryWindow());
    primaryWindowImpl->Create(640.0f, 480.0f);

    frameTimer = [[FrameTimer alloc] init:this];
    [frameTimer set:1.0 / 60.0];

    NotifyListeners(ON_DID_FINISH_LAUNCHING, notification, nullptr, nullptr);
    didFinishLaunching = true;
}

bool CoreNativeBridge::ApplicationOpenFile(NSString* filename)
{
    core->engineBackend->AddActivationFilename(StringFromNSString(filename));
    if (didFinishLaunching)
    {
        core->engineBackend->OnFileActivated();

        // System does not brings to front miniaturized window so do it explicitly
        Window* w = core->engineBackend->GetPrimaryWindow();
        if (w != nullptr)
        {
            w->ActivateAsync();
        }
    }
    return true;
}

void CoreNativeBridge::ApplicationDidChangeScreenParameters()
{
    Logger::Debug("****** CoreNativeBridge::ApplicationDidChangeScreenParameters");
}

void CoreNativeBridge::ApplicationDidBecomeActive(NSNotification* notification)
{
    NotifyListeners(ON_DID_BECOME_ACTIVE, notification, nullptr, nullptr);
}

void CoreNativeBridge::ApplicationDidResignActive(NSNotification* notification)
{
    NotifyListeners(ON_DID_RESIGN_ACTIVE, notification, nullptr, nullptr);
}

void CoreNativeBridge::ApplicationDidHide()
{
    core->didHideUnhide.Emit(true);
}

void CoreNativeBridge::ApplicationDidUnhide()
{
    core->didHideUnhide.Emit(false);
}

bool CoreNativeBridge::ApplicationShouldTerminate()
{
    if (!closeRequestByApp)
    {
        core->engineBackend->PostUserCloseRequest();
        return false;
    }

    if (!quitSent)
    {
        core->engineBackend->PostAppTerminate(false);
        return false;
    }
    return true;
}

bool CoreNativeBridge::ApplicationShouldTerminateAfterLastWindowClosed()
{
    return false;
}

void CoreNativeBridge::ApplicationWillTerminate(NSNotification* notification)
{
    NotifyListeners(ON_WILL_TERMINATE, notification, nullptr, nullptr);

    [frameTimer cancel];

    core->engineBackend->OnGameLoopStopped();

    [[DVApplication sharedApplication] setDelegate:nil];
    [appDelegate release];
    [frameTimer release];

    int exitCode = core->engineBackend->GetExitCode();
    core->engineBackend->OnEngineCleanup();
    std::exit(exitCode);
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

void CoreNativeBridge::NotifyListeners(eNotificationType type, NSObject* arg1, NSObject* arg2, NSObject* arg3)
{
    NSArray* listenersCopy = nil;
    {
        // Make copy to allow listeners unregistering inside a callback
        LockGuard<Mutex> lock(listenersMutex);
        listenersCopy = [appDelegateListeners copy];
    }

    for (id<DVEApplicationListener> listener in listenersCopy)
    {
        switch (type)
        {
        case ON_WILL_FINISH_LAUNCHING:
            if ([listener respondsToSelector:@selector(applicationWillFinishLaunching:)])
            {
                [listener applicationWillFinishLaunching:static_cast<NSNotification*>(arg1)];
            }
            break;
        case ON_DID_FINISH_LAUNCHING:
            if ([listener respondsToSelector:@selector(applicationDidFinishLaunching:)])
            {
                [listener applicationDidFinishLaunching:static_cast<NSNotification*>(arg1)];
            }
            break;
        case ON_DID_BECOME_ACTIVE:
            if ([listener respondsToSelector:@selector(applicationDidBecomeActive:)])
            {
                [listener applicationDidBecomeActive:static_cast<NSNotification*>(arg1)];
            }
            break;
        case ON_DID_RESIGN_ACTIVE:
            if ([listener respondsToSelector:@selector(applicationDidResignActive:)])
            {
                [listener applicationDidResignActive:static_cast<NSNotification*>(arg1)];
            }
            break;
        case ON_WILL_TERMINATE:
            if ([listener respondsToSelector:@selector(applicationWillTerminate:)])
            {
                [listener applicationWillTerminate:static_cast<NSNotification*>(arg1)];
            }
            break;
        case ON_DID_RECEIVE_REMOTE_NOTIFICATION:
            if ([listener respondsToSelector:@selector(application:didReceiveRemoteNotification:)])
            {
                [listener application:static_cast<NSApplication*>(arg1) didReceiveRemoteNotification:static_cast<NSDictionary*>(arg2)];
            }
            break;
        case ON_DID_REGISTER_REMOTE_NOTIFICATION:
            if ([listener respondsToSelector:@selector(application:didRegisterForRemoteNotificationsWithDeviceToken:)])
            {
                [listener application:static_cast<NSApplication*>(arg1) didRegisterForRemoteNotificationsWithDeviceToken:static_cast<NSData*>(arg2)];
            }
            break;
        case ON_DID_FAIL_TO_REGISTER_REMOTE_NOTIFICATION:
            if ([listener respondsToSelector:@selector(application:didFailToRegisterForRemoteNotificationsWithError:)])
            {
                [listener application:static_cast<NSApplication*>(arg1) didFailToRegisterForRemoteNotificationsWithError:static_cast<NSError*>(arg2)];
            }
            break;
        case ON_DID_ACTIVATE_NOTIFICATION:
            if ([listener respondsToSelector:@selector(userNotificationCenter:didActivateNotification:)])
            {
                [listener userNotificationCenter:static_cast<NSUserNotificationCenter*>(arg1) didActivateNotification:static_cast<NSUserNotification*>(arg2)];
            }
            break;
        default:
            break;
        }
    }

    [listenersCopy release];
}

void CoreNativeBridge::ApplicationDidActivateNotification(NSUserNotificationCenter* notificationCenter, NSUserNotification* notification)
{
    NotifyListeners(ON_DID_ACTIVATE_NOTIFICATION, notificationCenter, notification, nullptr);
}
} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
