#if defined(__DAVAENGINE_IOS__)

#include <UIKit/UIKit.h>

@interface NativeDelegateIos : NSObject
- (void)supportedInterfaceOrientations:UIInterfaceOrientationMask;
@end

@implementation NativeDelegateIos

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

@end

#endif // __DAVAENGINE_IPHONE__
