#include "Engine/Private/Ios/RenderViewControllerIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/Ios/WindowNativeBridgeIos.h"
#import <UIKit/UIScreenEdgePanGestureRecognizer.h>

@implementation RenderViewController

- (id)initWithBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nil)
    {
        bridge = nativeBridge;
        homeIndicatorAutoHidden = bridge->engineOptions->GetBool("ios_home_indicator_auto_hidden", false);
    }
    return self;
}

- (void)loadView
{
    bridge->LoadView();
}

- (void)viewDidLoad
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged:) name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
    return UIRectEdgeBottom;
}

- (void)orientationChanged:(NSNotification*)notification
{
    bridge->OrientationChanged();
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    bridge->ViewWillTransitionToSize(size.width, size.height);
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
    return homeIndicatorAutoHidden;
}

@end

#endif // __DAVAENGINE_IPHONE__
