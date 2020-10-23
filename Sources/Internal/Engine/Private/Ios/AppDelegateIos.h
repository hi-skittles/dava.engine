#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <UIKit/UIApplication.h>
#include "Engine/Private/EnginePrivateFwd.h"
#include "Concurrency/Mutex.h"

@protocol DVEApplicationListener;

// Implementation of UIApplicationDelegate
// Forwards all necessary methods to CoreNativeBridge
@interface AppDelegate : NSObject<UIApplicationDelegate>
{
    DAVA::Private::CoreNativeBridge* bridge;
}

@end

#endif // __DAVAENGINE_IPHONE__
