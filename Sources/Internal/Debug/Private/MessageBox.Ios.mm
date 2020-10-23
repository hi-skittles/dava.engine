#include "Base/Platform.h"
#include "Base/BaseTypes.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/AutoResetEvent.h"
#include "Engine/Private/EngineBackend.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"

#import <Foundation/NSThread.h>
#import <UIKit/UIAlertView.h>
#import <UIKit/UIApplication.h>

@interface AlertDialog : NSObject<UIAlertViewDelegate>
{
    BOOL dismissedOnResignActive;
    UIAlertView* alert;
    NSMutableArray<NSString*>* buttonNames;
}
- (int)showModal;
- (void)addButtonWithTitle:(NSString*)buttonTitle;
- (void)alertView:(UIAlertView*)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex;

@property(nonatomic, retain) NSString* title;
@property(nonatomic, retain) NSString* message;
@property(nonatomic, readonly) int clickedIndex;
@property(nonatomic, assign) BOOL dismissOnResignActive;

@end

@implementation AlertDialog

- (void)dealloc
{
    [_title release];
    [_message release];
    if (buttonNames != nil)
        [buttonNames release];

    [super dealloc];
}

- (int)showModal
{
    DAVA::Private::EngineBackend::showingModalMessageBox = true;

    @autoreleasepool
    {
        alert = [[[UIAlertView alloc] initWithTitle:_title
                                            message:_message
                                           delegate:self
                                  cancelButtonTitle:nil
                                  otherButtonTitles:nil, nil] autorelease];
        for (NSString* s : buttonNames)
        {
            [alert addButtonWithTitle:s];
        }

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(applicationWillResignActive:)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];

        _clickedIndex = -1;
        dismissedOnResignActive = NO;

        [alert show];
        @autoreleasepool
        {
            while (_clickedIndex < 0)
            {
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
            }
        }

        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [alert setDelegate:nil];
        alert = nil;
        if (dismissedOnResignActive)
        {
            _clickedIndex = -1;
        }
    }

    DAVA::Private::EngineBackend::showingModalMessageBox = false;
    return _clickedIndex;
}

- (void)applicationWillResignActive:(NSNotification*)notification
{
    if (_dismissOnResignActive)
    {
        dismissedOnResignActive = YES;
        [alert dismissWithClickedButtonIndex:0 animated:NO];
    }
}

- (void)addButtonWithTitle:(NSString*)buttonTitle
{
    if (buttonNames == nil)
    {
        buttonNames = [[NSMutableArray alloc] init];
    }
    [buttonNames addObject:buttonTitle];
}

- (void)alertView:(UIAlertView*)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    _clickedIndex = static_cast<int>(buttonIndex);
}

@end

namespace DAVA
{
namespace Debug
{
namespace MessageBoxDetail
{
Semaphore semaphore(1);
AutoResetEvent autoEvent;
} // namespace MessageBoxDetail

int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
{
    DVASSERT(0 < buttons.size() && buttons.size() <= 3);
    DVASSERT(0 <= defaultButton && defaultButton < static_cast<int>(buttons.size()));

    int result = -1;
    auto showMessageBox = [title, message, buttons, defaultButton, &result]() {
        @autoreleasepool
        {
            AlertDialog* alertDialog = [[[AlertDialog alloc] init] autorelease];
            [alertDialog setTitle:@(title.c_str())];
            [alertDialog setMessage:@(message.c_str())];
            [alertDialog setDismissOnResignActive:[NSThread isMainThread]];

            for (const String& s : buttons)
            {
                [alertDialog addButtonWithTitle:@(s.c_str())];
            }

            [alertDialog performSelectorOnMainThread:@selector(showModal)
                                          withObject:nil
                                       waitUntilDone:YES];
            result = [alertDialog clickedIndex];
            MessageBoxDetail::autoEvent.Signal();
        }
    };

    // Application hangs when MessageBox is shown before invoking UIApplicationMain (called in Engine::Run).
    // Experimentally I determined that currentMode of main run loop is nil before UIApplicationMain is called.
    if ([[NSRunLoop mainRunLoop] currentMode] != nil)
    {
        if ([NSThread isMainThread])
        {
            showMessageBox();
        }
        else
        {
            // Do not use Window::RunOnUIThread as message box becomes unresponsive to user input.
            // I do not know why so.
            MessageBoxDetail::semaphore.Wait();
            showMessageBox();
            MessageBoxDetail::autoEvent.Wait();
            MessageBoxDetail::semaphore.Post(1);
        }
    }
    return result;
}

} // namespace Debug
} // namespace DAVA
