#pragma once

#include "Base/BaseTypes.h"
#include "DeviceManager/DeviceManagerTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Functional/Function.h"

namespace DAVA
{
namespace Private
{
struct MainDispatcherEvent final
{
    enum eType : int32
    {
        DUMMY = 0,
        WINDOW_CREATED,
        WINDOW_DESTROYED,
        WINDOW_FOCUS_CHANGED,
        WINDOW_VISIBILITY_CHANGED,
        WINDOW_SIZE_CHANGED,
        WINDOW_DPI_CHANGED,
        WINDOW_CAPTURE_LOST,
        WINDOW_CANCEL_INPUT,
        WINDOW_VISIBLE_FRAME_CHANGED,
        WINDOW_SAFE_AREA_INSETS_CHANGED,

        FIRST_INPUT_EVENT,
        MOUSE_BUTTON_DOWN = FIRST_INPUT_EVENT,
        MOUSE_BUTTON_UP,
        MOUSE_WHEEL,
        MOUSE_MOVE,

        TOUCH_DOWN,
        TOUCH_UP,
        TOUCH_MOVE,

        TRACKPAD_GESTURE,

        GAMEPAD_BUTTON_DOWN,
        GAMEPAD_BUTTON_UP,
        GAMEPAD_MOTION,

        KEY_DOWN,
        KEY_UP,
        KEY_CHAR,
        LAST_INPUT_EVENT = KEY_CHAR,

        FUNCTOR,

        BACK_NAVIGATION,
        APP_SUSPENDED,
        APP_RESUMED,

        USER_CLOSE_REQUEST,
        APP_TERMINATE,

        GAMEPAD_ADDED,
        GAMEPAD_REMOVED,

        DISPLAY_CONFIG_CHANGED,

        INPUT_LANGUAGE_CHANGED,

        LOW_MEMORY
    };

    static bool IsInputEvent(eType type);

    /// Parameter for APP_TERMINATE event
    struct AppTerminateEvent
    {
        /// Flag indicating whether termination was initiated by system (value 1) or by application (value 0).
        /// System initiates termination on some platforms:
        ///     - on android when activity is finishing
        ///     - on mac when user pressed cmd+q
        uint32 triggeredBySystem;
    };

    /// Parameter for events:
    ///     - WINDOW_FOCUS_CHANGED: window got focus (value 1) or lost focus (value 0)
    ///     - WINDOW_VISIBILITY_CHANGED: window became visible (value 1) or became hidden (value 0)
    struct WindowStateEvent
    {
        uint32 state;
    };

    /// Parameter for WINDOW_DESTROYED event
    struct WindowDestroyedEvent
    {
        /// Flag indicating whether native window was truly destroyed (value 0) or detached from DAVA::Window instance (value 1)
        /// Windows are detached only on app exit
        uint32 detached;
    };

    /// Parameter for events:
    ///     - WINDOW_CREATED
    ///     - WINDOW_SIZE_CHANGED
    struct WindowSizeEvent
    {
        float32 width;
        float32 height;
        float32 surfaceWidth;
        float32 surfaceHeight;
        float32 surfaceScale;
        float32 dpi; //< is set only by WINDOW_CREATED
        eFullscreen fullscreen;
    };

    /// Parameter for event WINDOW_DPI_CHANGED
    struct WindowDpiEvent
    {
        float32 dpi;
    };

    /// Parameter for event WINDOW_VISIBLE_FRAME_CHANGED
    struct WindowVisibleFrameEvent
    {
        float32 x;
        float32 y;
        float32 width;
        float32 height;
    };

    /// Parameter for event WINDOW_SAFE_AREA_INSETS_CHANGED
    struct WindowSafeAreaInsetsEvent
    {
        float32 left;
        float32 top;
        float32 right;
        float32 bottom;
        bool isLeftNotch;
        bool isRightNotch;
    };

