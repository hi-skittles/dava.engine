#include "Engine/Private/Mac/WindowNativeBridgeMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSCursor.h>
#import <AppKit/NSWindow.h>
#import <AppKit/NSScreen.h>
#import <Carbon/Carbon.h>

#import "Engine/Private/Mac/DVApplication.h"

#include "Engine/Window.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Mac/WindowImplMac.h"
#include "Engine/Private/Mac/RenderViewMac.h"
#include "Engine/Private/Mac/WindowDelegateMac.h"

#include "Time/SystemTimer.h"
#include "Logger/Logger.h"

namespace DAVA
{
namespace Private
{
WindowNativeBridge::WindowNativeBridge(WindowImpl* windowImpl)
    : windowImpl(windowImpl)
    , window(windowImpl->window)
    , mainDispatcher(windowImpl->mainDispatcher)
{
}

WindowNativeBridge::~WindowNativeBridge() = default;

bool WindowNativeBridge::CreateWindow(float32 x, float32 y, float32 width, float32 height)
{
    windowImpl->uiDispatcher.LinkToCurrentThread();

    // clang-format off
    NSUInteger style = NSTitledWindowMask |
                       NSMiniaturizableWindowMask |
                       NSClosableWindowMask |
                       NSResizableWindowMask;
    // clang-format on

    // create window
    NSRect viewRect = NSMakeRect(x, y, width, height);
    nswindow = [[NSWindow alloc] initWithContentRect:viewRect
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    // set some window params
    [nswindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [nswindow setContentMinSize:NSMakeSize(128, 128)];

    // add window delegate
    windowDelegate = [[WindowDelegate alloc] initWithBridge:this];
    [nswindow setDelegate:windowDelegate];

    // create render view and add it into window
    renderView = [[RenderView alloc] initWithBridge:this];

    // now set renderView as contentView
    [nswindow setContentView:renderView];

    {
        float32 dpi = GetDpi();
        CGSize surfaceSize = [renderView convertSizeToBacking:viewRect.size];
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCreatedEvent(window, viewRect.size.width, viewRect.size.height, surfaceSize.width, surfaceSize.height, dpi, eFullscreen::Off));
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
    }

    [nswindow makeKeyAndOrderFront:nil];
    return true;
}

void WindowNativeBridge::ResizeWindow(float32 width, float32 height)
{
    NSScreen* screen = [nswindow screen];

    NSRect windowRect = [nswindow frame];
    NSRect screenRect = [screen visibleFrame];

    float32 dx = (windowRect.size.width - width) / 2.0;
    float32 dy = (windowRect.size.height - height) / 2.0;

    NSPoint pos = NSMakePoint(windowRect.origin.x + dx, windowRect.origin.y + dy);

    if (pos.x < screenRect.origin.x)
    {
        pos.x = screenRect.origin.x;
    }

    if ((pos.y + height) > (screenRect.origin.y + screenRect.size.height))
    {
        pos.y = (screenRect.origin.y + screenRect.size.height) - height;
    }

    NSSize sz = NSMakeSize(width, height);

    [nswindow setFrameOrigin:pos];
    [nswindow setContentSize:sz];
}

void WindowNativeBridge::ActivateWindow()
{
    // https://snippets.aktagon.com/snippets/357-showing-and-hiding-an-nswindow-programatically
    if ([nswindow isMiniaturized])
    {
        [nswindow deminiaturize:nil];
    }
    [nswindow makeKeyAndOrderFront:nil];
    [[DVApplication sharedApplication] activateIgnoringOtherApps:YES];
}

void WindowNativeBridge::CloseWindow()
{
    [nswindow close];
}

void WindowNativeBridge::SetTitle(const char8* title)
{
    [nswindow setTitle:[NSString stringWithUTF8String:title]];
}

void WindowNativeBridge::SetMinimumSize(float32 width, float32 height)
{
    NSSize sz = NSMakeSize(width, height);
    [nswindow setContentMinSize:sz];
}

void WindowNativeBridge::SetFullscreen(eFullscreen newMode)
{
    bool isFullscreenRequested = newMode == eFullscreen::On;

    if (isFullscreen != isFullscreenRequested)
    {
        [nswindow toggleFullScreen:nil];

        if (isFullscreen)
        {
            // If we're entering fullscreen we want our app to also become focused
            // To handle cases when app is being opened with fullscreen mode,
            // but another app gets focus before our app's window is created,
            // thus ignoring any input afterwards
            [[DVApplication sharedApplication] activateIgnoringOtherApps:YES];
        }
    }
}

void WindowNativeBridge::TriggerPlatformEvents()
{
    dispatch_async(dispatch_get_main_queue(), [this]() {
        windowImpl->ProcessPlatformEvents();
    });
}

void WindowNativeBridge::ApplicationDidHideUnhide(bool hidden)
{
    isAppHidden = hidden;
}

void WindowNativeBridge::WindowDidMiniaturize()
{
    isVisible = false;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, isVisible));
}

void WindowNativeBridge::WindowDidDeminiaturize()
{
}

void WindowNativeBridge::WindowDidBecomeKey()
{
    if (isAppHidden || !isVisible)
    {
        isVisible = true;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, isVisible));
    }
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, true));
}

