#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <UIKit/UIViewController.h>

#include "Engine/Private/EnginePrivateFwd.h"

@interface RenderViewController : UIViewController
{
    DAVA::Private::WindowNativeBridge* bridge;
    bool homeIndicatorAutoHidden;
}

- (id)initWithBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge;

@end

#endif // __DAVAENGINE_IPHONE__