    /// Parameter for mouse events:
    ///     - MOUSE_BUTTON_DOWN
    ///     - MOUSE_BUTTON_UP
    ///     - MOUSE_MOVE
    ///     - MOUSE_WHEEL
    struct MouseEvent
    {
        eMouseButtons button; // What button is pressed (MOUSE_BUTTON_DOWN and MOUSE_BUTTON_UP)
        eModifierKeys modifierKeys; // Modifier keys accompanying mouse event (shift, alt, control)
        uint32 clicks; // Number of button clicks (MOUSE_BUTTON_DOWN)
        float32 x; // Point where mouse click, mouse wheel occured or point where mouse is moved,
        float32 y; // if isRelative then point designate relative mouse move not absolute
        float32 scrollDeltaX; // Scroll deltas for
        float32 scrollDeltaY; //      MOUSE_WHEEL event
        bool isRelative : 1; // Mouse event occured while window cursor is in pinning mode
    };

    /// Parameter for touch events:
    ///     - TOUCH_DOWN
    ///     - TOUCH_UP
    ///     - TOUCH_MOVE
    struct TouchEvent
    {
        uint32 touchId;
        eModifierKeys modifierKeys; // Modifier keys accompanying touch event (shift, alt, control)
        float32 x;
        float32 y;
    };

    // Parameter for TRACKPAD_GESTURE event
    struct TrackpadGestureEvent
    {
        float32 magnification;
        float32 rotation;
        float32 deltaX;
        float32 deltaY;
        float32 x;
        float32 y;
        eModifierKeys modifierKeys;
    };

    // Parameter for gamepad events:
    //      - GAMEPAD_BUTTON_DOWN
    //      - GAMEPAD_BUTTON_UP
    //      - GAMEPAD_MOTION
    struct GamepadEvent
    {
        uint32 deviceId;
        uint32 button; // Button identifier as reported by system for GAMEPAD_BUTTON_DOWN and GAMEPAD_BUTTON_UP
        uint32 axis; // Axis identifier as reported by system for GAMEPAD_MOTION
        // Axis value for GAMEPAD_MOTION, usually normalized to [-1.0, 1.0] or [0.0, -1.0].
        // Or button state for GAMEPAD_BUTTON_DOWN and GAMEPAD_BUTTON_UP.
        // Some strange behaviour is necessary due to odd DAVA::GamepadDevice implementation:
        //  - some buttons are translated into range [-1, 1], e.g. Dpad left is -1, Dpad right is 1, no Dpad is 0
        //  - other buttons state (A, B, X, Y) are also interpreted in the following way: 1 if button is down, 0 - otherwise
        float32 value;
    };

    /// Parameter for events:
    ///     - KEY_DOWN
    ///     - KEY_UP
    ///     - KEY_CHAR
    struct KeyEvent
    {
        uint32 keyScancode;
        uint32 keyVirtual;
        eModifierKeys modifierKeys; // Modifier keys accompanying key event (shift, alt, control)
        bool isRepeated;
    };

    // Parameter for DISPLAY_CONFIG_CHANGED event
    // Handler is responsible for freeing displayInfo (delete[] displayInfo)
    struct DisplayConfigEvent
    {
        DisplayInfo* displayInfo;
        size_t count;
    };

    MainDispatcherEvent() = default;
    MainDispatcherEvent(eType type)
        : type(type)
    {
    }
    MainDispatcherEvent(Window* window)
        : window(window)
    {
    }
    MainDispatcherEvent(eType type, Window* window)
        : type(type)
        , window(window)
    {
    }

    eType type = DUMMY;
    int64 timestamp = 0;
    Window* window = nullptr;
    Function<void()> functor;
    union
    {
        AppTerminateEvent terminateEvent;
        WindowStateEvent stateEvent;
        WindowDestroyedEvent destroyedEvent;
        WindowSizeEvent sizeEvent;
        WindowDpiEvent dpiEvent;
        WindowVisibleFrameEvent visibleFrameEvent;
        WindowSafeAreaInsetsEvent safeAreaInsetsEvent;
        MouseEvent mouseEvent;
        TouchEvent touchEvent;
        TrackpadGestureEvent trackpadGestureEvent;
        GamepadEvent gamepadEvent;
        KeyEvent keyEvent;
        DisplayConfigEvent displayConfigEvent;
    };

