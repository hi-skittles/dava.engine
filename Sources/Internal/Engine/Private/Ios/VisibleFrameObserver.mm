#include "Engine/Private/Ios/VisibleFrameObserver.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Ios/RenderViewIos.h"
#include "Engine/Private/Ios/WindowNativeBridgeIos.h"

#import <UIKit/UIKit.h>

@implementation VisibleFrameObserver

- (id)initWithBridge:(DAVA::Private::WindowNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nil)
    {
        bridge = nativeBridge;

        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center addObserver:self
                   selector:@selector(keyboardWillShow:)
                       name:UIKeyboardWillShowNotification
                     object:nil];
    }
    return self;
}

- (void)dealloc
{
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self
                      name:UIKeyboardDidShowNotification
                    object:nil];
    [super dealloc];
}

- (void)fireWithKeyboardFrame:(const CGRect*)keyboardFrame
{
    if (bridge->uiwindow == nil || bridge->renderView == nil)
    {
        // Skip notification if window not initialized yet
        return;
    }

    // Convert window frame to render view coordinates
    CGRect visibleFrame = [bridge->uiwindow convertRect:bridge->uiwindow.frame toView:bridge->renderView];

    if (keyboardFrame != nil)
    {
        // Convert keyboard frame to render view coordinates
        CGRect convertedKeyboardFrame = [bridge->uiwindow convertRect:*keyboardFrame toView:bridge->renderView];
        // Calculate visible frame
        visibleFrame.size.height = convertedKeyboardFrame.origin.y - visibleFrame.origin.y;
    }

    bridge->mainDispatcher->PostEvent(DAVA::Private::MainDispatcherEvent::CreateWindowVisibleFrameChangedEvent(bridge->window, visibleFrame.origin.x, visibleFrame.origin.y, visibleFrame.size.width, visibleFrame.size.height));
}

- (void)keyboardWillShow:(NSNotification*)notification
{
    CGRect keyboardFrame = [[notification.userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    [self fireWithKeyboardFrame:&keyboardFrame];

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(keyboardWillHide:)
                   name:UIKeyboardWillHideNotification
                 object:nil];
}

- (void)keyboardWillHide:(NSNotification*)notification
{
    CGRect keyboardFrame = [[notification.userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    [self fireWithKeyboardFrame:nil];

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self
                      name:UIKeyboardWillHideNotification
                    object:nil];
}

@end

#endif // __DAVAENGINE_IPHONE__
