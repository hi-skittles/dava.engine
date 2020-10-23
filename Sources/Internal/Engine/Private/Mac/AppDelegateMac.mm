#include "Engine/Private/Mac/AppDelegateMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#include "Engine/Private/Mac/CoreNativeBridgeMac.h"

@implementation AppDelegate

- (id)initWithBridge:(DAVA::Private::CoreNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nullptr)
    {
        bridge = nativeBridge;
    }
    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
    bridge->ApplicationWillFinishLaunching(notification);
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    bridge->ApplicationDidFinishLaunching(notification);
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
    bool r = bridge->ApplicationOpenFile(filename);
    return r ? YES : NO;
}

- (void)applicationDidChangeScreenParameters:(NSNotification*)notification
{
    bridge->ApplicationDidChangeScreenParameters();
}

- (void)applicationDidBecomeActive:(NSNotification*)notification
{
    bridge->ApplicationDidBecomeActive(notification);
}

- (void)applicationDidResignActive:(NSNotification*)notification
{
    bridge->ApplicationDidResignActive(notification);
}

- (void)applicationDidHide:(NSNotification*)notification
{
    bridge->ApplicationDidHide();
}

- (void)applicationDidUnhide:(NSNotification*)notification
{
    bridge->ApplicationDidUnhide();
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    bool r = bridge->ApplicationShouldTerminate();
    return r ? NSTerminateNow : NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    bool r = bridge->ApplicationShouldTerminateAfterLastWindowClosed();
    return r ? YES : NO;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    bridge->ApplicationWillTerminate(notification);
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center didActivateNotification:(NSUserNotification*)notification
{
    bridge->ApplicationDidActivateNotification(center, notification);
}

@end

#endif // __DAVAENGINE_MACOS__
