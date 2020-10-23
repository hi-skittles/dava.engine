#pragma once

#include "Base/BaseTypes.h"
#include "Math/Math2D.h"

namespace DAVA
{
/**
    \ingroup engine
    Engine run modes.
*/
enum class eEngineRunMode : int32
{
    GUI_STANDALONE = 0, //!< Run engine as standalone GUI application
    GUI_EMBEDDED, //!< Run engine inside other framework, e.g. Qt
    CONSOLE_MODE //!< Run engine as standalone console application
};

/**
    \ingroup engine
    Constants that name fullscreen modes.
*/
enum class eFullscreen : uint32
{
    On = 0, //<! Full screen mode on
    Off, //<! Full screen mode off (windowed mode)
};

/**
    \ingroup engine
    Constants that name supported input device types.
    Bitwise operators can be applied to enum members (|, |=, &, &=, ^, ^=, ~).
*/
enum class eInputDevices : uint32
{
    UNKNOWN = 0, //!< Special value used in some case to specify that input device is unrecognized
    TOUCH_SURFACE = 0x0100, //!< Touch surface like touch screen on mobile devices
    MOUSE = 0x0200,
    KEYBOARD = 0x0400,
    GAMEPAD = 0x0800,
    PEN = 0x1000,
    TOUCH_PAD = 0x2000, //!< Touch pad which can be found on notebooks

    CLASS_ALL = TOUCH_SURFACE | MOUSE | KEYBOARD | GAMEPAD | PEN | TOUCH_PAD,
    CLASS_POINTER = TOUCH_SURFACE | MOUSE | PEN | TOUCH_PAD,
    CLASS_KEYBOARD = KEYBOARD,
    CLASS_GAMEPAD = GAMEPAD,
};

using eInputDeviceTypes = eInputDevices;

DAVA_DEFINE_ENUM_BITWISE_OPERATORS(eInputDevices)

/**
    \ingroup engine
    Translate integer value to `eInputDevices` enumeration with respect to pre-corev2 device constants values.

    This function is introduced for compatibility reason between corev1 and corev2 as game saves `eInputDevices`
    values on server.

    If integer value does not correspond to any `eInputDevices` enumeration returns `eInputDevices::UNKNOWN`.
*/
eInputDevices TranslateUIntToInputDevice(uint32 value);

/**
    \ingroup engine
    Constants that name mouse buttons.
*/
enum class eMouseButtons : uint32
{
    NONE = 0, //!< Special value used in some cases to specify that no mouse button is involved

    LEFT = 1,
    RIGHT = 2,
    MIDDLE = 3,
    EXTENDED1 = 4,
    EXTENDED2 = 5,

    FIRST = LEFT,
    LAST = EXTENDED2,

    COUNT = LAST, //!< Number of supported mouse buttons
};

/**
    \ingroup engine
    Modifier keys (shift, alt, control, etc) that accompany some input events (mouse, touch, key).
    Bitwise operators can be applied to enum members (|, |=, &, &=, ^, ^=, ~).
*/
enum class eModifierKeys : uint32
{
    NONE = 0, //!< No modifier keys are pressed

    SHIFT = 0x01, //!< Any shift is pressed
    CONTROL = 0x02, //!< Any control is pressed
    ALT = 0x04, //!< Any alt is pressed
    COMMAND = 0x08, //!< Any command key is pressed (macOS only)

    FIRST = SHIFT,
    LAST = COMMAND,

    MASK = LAST | ~LAST, //!< Value used to mask useful bits
};

DAVA_DEFINE_ENUM_BITWISE_OPERATORS(eModifierKeys)

/**
    \ingroup engine
    Constants that name gamepad elements - buttons and axes.
*/
enum class eGamepadElements : uint32
{
    A = 0,
    B,
    X,
    Y,
    LEFT_SHOULDER,
    RIGHT_SHOULDER,
    LEFT_TRIGGER,
    RIGHT_TRIGGER,
    LEFT_THUMBSTICK_X,
    LEFT_THUMBSTICK_Y,
    RIGHT_THUMBSTICK_X,
    RIGHT_THUMBSTICK_Y,
    DPAD_X,
    DPAD_Y,

    LAST = DPAD_Y,
};

/**
    \ingroup engine
    Constants that name gamepad profiles.
*/
enum class eGamepadProfiles : uint32
{
    SIMPLE = 0, //!< Two shoulder buttons, directional pad
    EXTENDED, //!< Two shoulder buttons, two triggers, two thumbsticks, directional pad
};

/** Cursor capture modes */
enum class eCursorCapture : int32
{
    OFF = 0, //!< Disable any capturing(send absolute xy)
    FRAME, //!< Capture system cursor into window rect(send absolute xy) */
    PINNING, //!< Capture system cursor on current position(send xy move delta) */
};

} // namespace DAVA
