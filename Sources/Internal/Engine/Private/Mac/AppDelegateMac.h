#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSApplication.h>

#include "Engine/Private/EnginePrivateFwd.h"

// Implementation of NSApplicationDelegate
// Forwards all necessary methods to CoreNativeBridge
@interface AppDelegate : NSObject<NSApplicationDelegate, NSUserNotificationCenterDelegate>
{
    DAVA::Private::CoreNativeBridge* bridge;
}

- (id)initWithBridge:(DAVA::Private::CoreNativeBridge*)nativeBridge;

@end

#endif // __DAVAENGINE_MACOS__
