#include "Engine/Private/Ios/NativeViewPoolIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Debug/DVAssert.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@implementation NativeViewPool

- (id)init
{
    self = [super init];
    if (self != nil)
    {
        pool.reserve(10);
    }
    return self;
}

- (void)dealloc
{
    for (std::pair<UIView*, bool> pair : pool)
    {
        DVASSERT(pair.second == false, "Someone did not return view back to pool");
        [pair.first release];
    }
    [super dealloc];
}

- (UIView*)queryView:(NSString*)viewClassName
{
    // TODO: check if viewClassName is valid class and is subclass of UIView
    for (std::pair<UIView*, bool>& pair : pool)
    {
        if (!pair.second)
        {
            NSString* x = [[pair.first class] description];
            if ([x isEqualToString:viewClassName])
            {
                pair.second = true;
                return pair.first;
            }
        }
    }

    Class viewClass = NSClassFromString(viewClassName);
    UIView* newView = [[viewClass alloc] init];
    pool.emplace_back(newView, true);
    return newView;
}

- (void)returnView:(UIView*)view
{
    for (std::pair<UIView*, bool>& pair : pool)
    {
        if (pair.first == view)
        {
            pair.second = false;
            return;
        }
    }
    DVASSERT(false, "You've tried to return a view that has never been in the pool");
}

@end

#endif // __DAVAENGINE_IPHONE__
