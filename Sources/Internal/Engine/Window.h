#pragma once

#include "Base/BaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/EngineBackend.h"
#include "Functional/Signal.h"
#include "Math/Math2D.h"
#include "Math/Rect.h"
#include "Math/Vector.h"

#include <bitset>

namespace rhi
{
struct InitParam;
}

namespace DAVA
{
class InputSystem;
class UIControlSystem;
class VirtualCoordinatesSystem;

/** Describes input handling modes for `Window` class */
enum class eInputHandlingModes
{
    /** Window handles input only when focused */
    HANDLE_ONLY_WHEN_FOCUSED,

    /** Window handles input in both focused and unfocused states */
    HANDLE_ALWAYS
};

/**
    \ingroup engine
    The `Window` class represents an abstract window - rectangular area on display application can draw at.
    Window class instance relates to window object in the underlying windowing system.

    The Window class has these types of members:
        - signals which are fired when window properties or state has changed (e.g., window crated, focus state changed, size changed).
        - properties which describe window state (e.g., focus state, size).
        - methods which can change properties (e.g., resize window, close window).

    Window class lifetime includes the following stages:
        1. Window class instantiation. Application can subscribe to signals, set initial properties (through methods with `Async` suffix)
           which will be applied after stage 2. Note that properties (size, DPI, etc) do not have any meaningful values yet.
           Window class instantiation is always performed by dava.engine in response of application request.
        2. Underlying native window creation: application gets notified through `Engine::windowCreated` signal. When signal is delivered to application
           dava.engine has already initialized renderer for window and window become fully functional.
        3. Window life cycle.
        4. Underlying native window destruction: application gets notified through `Engine::windowDestroyed` signal. Application shall not use
           window instance any more.
        5. Window class instance deletion, hidden from application.

    Window has special attribute denoting whether window is primary:
        - primary window is the first Window instance created by dava.engine.
        - primary window becomes available to application after `Engine::Init` method has been invoked, except when `Engine` has been
          initialized to run in console mode (`eEngineRunMode::CONSOLE_MODE`).
        - closing primary window leads to application exit.
    Application can get primary window instance through `Engine::PrimaryWindow` method or throught freestanding `GetPrimaryWindow` function.

    Window signals sequence denoting window state:
        1. `Engine::windowCreated`, next state is always 2.
        2. `Window::visibilityChanged(true)`, next state is 3 or 5.
        3. `Window::focusChanged(true)`, next state is always 4.
        4. `Window::focusChanged(false)`, next state is 3 or 5.
        5. `Window::visibilityChanged(false)`, next state is 2 or 6.
        6. `Engine::windowDestroyed`.

    Window visibility meaning:
        - on desktop platfroms window is considered invisible if it has been minimized.
        - on mobile platfroms window is considered invisible usually if application has gone background.

    \note Window class methods with `Async` suffix apply window properties or change window state a bit later (usually in 1 or 2 frames),
          application can get to know about real changes through signals.

    \todo Secondary windows are not implemented yet.
*/
class Window final
{
    friend class Private::EngineBackend;
    friend class Private::PlatformCore;

private:
    Window(Private::EngineBackend* engineBackend, bool primary);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

public:
    /** Check whether window is primary */
    bool IsPrimary() const;

    /** Check whether window has alive native window */
    bool IsAlive() const;

    /**
        Check whether window is visible.

        Use `visibilityChanged` signal to track visibility changing.
    */
    bool IsVisible() const;

    /**
        Check whether window has keyboard focus.

        No input events are delivered to `InputSystem` if window has no focus.

        Use `focusChanged` signal to track focus changing.
    */
    bool HasFocus() const;

    /** 
        Return current dots-per-inch (DPI) setting for a display where window is placed on.

        Use `dpiChanged` signal to track DPI changing which occurs when window's display DPI has changed or
        window has been moved to another display with different DPI.
    */
    float32 GetDPI() const;

    /** 
        Return size of the window's client area in device-independent pixels (DIPs).

        Operating systems use DIPs to position windows and other interface elements on display.
        Coordinates for touch and mouse input events are also specified in DIPs.

        DIP size is expressed in physical pixels and depends on system settings. Usually there is a
        one-to-one correspondence between DIP and physical pixel, but on systems with high-DPI display
        support DIP can contain 1.5, 2, or even 3 physical pixels. Some systems allow users to implicitly
        change DIP size (e.g. Windows gives facilities to change text size 100%, 125%, 200% and DIP will
        occupy 1, 1.25 or 2 physical pixels respectively).

        Window's surface size is always specified in physical pixels.

        Use `sizeChanged` signal to track window size changing.
    */
    Size2f GetSize() const;

    /** 
        Request to set window's client area size specified in DIPs (see `GetSize` method).

        Request has no effect in these cases:
            - window does not support resizing, this is the case on mobile platforms.
            - the requested size is greater than the available work area which depends on system settings.
            - the requested size is less than minimum size defined by application or system.

        Performed asynchronously, use `sizeChanged` signal to track window size changing.
    */
    void SetSizeAsync(Size2f size);