void WindowNativeBridge::WindowDidResignKey()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, false));
    if (captureMode == eCursorCapture::PINNING)
    {
        SetCursorCapture(eCursorCapture::OFF);
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCaptureLostEvent(window));
    }
    SetCursorVisibility(true);
    if (isAppHidden)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, false));
    }
}

void WindowNativeBridge::HandleSizeChanging(WindowNativeBridge::SizeChangingReason reason)
{
    float32 dpi = GetDpi();
    eFullscreen fullscreen = isFullscreen ? eFullscreen::On : eFullscreen::Off;

    CGSize windowSize = [renderView frame].size;
    CGFloat backingScale = [nswindow backingScaleFactor];
    CGFloat surfaceWidth = windowSize.width * surfaceScale * backingScale;
    CGFloat surfaceHeight = windowSize.height * surfaceScale * backingScale;

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSizeChangedEvent(window, windowSize.width, windowSize.height, surfaceWidth, surfaceHeight, surfaceScale, dpi, fullscreen));

    if (reason == WindowNativeBridge::SizeChangingReason::WindowDpiChanged)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowDpiChangedEvent(window, dpi));
    }

    // When window's bounds change, pinned cursor might appear outside of the window and become lost.
    // Due to this, I update the pinned cursor's position each time the window is resized.
    // Nevertheless, the window's bounds still can be moved and lose the pinned cursor.
    if (eCursorCapture::PINNING == captureMode)
    {
        SetSystemCursorCapture(true);
    }
}

void WindowNativeBridge::WindowDidResize()
{
    HandleSizeChanging(WindowNativeBridge::SizeChangingReason::WindowSurfaceChanged);
}

void WindowNativeBridge::WindowDidChangeScreen()
{
    HandleSizeChanging(WindowNativeBridge::SizeChangingReason::WindowDpiChanged);
}

bool WindowNativeBridge::WindowShouldClose()
{
    if (!windowImpl->closeRequestByApp)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateUserCloseRequestEvent(window));
        return false;
    }
    return true;
}

void WindowNativeBridge::WindowWillClose()
{
    windowImpl->WindowWillClose();
    mainDispatcher->SendEvent(MainDispatcherEvent::CreateWindowDestroyedEvent(window));

    [nswindow setContentView:nil];
    [nswindow setDelegate:nil];

    [renderView release];
    [windowDelegate release];
}

void WindowNativeBridge::WindowWillEnterFullScreen()
{
    isFullscreen = true;
}

void WindowNativeBridge::WindowWillExitFullScreen()
{
    isFullscreen = false;
}

