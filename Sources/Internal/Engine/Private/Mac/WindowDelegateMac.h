#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSWindow.h>

#include "Engine/Private/EnginePrivateFwd.h"

// Implementation of NSWindowDelegate
// Forwards all necessary methods to WindowNativeBridge
@interface WindowDelegate : NSObject<NSWindowDelegate>
{
    DAVA::Private::WindowNativeBridge* bridge;
}

- (id)initWithBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge;

@end

#endif // __DAVAENGINE_MACOS__
