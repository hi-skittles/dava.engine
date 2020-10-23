#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSObject.h>

@class UIView;

@interface NativeViewPool : NSObject
{
    // Each element contains UIView-derived class and flag
    // which indicates whether UIView is free to use (false value) or
    // is already in use (true value)
    DAVA::Vector<std::pair<UIView*, bool>> pool;
}

// Create or get free UIView from pool by its Objective-C class name, e.g. UIWebView, UITextField
- (UIView*)queryView:(NSString*)viewClassName;
// Mark view as free to use for subsequent queryView calls
- (void)returnView:(UIView*)view;

@end

#endif // __DAVAENGINE_IPHONE__