    template <typename F>
    static MainDispatcherEvent CreateFunctorEvent(F&& functor);

    static MainDispatcherEvent CreateAppTerminateEvent(bool triggeredBySystem);
    static MainDispatcherEvent CreateUserCloseRequestEvent(Window* window);

    static MainDispatcherEvent CreateGamepadAddedEvent(uint32 deviceId);
    static MainDispatcherEvent CreateGamepadRemovedEvent(uint32 deviceId);
    static MainDispatcherEvent CreateGamepadMotionEvent(uint32 deviceId, uint32 axis, float32 value);
    static MainDispatcherEvent CreateGamepadButtonEvent(uint32 deviceId, eType gamepadButtonEventType, uint32 button);

    static MainDispatcherEvent CreateDisplayConfigChangedEvent(DisplayInfo* displayInfo, size_t count);

    static MainDispatcherEvent CreateWindowCreatedEvent(Window* window, float32 w, float32 h, float32 surfaceW, float32 surfaceH, float32 dpi, eFullscreen fullscreen);
    static MainDispatcherEvent CreateWindowDestroyedEvent(Window* window);
    static MainDispatcherEvent CreateWindowSizeChangedEvent(Window* window, float32 w, float32 h, float32 surfaceW, float32 surfaceH, float32 surfaceScale, float32 dpi, eFullscreen fullscreen);
    static MainDispatcherEvent CreateWindowFocusChangedEvent(Window* window, bool focusState);
    static MainDispatcherEvent CreateWindowVisibilityChangedEvent(Window* window, bool visibilityState);
    static MainDispatcherEvent CreateWindowDpiChangedEvent(Window*, float32 dpi);
    static MainDispatcherEvent CreateWindowCancelInputEvent(Window* window);
    static MainDispatcherEvent CreateWindowVisibleFrameChangedEvent(Window* window, float32 x, float32 y, float32 width, float32 height);
    static MainDispatcherEvent CreateWindowSafeAreaInsetsChangedEvent(Window* window, float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch, bool isRightNotch);

    static MainDispatcherEvent CreateWindowKeyPressEvent(Window* window, eType keyEventType, uint32 keyScancode, uint32 keyVirtual, eModifierKeys modifierKeys, bool isRepeated);
    static MainDispatcherEvent CreateWindowMouseClickEvent(Window* window, eType mouseClickEventType, eMouseButtons button, float32 x, float32 y, uint32 clicks, eModifierKeys modifierKeys, bool isRelative);
    static MainDispatcherEvent CreateWindowMouseMoveEvent(Window* window, float32 x, float32 y, eModifierKeys modifierKeys, bool isRelative);
    static MainDispatcherEvent CreateWindowMouseWheelEvent(Window* window, float32 x, float32 y, float32 deltaX, float32 deltaY, eModifierKeys modifierKeys, bool isRelative);
    static MainDispatcherEvent CreateWindowTouchEvent(Window* window, eType touchEventType, uint32 touchId, float32 x, float32 y, eModifierKeys modifierKeys);
    static MainDispatcherEvent CreateWindowMagnificationGestureEvent(Window* window, float32 x, float32 y, float32 magnification, eModifierKeys modifierKeys);
    static MainDispatcherEvent CreateWindowRotationGestureEvent(Window* window, float32 rotation, eModifierKeys modifierKeys);
    static MainDispatcherEvent CreateWindowSwipeGestureEvent(Window* window, float32 x, float32 y, float32 deltaX, float32 deltaY, eModifierKeys modifierKeys);
    static MainDispatcherEvent CreateWindowCaptureLostEvent(Window* window);

    static MainDispatcherEvent CreateInputLanguageChangedEvent();
};

template <typename F>
MainDispatcherEvent MainDispatcherEvent::CreateFunctorEvent(F&& functor)
{
    MainDispatcherEvent e(MainDispatcherEvent::FUNCTOR);
    e.functor = std::forward<F>(functor);
    return e;
}

} // namespace Private
} // namespace DAVA