    /**
        Request to activate minimized or inactive window, i.e. put it at the top of window hierarchy to be visible by user.

        Note that this method is supported only on desktop platforms except Win10.

        Performed asynchronously, window activation state changes can be traced by `visibilityChanged` and `focusChanged` signals.
    */
    void ActivateAsync();

    /**
        Set the smallest size, in DIPs, of window client area.

        Smallest and default minimum size is 128x128. Setting minimum size to {0, 0} applies default minimum size {128x128}.

        Minimum window size setting is applied only to platforms which support window size changing, usually desktops.
        Some platforms limit largest minimum size, e.g. Win10 limits largest minimum size to 500x500 DIPs.

        If preferred minimum size is larger than current window size window is resized to given minimum size.
    */
    void SetMinimumSize(Size2f size);

    /**
         Return window rendering surface size in physical pixels.

         Surface size depends on window size together with DPI (or more precisely DIP size) and surface scale factor.
         Application can indirectly specify surface size by applying scale factor through `SetSurfaceScale` method.

         Use `sizeChanged` signal to track surface size changing.
    */
    Size2f GetSurfaceSize() const;

    /** 
        Return window rendering surface scale.

        Default value is 1.
    */
    float32 GetSurfaceScale() const;

    /** 
        Request to set window rendering surface scale.

        \pre Scale value should be in range (0, 1].

        Request has no effect if underlying rendering system does not support surface scaling, as 
        in the case of OpenGL on Win32.

        Surface scaling is usually used to tune rendering performance by decreasing surface size.

        Performed asynchronously, use `sizeChanged` signal and `GetSurfaceScale` method inside signal handler to track scale changing.
    */
    void SetSurfaceScaleAsync(float32 scale);

    /**
        Set window virtual size.

        Virtual size is specified in an application-defined units and used to draw UI controls.
        Virtual size is usually set on window creation or in response to `sizeChanged` signal.

        By default window has virtual size of 1024x768 units.
    */
    void SetVirtualSize(float32 w, float32 h);

    /** Get window virtual size. */
    Size2f GetVirtualSize() const;

    /**
        Request to close window.

        Performed asynchronously, use `Engine::windowDestroyed` signal to get to know that window is destroying.

        \note Closing primary window leads to application exit.
    */
    void CloseAsync();

    /**
        Request to set window title if underlying window system allows.

        Performed asynchronously.
    */
    void SetTitleAsync(const String& title);

    /** Get current window mode: fullscreen or windowed. */
    eFullscreen GetFullscreen() const;

    /**
        Request to switch window to fullscreen or windowed mode.

        Request has no effect in these cases:
            - window does not support fullscreen switching.
            - window is already in specified mode.

        Performed asynchronously, use `sizeChanged` signal and `GetFullscreen` method inside signal handler to track mode changing.
    */
    void SetFullscreenAsync(eFullscreen newMode);

    Engine* GetEngine() const;
    void* GetNativeHandle() const;

    /**
        Run task on window's UI thread.
        This method can be called from any thread.
    */
    void RunOnUIThreadAsync(const Function<void()>& task);

    /**
        Run task on window's UI thread and wait its completion.
        This method can be called from any thread.
    */
    void RunOnUIThread(const Function<void()>& task);

    /**
        Request to set mouse cursor capture mode.

        Request has no effect if underlying window system does not support cursor mode changing (mobile platforms).

        For application cursor capture mode remains intact until next `SetCursorCapture` call with other value but Window
        always cancels capture mode after losing focus and automatically restores it on gaining focus.
        Application can recognize whether `eCursorCapture::PINNING` is enabled by examining UIEvent::isRelative field when
        processing input events from mouse device. Also in `eCursorCapture::PINNING` mode cursor is automatically become hidden.
    */
    void SetCursorCapture(eCursorCapture mode);

    /**
        Get cursor capture mode.

        Returned value is always the same as previously set by `SetCursorCapture`. Default value is `eCursorCapture::OFF`.
    */
    eCursorCapture GetCursorCapture() const;

    /**
        Request to show or hide mouse cursor for this window.

        Request has no effect if underlying window system does not support cursor hiding (mobile platforms).

        For application cursor visibility state remains intact until next `SetCursorVisibility` call with other value but Window
        always shows cursor after losing focus and automatically hides it on gaining focus.
        Application cannot control cursor visibility when `eCursorCapture::PINNING` mode is enabled, in this case cursor is
        automatically hidden.
    */
    void SetCursorVisibility(bool visible);

    /**
        Get cursor visibility.

        Returned value is always the same as previously set by `SetCursorVisibility`. Default value is true.
    */
    bool GetCursorVisibility() const;

    /**
        Set input handling mode.

        By default, eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED is used.
    */
    void SetInputHandlingMode(eInputHandlingModes mode);

    /** Get Window's UIControlSystem */
    UIControlSystem* GetUIControlSystem() const;

