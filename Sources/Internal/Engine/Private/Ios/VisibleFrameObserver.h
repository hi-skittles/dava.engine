#pragma once

#if defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSObject.h>

@class NSNotification;

namespace DAVA
{
namespace Private
{
class WindowNativeBridge;
}
}

@interface VisibleFrameObserver : NSObject
{
    DAVA::Private::WindowNativeBridge* bridge;
}

- (id)initWithBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge;
- (void)dealloc;

@end

#endif // __DAVAENGINE_IPHONE__
