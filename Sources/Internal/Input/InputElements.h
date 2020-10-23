#pragma once

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
/**
    \addtogroup input
    \{
*/

/**
    List of all supported input elements.

    An input element is a part of a device which can be used for input. For example, a keyboard button, a mouse button, a mouse wheel, gamepad's stick etc.
    Number values used in this enum are safe to be saved in order to be restored later (e.g. in a config file).

    These values can be tested for different properties using functions defined below (i.e. what device type it belongs to, is it a button or not, etc.).
*/
enum eInputElements : uint32
{
    // Order of elements and values should not be changed
    // To provide backward compatibility in case some of these values are saved somewhere

    NONE = 0,

    // Keyboard scancode keys
    // These are named after characters they produce in QWERTY US layout
    // E.g. KB_W produces 'w' in QWERTY, but 'z' in AZERTY
    // TODO: except for Android, since we handle keycodes instead of scancodes there (see DavaSurfaceView.onKey). Needs some research

    // TODO: add support for virtual keys in addition to scancode keys we have now

    // TODO: order of keyboard elements is made to be backward compatible with old DAVA::Key implementation,
    // since these values could be saved to config file by a client.
    // Reorder these into more logical sequences and remove KB_UNUSED when switching to new action system

    KB_ESCAPE,
    KB_BACKSPACE,
    KB_TAB,
    KB_ENTER,
    KB_SPACE,
    KB_LSHIFT,
    KB_LCTRL,
    KB_LALT,
    KB_LCMD,
    KB_RCMD,
    KB_MENU,
    KB_PAUSE,
    KB_CAPSLOCK,
    KB_NUMLOCK,
    KB_SCROLLLOCK,
    KB_PAGEUP,
    KB_PAGEDOWN,
    KB_HOME,
    KB_END,
    KB_INSERT,
    KB_DELETE,
    KB_LEFT,
    KB_UP,
    KB_RIGHT,
    KB_DOWN,
    KB_0,
    KB_1,
    KB_2,
    KB_3,
    KB_4,
    KB_5,
    KB_6,
    KB_7,
    KB_8,
    KB_9,
    KB_A,
    KB_B,
    KB_C,
    KB_D,
    KB_E,
    KB_F,
    KB_G,
    KB_H,
    KB_I,
    KB_J,
    KB_K,
    KB_L,
    KB_M,
    KB_N,
    KB_O,
    KB_P,
    KB_Q,
    KB_R,
    KB_S,
    KB_T,
    KB_U,
    KB_V,
    KB_W,
    KB_X,
    KB_Y,
    KB_Z,
    KB_GRAVE,
    KB_MINUS,
    KB_EQUALS,
    KB_BACKSLASH,
    KB_LBRACKET,
    KB_RBRACKET,
    KB_SEMICOLON,
    KB_APOSTROPHE,
    KB_COMMA,
    KB_PERIOD,
    KB_SLASH,
    KB_NUMPAD_0,
    KB_NUMPAD_1,
    KB_NUMPAD_2,
    KB_NUMPAD_3,
    KB_NUMPAD_4,
    KB_NUMPAD_5,
    KB_NUMPAD_6,
    KB_NUMPAD_7,
    KB_NUMPAD_8,
    KB_NUMPAD_9,
    KB_MULTIPLY,
    KB_DIVIDE,
    KB_NUMPAD_PLUS,
    KB_NUMPAD_MINUS,
    KB_NUMPAD_DELETE,
    KB_F1,
    KB_F2,
    KB_F3,
    KB_F4,
    KB_F5,
    KB_F6,
    KB_F7,
    KB_F8,
    KB_F9,
    KB_F10,
    KB_F11,
    KB_F12,
    KB_UNUSED1, // Used to be Key::Back on Andorid, which does not exist anymore. Left for compatibility
    KB_UNUSED2, // Used to be Key::Menu on Android, which does not exist anymore. Left for compatibility
    KB_NONUSBACKSLASH,
    KB_NUMPAD_ENTER,
    KB_PRINTSCREEN,
    KB_RSHIFT,
    KB_RCTRL,
    KB_RALT,
    KB_F13,
    KB_F14,
    KB_F15,
    KB_F16,
    KB_F17,
    KB_F18,
    KB_F19,
    KB_LWIN,
    KB_RWIN,
    KB_CAMERA_FOCUS,
    KB_FUNCTION,
    KB_MEDIA_PREVIOUS,
    KB_MEDIA_NEXT,
    KB_MEDIA_PLAY_PAUSE,
    KB_MEDIA_EJECT,
    KB_VOLUME_MUTE,
    KB_VOLUME_UP,
    KB_VOLUME_DOWN,

    // Mouse

    MOUSE_LBUTTON = 1024, // Range from 1 to 1023 is reserved for keyboard
    MOUSE_RBUTTON,
    MOUSE_MBUTTON,
    MOUSE_EXT1BUTTON,
    MOUSE_EXT2BUTTON,
    MOUSE_WHEEL,
    MOUSE_POSITION,