    static const int smallestWidth = 256; //<! Smallest window width that window can be resized ever (desktops only)
    static const int smallestHeight = 256; //<! Smallest window height that window can be resized ever (desktops only)

public:
    // Signals
    Signal<Window*, bool /*visible*/> visibilityChanged; //<! Emitted when window visibility has changed.
    Signal<Window*, bool /*hasFocus*/> focusChanged; //<! Emitted when window has gained or lost keyboard focus.
    Signal<Window*, eCursorCapture /*cursorCapture*/> cursorCaptureChanged; //<!Emitted when window cursor capture mode has changed.
    Signal<Window*, float32> dpiChanged; //<! Emitted when DPI of the display where window is on has changed.
    Signal<Window*, Size2f /* windowSize*/, Size2f /* surfaceSize */> sizeChanged; //<! Emitted when window client ares size or surface size has changed.
    Signal<Window*, float32> update; //!< Emitted on each frame if window is visible.
    Signal<Window*> draw; //!< Emited after `update` signal after `UIControlSystem::Draw`
    Signal<Window*, Rect /*visibleFrameRect*/> visibleFrameChanged; //!< Emitted when window visible frame changed (showed virtual keyboard over window).
    Signal<Window*> backNavigation; //!< Emitted when user presses a back button or its alternative (like Win + Backspace on UWP).

private:
    /// Initialize platform specific render params, e.g. acquire/release context functions for Qt platform
    void InitCustomRenderParams(rhi::InitParam& params);
    void Update(float32 frameDelta);
    void Draw();

    /// Process main dispatcher events targeting this window
    bool EventHandler(const Private::MainDispatcherEvent& e);
    /// Do some window specific tasks after all dispatcher events have been processed on current frame,
    /// e.g. initiate processing tasks on window UI thread
    void FinishEventHandlingOnCurrentFrame();

    void HandleWindowCreated(const Private::MainDispatcherEvent& e);
    void HandleWindowDestroyed(const Private::MainDispatcherEvent& e);
    void HandleCursorCaptureLost(const Private::MainDispatcherEvent& e);
    void HandleSizeChanged(const Private::MainDispatcherEvent& e);
    void HandleDpiChanged(const Private::MainDispatcherEvent& e);
    void HandleCancelInput(const Private::MainDispatcherEvent& e);
    void HandleVisibleFrameChanged(const Private::MainDispatcherEvent& e);
    void HandleSafeAreaInsetsChanged(const Private::MainDispatcherEvent& e);
    void HandleFocusChanged(const Private::MainDispatcherEvent& e);
    void HandleVisibilityChanged(const Private::MainDispatcherEvent& e);
    void HandleTrackpadGesture(const Private::MainDispatcherEvent& e);
    bool HandleInputActivation(const Private::MainDispatcherEvent& e);

    void MergeSizeChangedEvents(const Private::MainDispatcherEvent& e);
    void UpdateVirtualCoordinatesSystem();

private:
    Private::EngineBackend* engineBackend = nullptr;
    Private::MainDispatcher* mainDispatcher = nullptr;
    std::unique_ptr<Private::WindowImpl> windowImpl;

    InputSystem* inputSystem = nullptr;
    UIControlSystem* uiControlSystem = nullptr;

    bool isPrimary = false;
    bool isAlive = false;
    bool isVisible = false;
    bool hasFocus = false;
    bool sizeEventsMerged = false; // Flag indicating that all size events are merged on current frame
    eFullscreen fullscreenMode = eFullscreen::Off;

    eCursorCapture cursorCapture = eCursorCapture::OFF;
    bool cursorVisible = false;
    bool waitInputActivation = false;
    float32 dpi = 0.0f; //!< Window DPI
    float32 width = 0.0f; //!< Window client area width.
    float32 height = 0.0f; //!< Window client area height.
    float32 surfaceWidth = 0.0f; //!< Window rendering surface width.
    float32 surfaceHeight = 0.0f; //!< Window rendering surface height.
    float32 surfaceScale = 1.0f; //!< Window rendering surface scale.

    eInputHandlingModes inputHandlingMode = eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED;
};

inline bool Window::IsPrimary() const
{
    return isPrimary;
}

inline bool Window::IsAlive() const
{
    return isAlive;
}

inline bool Window::IsVisible() const
{
    return isVisible;
}

inline bool Window::HasFocus() const
{
    return hasFocus;
}

inline float32 Window::GetDPI() const
{
    return dpi;
}

inline Size2f Window::GetSize() const
{
    return { width, height };
}

inline Size2f Window::GetSurfaceSize() const
{
    return { surfaceWidth, surfaceHeight };
}

inline float32 Window::GetSurfaceScale() const
{
    return surfaceScale;
}

inline UIControlSystem* Window::GetUIControlSystem() const
{
    return uiControlSystem;
}

inline eFullscreen Window::GetFullscreen() const
{
    return fullscreenMode;
}

} // namespace DAVA