void WindowNativeBridge::MouseClick(NSEvent* theEvent)
{
    eMouseButtons button = GetMouseButton(theEvent);
    if (button != eMouseButtons::NONE)
    {
        MainDispatcherEvent::eType type = MainDispatcherEvent::DUMMY;
        switch ([theEvent type])
        {
        case NSLeftMouseDown:
        case NSRightMouseDown:
        case NSOtherMouseDown:
            type = MainDispatcherEvent::MOUSE_BUTTON_DOWN;
            break;
        case NSLeftMouseUp:
        case NSRightMouseUp:
        case NSOtherMouseUp:
            type = MainDispatcherEvent::MOUSE_BUTTON_UP;
            break;
        default:
            return;
        }

        NSSize sz = [renderView frame].size;
        NSPoint pt = [theEvent locationInWindow];

        float32 x = pt.x;
        float32 y = sz.height - pt.y;
        eModifierKeys modifierKeys = GetModifierKeys(theEvent);
        bool isRelative = (captureMode == eCursorCapture::PINNING);
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, type, button, x, y, 1, modifierKeys, isRelative));
    }
}

void WindowNativeBridge::MouseMove(NSEvent* theEvent)
{
    if (mouseMoveSkipCount)
    {
        mouseMoveSkipCount--;
        return;
    }
    NSSize sz = [renderView frame].size;
    NSPoint pt = theEvent.locationInWindow;
    bool isRelative = (captureMode == eCursorCapture::PINNING);
    float32 x = pt.x;
    float32 y = sz.height - pt.y;

    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    if (isRelative)
    {
        x = [theEvent deltaX];
        y = [theEvent deltaY];
    }
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, x, y, modifierKeys, isRelative));
}

void WindowNativeBridge::MouseWheel(NSEvent* theEvent)
{
    static const float32 scrollK = 10.0f;

    NSSize sz = [renderView frame].size;
    NSPoint pt = theEvent.locationInWindow;

    float32 x = pt.x;
    float32 y = sz.height - pt.y;
    float32 wheelDeltaX = [theEvent scrollingDeltaX];
    float32 wheelDeltaY = [theEvent scrollingDeltaY];

    // detect the wheel event device
    // http://stackoverflow.com/questions/13807616/mac-cocoa-how-to-differentiate-if-a-nsscrollwheel-event-is-from-a-mouse-or-trac
    if (NSEventPhaseNone != [theEvent momentumPhase] || NSEventPhaseNone != [theEvent phase])
    {
        // TODO: add support for mouse/touch in DispatcherEvent
        //event.device = DAVA::UIEvent::Device::TOUCH_PAD;
    }
    else
    {
        //event.device = DAVA::UIEvent::Device::MOUSE;
        // Invert scroll directions back because MacOS do it by self when Shift pressed
        if (([theEvent modifierFlags] & NSShiftKeyMask) != 0)
        {
            std::swap(wheelDeltaX, wheelDeltaY);
        }
    }

    if ([theEvent hasPreciseScrollingDeltas] == YES)
    {
        // Touchpad or other precise device send integer values (-3, -1, 0, 1, 40, etc)
        wheelDeltaX /= scrollK;
        wheelDeltaY /= scrollK;
    }
    else
    {
        // Mouse sends float values from 0.1 for one wheel tick
        wheelDeltaX *= scrollK;
        wheelDeltaY *= scrollK;
    }
    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    bool isRelative = captureMode == eCursorCapture::PINNING;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseWheelEvent(window, x, y, wheelDeltaX, wheelDeltaY, modifierKeys, isRelative));
}

void WindowNativeBridge::KeyEvent(NSEvent* theEvent)
{
    uint32 key = [theEvent keyCode];
    bool isRepeated = [theEvent isARepeat];
    bool isPressed = [theEvent type] == NSKeyDown;

    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, type, key, 0, modifierKeys, isRepeated));

    if ([theEvent type] == NSKeyDown)
    {
        NSString* chars = [theEvent characters];
        NSUInteger n = [chars length];
        if (n > 0)
        {
            MainDispatcherEvent e = MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_CHAR, 0, 0, modifierKeys, false);
            for (NSUInteger i = 0; i < n; ++i)
            {
                uint32 key = [chars characterAtIndex:i];

                // Some key combinations can produce non-empty NSString with zero chars in it (e.g. ctrl + space)
                // Do not handle such symbols
                if (key > 0)
                {
                    e.keyEvent.keyVirtual = key;
                    mainDispatcher->PostEvent(e);
                }
            }
        }
    }
}