    // Gamepad

    GAMEPAD_START = 1024 * 2, // Range from 1024 to 2047 is reserved for mouse
    GAMEPAD_A,
    GAMEPAD_B,
    GAMEPAD_X,
    GAMEPAD_Y,
    GAMEPAD_DPAD_LEFT,
    GAMEPAD_DPAD_RIGHT,
    GAMEPAD_DPAD_UP,
    GAMEPAD_DPAD_DOWN,
    GAMEPAD_LTHUMB,
    GAMEPAD_RTHUMB,
    GAMEPAD_LSHOULDER,
    GAMEPAD_RSHOULDER,

    GAMEPAD_AXIS_LTRIGGER,
    GAMEPAD_AXIS_RTRIGGER,
    GAMEPAD_AXIS_LTHUMB,
    GAMEPAD_AXIS_RTHUMB,

    // Touch

    TOUCH_CLICK0 = 1024 * 3, // Range from 2048 to 3071 is reserved for gamepad
    TOUCH_CLICK1,
    TOUCH_CLICK2,
    TOUCH_CLICK3,
    TOUCH_CLICK4,
    TOUCH_CLICK5,
    TOUCH_CLICK6,
    TOUCH_CLICK7,
    TOUCH_CLICK8,
    TOUCH_CLICK9,

    TOUCH_POSITION0,
    TOUCH_POSITION1,
    TOUCH_POSITION2,
    TOUCH_POSITION3,
    TOUCH_POSITION4,
    TOUCH_POSITION5,
    TOUCH_POSITION6,
    TOUCH_POSITION7,
    TOUCH_POSITION8,
    TOUCH_POSITION9,

    // This element is used for sending UIEvent indicating back navigation
    // TODO: make clients to use Window::backNavigation event only!
    BACK = 1024 * 4, // Range from 3071 to 4095 is reserved for touch screen

    // Counters

    FIRST = NONE,
    LAST = TOUCH_POSITION9,

    MOUSE_FIRST = MOUSE_LBUTTON,
    MOUSE_LAST = MOUSE_POSITION,
    MOUSE_FIRST_BUTTON = MOUSE_LBUTTON,
    MOUSE_LAST_BUTTON = MOUSE_EXT2BUTTON,

    KB_FIRST = KB_ESCAPE,
    KB_LAST = KB_VOLUME_DOWN,

    GAMEPAD_FIRST = GAMEPAD_START,
    GAMEPAD_LAST = GAMEPAD_AXIS_RTHUMB,
    GAMEPAD_FIRST_BUTTON = GAMEPAD_START,
    GAMEPAD_LAST_BUTTON = GAMEPAD_RSHOULDER,
    GAMEPAD_FIRST_AXIS = GAMEPAD_AXIS_LTRIGGER,
    GAMEPAD_LAST_AXIS = GAMEPAD_AXIS_RTHUMB,

    TOUCH_FIRST = TOUCH_CLICK0,
    TOUCH_LAST = TOUCH_POSITION9,

    TOUCH_FIRST_CLICK = TOUCH_CLICK0,
    TOUCH_LAST_CLICK = TOUCH_CLICK9,

    TOUCH_FIRST_POSITION = TOUCH_POSITION0,
    TOUCH_LAST_POSITION = TOUCH_POSITION9
};

// Helper values
enum
{
    INPUT_ELEMENTS_KB_COUNT = eInputElements::KB_LAST - eInputElements::KB_FIRST + 1,
    INPUT_ELEMENTS_MOUSE_BUTTON_COUNT = eInputElements::MOUSE_LAST_BUTTON - eInputElements::MOUSE_FIRST_BUTTON + 1,
    INPUT_ELEMENTS_TOUCH_CLICK_COUNT = eInputElements::TOUCH_LAST_CLICK - eInputElements::TOUCH_FIRST_CLICK + 1,
    INPUT_ELEMENTS_TOUCH_POSITION_COUNT = eInputElements::TOUCH_LAST_POSITION - eInputElements::TOUCH_FIRST_POSITION + 1
};

// Each touch should have both _CLICK and _POSITION pieces
static_assert(INPUT_ELEMENTS_TOUCH_CLICK_COUNT == INPUT_ELEMENTS_TOUCH_POSITION_COUNT, "Amount of touch clicks does not match amount of touch positions");

/** List of element types. */
enum class eInputElementTypes
{
    /** Button, which can just be pressed and released. */
    DIGITAL,

    /** Element whose state can only be described using multiple float values. */
    ANALOG
};

/** Contains additional information about an input element. */
struct InputElementInfo final
{
    /** Name of the element */
    String name;

    /** Type of the element */
    eInputElementTypes type;
};

/**
    Return true if specified `element` is a mouse element.
    This function is thread-safe.
 */
inline bool IsMouseInputElement(eInputElements element)
{
    return eInputElements::MOUSE_FIRST <= element && element <= eInputElements::MOUSE_LAST;
}

