#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#include "Concurrency/Mutex.h"
#include "Engine/Private/EnginePrivateFwd.h"

@class NSObject;
@class NSMutableArray;
@class NSNotification;
@class NSString;
@class NSUserNotification;
@class NSUserNotificationCenter;
@class FrameTimer;
@class AppDelegate;

@protocol DVEApplicationListener;

namespace DAVA
{
namespace Private
{
// Bridge between C++ and Objective-C for macos PlatformCore class
// Responsibilities:
//  - holds neccesary Objective-C objects
//  - processes notifications from AppDelegate which implements
//    interface NSApplicationDelegate
//
// CoreNativeBridge is friend of macos PlatformCore
struct CoreNativeBridge final
{
    CoreNativeBridge(PlatformCore* core);
    ~CoreNativeBridge();

    void Run();
    void Quit();
    void OnFrameTimer();

    // Callbacks from AppDelegate
    void ApplicationWillFinishLaunching(NSNotification* notification);
    void ApplicationDidFinishLaunching(NSNotification* notification);
    bool ApplicationOpenFile(NSString* filename);
    void ApplicationDidChangeScreenParameters();
    void ApplicationDidBecomeActive(NSNotification* notification);
    void ApplicationDidResignActive(NSNotification* notification);
    void ApplicationDidHide();
    void ApplicationDidUnhide();
    bool ApplicationShouldTerminate();
    bool ApplicationShouldTerminateAfterLastWindowClosed();
    void ApplicationWillTerminate(NSNotification* notification);
    void ApplicationDidActivateNotification(NSUserNotificationCenter* notificationCenter, NSUserNotification* notification);

    void RegisterDVEApplicationListener(id<DVEApplicationListener> listener);
    void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener);

    enum eNotificationType
    {
        ON_WILL_FINISH_LAUNCHING,
        ON_DID_FINISH_LAUNCHING,
        ON_DID_BECOME_ACTIVE,
        ON_DID_RESIGN_ACTIVE,
        ON_WILL_TERMINATE,
        ON_DID_RECEIVE_REMOTE_NOTIFICATION,
        ON_DID_REGISTER_REMOTE_NOTIFICATION,
        ON_DID_FAIL_TO_REGISTER_REMOTE_NOTIFICATION,
        ON_DID_ACTIVATE_NOTIFICATION,
    };
    void NotifyListeners(eNotificationType type, NSObject* arg1, NSObject* arg2, NSObject* arg3);

    PlatformCore* core = nullptr;

    AppDelegate* appDelegate = nullptr;
    FrameTimer* frameTimer = nullptr;

    Mutex listenersMutex;
    NSMutableArray* appDelegateListeners;

    bool quitSent = false;
    bool closeRequestByApp = false;
    int32 curFps = 0;

    bool didFinishLaunching = false;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
