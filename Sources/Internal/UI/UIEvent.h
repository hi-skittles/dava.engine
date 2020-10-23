#pragma once

#include "Engine/EngineTypes.h"
#include "Math/Vector.h"
#include "Input/InputElements.h"

namespace DAVA
{
class UIControl;
class Window;
/**
    \ingroup controlsystem
    \brief User input representation.
    Represent user input event used in the control system. Contains all input data.
*/
class UIEvent
{
public:
    enum class Phase : int32
    {
        ERROR = 0,
        BEGAN, //!<Screen touch or mouse button press is began.
        DRAG, //!<User moves mouse with presset button or finger over the screen.
        ENDED, //!<Screen touch or mouse button press is ended.
        MOVE, //!<Mouse move event. Mouse moves without pressing any buttons. Works only with mouse controller.
        WHEEL, //!<Mouse wheel event. MacOS & Win32 only
        CANCELLED, //!<(ios only)Event was cancelled by the platform or by the control system for the some reason.
        CHAR, //!<Event some symbol was intered.
        CHAR_REPEAT, //!< Usefull if User hold key in text editor and wait
        KEY_DOWN,
        KEY_DOWN_REPEAT, //!< Usefull if user hold key in text editor and wait cursor to move
        KEY_UP,
        JOYSTICK,
        GESTURE, // mac os touch pad gestures only for now
    };

    /**
     \enum Input state accordingly to control.
     */
    enum eControlInputState
    {
        CONTROL_STATE_RELEASED = 0, //!<Input is released
        CONTROL_STATE_INSIDE, //!<Input processed inside control rerct for now
        CONTROL_STATE_OUTSIDE //!<Input processed outside of the control rerct for now
    };

    /**
     \ Input can be handled in the different ways.
     */
    enum eInputHandledType
    {
        INPUT_NOT_HANDLED = 0, //!<Input is not handled at all.
        INPUT_HANDLED_SOFT = 1, //!<Input is handled, but input control can be changed by GetEngineContext()->uiControlSystem->SwitchInputToControl() method.
        INPUT_HANDLED_HARD = 2, //!<Input is handled completely, input control can't be changed.
    };

    friend class UIControlSystem;

    UIEvent() = default;

    void SetInputHandledType(eInputHandledType value)
    {
        // Input Handled Type can be only increased.
        if (inputHandledType < value)
        {
            inputHandledType = value;
        }
    }

    eInputHandledType GetInputHandledType()
    {
        return inputHandledType;
    }

    void ResetInputHandledType()
    {
        inputHandledType = INPUT_NOT_HANDLED;
    }

    struct WheelDelta
    {
        float32 x;
        float32 y;
    };

    struct Gesture
    {
        float32 magnification; // delta -1..1
        float32 rotation; // delta angle in degrees -cw +ccw
        float32 dx; // -1..1 (-1 left)
        float32 dy; // -1..1 (-1 top)
    };

    union {
        uint32 touchId;
        eInputElements key;
        char32_t keyChar; // unicode utf32 char
        eMouseButtons mouseButton;
        eGamepadElements element;
        WheelDelta wheelDelta; // scroll delta in mouse wheel clicks (or lines)
        Gesture gesture; // pinch/rotate/swipe
    };
    Vector2 point; // point of pressure in virtual coordinates
    Vector2 physPoint; // point of pressure in physical coordinates
    bool isRelative = false; // cursor coordinates in eCursorCapture::PINNING mode
    float64 timestamp = 0.0; //(TODO not all platforms) time stemp of the event occurrence
    Phase phase = Phase::ERROR; // began, ended, moved. See Phase
    UIControl* touchLocker = nullptr; // control that handles this input
    int32 controlState = CONTROL_STATE_RELEASED; // input state relative to control (outside, inside). Used for point inputs only(mouse, touch)
    uint32 tapCount = 0; // (TODO not all platforms) count of the continuous inputs (clicks for mouse)
    eInputHandledType inputHandledType = INPUT_NOT_HANDLED; //!< input handled type, INPUT_NOT_HANDLED by default.
    eInputDevices device = eInputDevices::UNKNOWN;
    Window* window = nullptr;
    eModifierKeys modifiers = eModifierKeys::NONE;
};
}