void WindowNativeBridge::FlagsChanged(NSEvent* theEvent)
{
    // Here we detect modifier key flags presses (Shift, Alt, Ctrl, Cmd, Capslock).
    // But Capslock is toggle key so we cannot determine it is pressed or unpressed
    // only is toggled and untoggled.

    static constexpr uint32 interestingFlags[] = {
        NX_DEVICELCTLKEYMASK,
        NX_DEVICERCTLKEYMASK,
        NX_DEVICELSHIFTKEYMASK,
        NX_DEVICERSHIFTKEYMASK,
        NX_DEVICELCMDKEYMASK,
        NX_DEVICERCMDKEYMASK,
        NX_DEVICELALTKEYMASK,
        NX_DEVICERALTKEYMASK,
        NX_ALPHASHIFTMASK, // Capslock
    };

    static constexpr uint32 flagsKeys[] = {
        kVK_Control,
        kVK_RightControl,
        kVK_Shift,
        kVK_RightShift,
        kVK_Command,
        0x36, // kVK_RightCommand from HIToolbox/Events.h, defined only on macOS 10.12+
        kVK_Option,
        kVK_RightOption,
        kVK_CapsLock
    };

    uint32 newModifierFlags = [theEvent modifierFlags];
    uint32 changedModifierFlags = newModifierFlags ^ lastModifierFlags;

    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    MainDispatcherEvent e = MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_DOWN, 0, 0, modifierKeys, false);

    for (int i = 0; i < COUNT_OF(interestingFlags); ++i)
    {
        const uint32 flag = interestingFlags[i];
        if (flag & changedModifierFlags)
        {
            uint32 scancode = flagsKeys[i];
            e.keyEvent.keyScancode = scancode;

            bool isPressed = (flag & newModifierFlags) == flag;
            e.type = isPressed ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;

            mainDispatcher->PostEvent(e);
        }
    }

    lastModifierFlags = newModifierFlags;
}

void WindowNativeBridge::MagnifyWithEvent(NSEvent* theEvent)
{
    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    float32 magnification = [theEvent magnification];

    NSSize sz = [renderView frame].size;
    NSPoint pt = [theEvent locationInWindow];

    float32 x = pt.x;
    float32 y = sz.height - pt.y;

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMagnificationGestureEvent(window, x, y, magnification, modifierKeys));
}

void WindowNativeBridge::RotateWithEvent(NSEvent* theEvent)
{
    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    float32 rotation = [theEvent rotation];
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowRotationGestureEvent(window, rotation, modifierKeys));
}

void WindowNativeBridge::SwipeWithEvent(NSEvent* theEvent)
{
    eModifierKeys modifierKeys = GetModifierKeys(theEvent);
    float32 deltaX = [theEvent deltaX];
    float32 deltaY = [theEvent deltaY];

    NSSize sz = [renderView frame].size;
    NSPoint pt = [theEvent locationInWindow];

    float32 x = pt.x;
    float32 y = sz.height - pt.y;

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSwipeGestureEvent(window, x, y, deltaX, deltaY, modifierKeys));
}

eModifierKeys WindowNativeBridge::GetModifierKeys(NSEvent* theEvent)
{
    // TODO: NSControlKeyMask, NSAlternateKeyMask, etc are deprecated in xcode 8 and replaced with NSEventModifierFlagControl, ...

    eModifierKeys result = eModifierKeys::NONE;
    NSEventModifierFlags flags = [theEvent modifierFlags];
    if (flags & NSShiftKeyMask)
    {
        result |= eModifierKeys::SHIFT;
    }
    if (flags & NSControlKeyMask)
    {
        result |= eModifierKeys::CONTROL;
    }
    if (flags & NSAlternateKeyMask)
    {
        result |= eModifierKeys::ALT;
    }
    if (flags & NSCommandKeyMask)
    {
        result |= eModifierKeys::COMMAND;
    }
    return result;
}

