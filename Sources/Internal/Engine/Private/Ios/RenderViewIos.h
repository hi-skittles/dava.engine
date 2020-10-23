#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/EnginePrivateFwd.h"

#import <UIKit/UIView.h>

@interface RenderView : UIView
{
    DAVA::Private::WindowNativeBridge* bridge;
}

- (id)initWithFrame:(CGRect)frame andBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge;

- (void)setSurfaceScale:(DAVA::float32)surfaceScale;
- (DAVA::float32)surfaceScale;
- (CGSize)surfaceSize;

@end

///////////////////////////////////////////////////////////////////////

@interface RenderViewMetal : RenderView
+ (Class)layerClass;
@end

@interface RenderViewGL : RenderView
+ (Class)layerClass;
@end

#endif // __DAVAENGINE_IPHONE__
