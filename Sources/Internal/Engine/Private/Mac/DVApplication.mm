#import "Engine/Private/Mac/DVApplication.h"

#if defined(__DAVAENGINE_MACOS__)

@implementation DVApplication

- (void)sendEvent:(NSEvent*)event
{
    // NSKeyUp is not sent by default sendEvent: implementation if cmd is pressed.
    // Since we want our keyboard to provide real state of a physical keyboard,
    // this method is overriden and sends keyup event directly to a window in this case.
    // Thus avoiding stuck button state.
    // In other cases events are sent same way as in NSApplication

    if ([event type] == NSKeyUp && ([event modifierFlags] & NSCommandKeyMask))
        [[self keyWindow] sendEvent:event];
    else
        [super sendEvent:event];
}

@end

#endif