float32 WindowNativeBridge::GetDpi()
{
    NSScreen* screen = [NSScreen mainScreen];
    NSDictionary* description = [screen deviceDescription];
    NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
    CGSize displayPhysicalSize = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);

    // there being 25.4 mm in an inch
    return (displayPixelSize.width / displayPhysicalSize.width) * 25.4f;
}

eMouseButtons WindowNativeBridge::GetMouseButton(NSEvent* theEvent)
{
    eMouseButtons result = static_cast<eMouseButtons>([theEvent buttonNumber] + 1);
    if (eMouseButtons::FIRST <= result && result <= eMouseButtons::LAST)
    {
        return result;
    }
    return eMouseButtons::NONE;
}

void WindowNativeBridge::MouseEntered(NSEvent* theEvent)
{
    cursorInside = true;
    UpdateSystemCursorVisible();
    if (eCursorCapture::PINNING == captureMode)
    {
        SetSystemCursorCapture(true);
    }
}

void WindowNativeBridge::MouseExited(NSEvent* theEvent)
{
    cursorInside = false;
    UpdateSystemCursorVisible();
    if (eCursorCapture::PINNING == captureMode)
    {
        SetSystemCursorCapture(false);
    }
}

void WindowNativeBridge::SetCursorCapture(eCursorCapture mode)
{
    if (captureMode != mode)
    {
        captureMode = mode;
        switch (mode)
        {
        case eCursorCapture::FRAME:
            //not implemented
            break;
        case eCursorCapture::PINNING:
        {
            SetSystemCursorCapture(true);
            break;
        }
        case eCursorCapture::OFF:
        {
            SetSystemCursorCapture(false);
            break;
        }
        }
    }
}

void WindowNativeBridge::SetSystemCursorCapture(bool capture)
{
    if (capture)
    {
        CGAssociateMouseAndMouseCursorPosition(false);
        // set cursor in window center
        NSRect windowRect = [nswindow frame];
        NSRect screenRect = [[NSScreen mainScreen] frame];
        // Window origin is at bottom-left edge, but CGWarpMouseCursorPosition requires point in screen coordinates
        windowRect.origin.y = screenRect.size.height - (windowRect.origin.y + windowRect.size.height);
        CGPoint cursorpos;
        cursorpos.x = windowRect.origin.x + windowRect.size.width / 2.0f;
        cursorpos.y = windowRect.origin.y + windowRect.size.height / 2.0f;
        CGWarpMouseCursorPosition(cursorpos);
        mouseMoveSkipCount = SKIP_N_MOUSE_MOVE_EVENTS;
    }
    else
    {
        CGAssociateMouseAndMouseCursorPosition(true);
    }
}

void WindowNativeBridge::UpdateSystemCursorVisible()
{
    static bool mouseVisibleState = true;
    
#ifdef __DAVAENGINE_STEAM__ // fix for Steam overlay only
    bool visible = !cursorInside || mouseVisible;
#else
    bool visible = mouseVisible;
#endif
    if (mouseVisibleState != visible)
    {
        mouseVisibleState = visible;
        if (visible)
        {
            CGError check = CGDisplayShowCursor(kCGDirectMainDisplay);
            DVASSERT(kCGErrorSuccess == check);
        }
        else
        {
            CGError check = CGDisplayHideCursor(kCGDirectMainDisplay);
            DVASSERT(kCGErrorSuccess == check);
        }
    }
}

void WindowNativeBridge::SetCursorVisibility(bool visible)
{
    if (mouseVisible != visible)
    {
        mouseVisible = visible;
        UpdateSystemCursorVisible();
    }
}

void WindowNativeBridge::SetSurfaceScale(const float32 scale)
{
    surfaceScale = scale;
    HandleSizeChanging(WindowNativeBridge::SizeChangingReason::WindowSurfaceChanged);
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
