#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/**
    \addtogroup input
    \{
*/

/**
    Describes digital element state.

    Digital element is a button, and thus it can be in four different states, which are represented by this structure:
        - Released: button is released right now, and it happened at least one frame age
        - Just pressed: button has just been pressed during this frame
        - Pressed: button is pressed right now, but it happened at least one frame ago
        - Just released: button has been released during this frame

    Instances of this structure can be tested for required state using these four methods:
    - `DigitalElementState::IsPressed()`
    - `DigitalElementState::IsJustPressed()`
    - `DigitalElementState::IsReleased()`
    - `DigitalElementState::IsJustReleased()`

    For example:
    \code
    Keyboard* kb = deviceManager->GetKeyboard();
    if (kb->GetKeyState(eInputElements::KB_TAB).IsJustPressed())
    {
        // Tab button has just been pressed, do something
        // ...
    }
    \endcode

    Below is the table that shows how these states change during a typical flow: when a user presses a button, holds it for some time, and finally releases it:
    |                                                  | IsPressed() | IsJustPressed() | IsReleased() | IsJustReleased() |
    |--------------------------------------------------|-------------|-----------------|--------------|------------------|
    | Initial element state                            | -           | -               | +            | -                |
    | Right after a user pressed a button (same frame) | +           | +               | -            | -                |
    | A user keeps the button pressed (next frames)    | +           | -               | -            | -                |
    | A user released the button (same frame)          | -           | -               | +            | +                |
    | A user released the button (next frames)         | -           | -               | +            | -                |
*/
struct DigitalElementState final
{
    /** Return flag indicating if a button is pressed. */
    bool IsPressed() const
    {
        return pressed;
    }

    /** Return flag indicating if a button has just been pressed. */
    bool IsJustPressed() const
    {
        return (pressed && justChanged);
    }

    /** Return flag indicating if a button is released. */
    bool IsReleased() const
    {
        return !pressed;
    }

    /** Return flag indicating if a button has just been released. */
    bool IsJustReleased() const
    {
        return (!pressed && justChanged);
    }

    // Methods below are useful for implementing `InputDevice` classes

    /**
        Drops `justChanged` internal flag, thus changing JustPressed and JustReleased states to Pressed and Released accordingly.
        This method should be called at the end of a frame by `InputDevice` implementations, hence the name.
    */
    void OnEndFrame()
    {
        justChanged = false;
    }

    /** Create instance of `DigitalElementState` in pressed state. */
    static DigitalElementState Pressed()
    {
        DigitalElementState result;
        result.pressed = true;
        result.justChanged = false;

        return result;
    }

    /** Create instance of `DigitalElementState` in just pressed state. */
    static DigitalElementState JustPressed()
    {
        DigitalElementState result;
        result.pressed = true;
        result.justChanged = true;

        return result;
    }

    /** Create instance of `DigitalElementState` in released state. */
    static DigitalElementState Released()
    {
        DigitalElementState result;

        return result;
    }

    /** Create instance of `DigitalElementState` in just released state. */
    static DigitalElementState JustReleased()
    {
        DigitalElementState result;
        result.justChanged = true;

        return result;
    }

    /** Modify the state as if the button it is associated with has been pressed. */
    void Press()
    {
        if (!pressed)
        {
            pressed = true;
            justChanged = true;
        }
        else
        {
            justChanged = false;
        }
    }

    /** Modify the state as if the button it is associated with has been released. */
    void Release()
    {
        if (pressed)
        {
            pressed = false;
            justChanged = true;
        }
        else
        {
            justChanged = false;
        }
    }

    bool operator==(const DigitalElementState& other) const
    {
        return (pressed == other.pressed) && (justChanged == other.justChanged);
    }

    bool operator!=(const DigitalElementState& other) const
    {
        return !(*this == other);
    }

private:
    bool pressed = false;
    bool justChanged = false;
};

/**
    Describes analog element state.

    Analog element is a part of an input device that requires multiple float values to describe its state.
    Examples are:
        - Mouse position
        - Gamepad stick
        - Gamepad gyroscope
        - Touch position

    Meanings of `x`, `y` and `z` values can be different for different elements. For example:
    - Gamepad stick defines x and y values in range of [-1; 1] for according axes
    - Mouse position defines x and y values as according coordinates in window space, if mouse capture is turned off
    - Mouse position defines x and y values as a relative offset from the previous position, if mouse capture is turned on
*/
struct AnalogElementState final
{
    AnalogElementState() = default;
    AnalogElementState(float32 x_, float32 y_, float32 z_)
        : x(x_)
        , y(y_)
        , z(z_)
    {
    }
    /** Analog X value */
    float32 x = 0.f;

    /** Analog Y value */
    float32 y = 0.f;

    /** Analog Z value */
    float32 z = 0.f;
};

/**
    \}
*/
}