/**
    Return true if specified `element` is a mouse button element.
    This function is thread-safe.
 */
inline bool IsMouseButtonInputElement(eInputElements element)
{
    return eInputElements::MOUSE_FIRST_BUTTON <= element && element <= eInputElements::MOUSE_LAST_BUTTON;
}

/**
    Return true if specified `element` is a gamepad element.
    This function is thread-safe.
 */
inline bool IsGamepadInputElement(eInputElements element)
{
    return eInputElements::GAMEPAD_FIRST <= element && element <= eInputElements::GAMEPAD_LAST;
}

/**
    Return true if specified `element` is a gamepad button element.
    This function is thread-safe.
 */
inline bool IsGamepadButtonInputElement(eInputElements element)
{
    return eInputElements::GAMEPAD_FIRST_BUTTON <= element && element <= eInputElements::GAMEPAD_LAST_BUTTON;
}

/**
    Return true if specified `element` is a gamepad axis element.
    This function is thread-safe.
 */
inline bool IsGamepadAxisInputElement(eInputElements element)
{
    return eInputElements::GAMEPAD_FIRST_AXIS <= element && element <= eInputElements::GAMEPAD_LAST_AXIS;
}

/**
    Return true if specified `element` is a keyboard element.
    This function is thread-safe.
 */
inline bool IsKeyboardInputElement(eInputElements element)
{
    return eInputElements::KB_FIRST <= element && element <= eInputElements::KB_LAST;
}

/**
    Return true if specified keyboard `element` is a keyboard modifier element.
    This function is thread-safe.
 */
inline bool IsKeyboardModifierInputElement(eInputElements element)
{
    return (element == eInputElements::KB_LSHIFT ||
            element == eInputElements::KB_LCTRL ||
            element == eInputElements::KB_LALT ||
            element == eInputElements::KB_RSHIFT ||
            element == eInputElements::KB_RCTRL ||
            element == eInputElements::KB_RALT ||
            element == eInputElements::KB_LCMD ||
            element == eInputElements::KB_RCMD);
}

/**
    Return true if specified keyboard `element` is a keyboard 'system' element.
    This function is thread-safe.
 */
inline bool IsKeyboardSystemInputElement(eInputElements element)
{
    return (element == eInputElements::KB_ESCAPE ||
            element == eInputElements::KB_CAPSLOCK ||
            element == eInputElements::KB_LWIN ||
            element == eInputElements::KB_RWIN ||
            element == eInputElements::KB_PRINTSCREEN ||
            element == eInputElements::KB_SCROLLLOCK ||
            element == eInputElements::KB_PAUSE ||
            element == eInputElements::KB_INSERT ||
            element == eInputElements::KB_HOME ||
            element == eInputElements::KB_DELETE ||
            element == eInputElements::KB_END ||
            element == eInputElements::KB_NUMLOCK ||
            element == eInputElements::KB_MENU ||
            element == eInputElements::KB_CAMERA_FOCUS ||
            element == eInputElements::KB_FUNCTION ||
            element == eInputElements::KB_MEDIA_PREVIOUS ||
            element == eInputElements::KB_MEDIA_NEXT ||
            element == eInputElements::KB_MEDIA_PLAY_PAUSE ||
            element == eInputElements::KB_MEDIA_EJECT ||
            element == eInputElements::KB_VOLUME_DOWN ||
            element == eInputElements::KB_VOLUME_UP ||
            element == eInputElements::KB_VOLUME_MUTE);
}

/**
    Return true if specified `element` is a touch element.
    This function is thread-safe.
 */
inline bool IsTouchInputElement(eInputElements element)
{
    return eInputElements::TOUCH_FIRST <= element && element <= eInputElements::TOUCH_LAST;
}

/**
    Return true if specified `element` is a touch click element.
    This function is thread-safe.
 */
inline bool IsTouchClickInputElement(eInputElements element)
{
    return eInputElements::TOUCH_FIRST_CLICK <= element && element <= eInputElements::TOUCH_LAST_CLICK;
}

/**
    Return true if specified `element` is a touch position element.
    This function is thread-safe.
 */
inline bool IsTouchPositionInputElement(eInputElements element)
{
    return eInputElements::TOUCH_FIRST_POSITION <= element && element <= eInputElements::TOUCH_LAST_POSITION;
}

/**
    Return TOUCH_POSITION element for specified click `element. I.e. TOUCH_POSITION3 for TOUCH_CLICK3 etc.
    This function is thread-safe.
*/
inline eInputElements GetTouchPositionElementFromClickElement(eInputElements element)
{
    DVASSERT(IsTouchClickInputElement(element));

    return static_cast<eInputElements>(eInputElements::TOUCH_FIRST_POSITION + (element - eInputElements::TOUCH_FIRST_CLICK));
}

/**
    Get additional information about an element.
    This function is thread-safe.
*/
const InputElementInfo& GetInputElementInfo(eInputElements element);

// TODO: add glyphs api (i.e. getting element's `image` representation)

/**
    \}
*/

} // namespace DAVA
