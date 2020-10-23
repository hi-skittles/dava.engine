#pragma once

#include "Engine/EngineTypes.h"
#include "Input/InputDevice.h"

namespace DAVA
{
class Window;

/**
    \ingroup input

    Represents input event triggered by some device.
*/
struct InputEvent
{
    InputEvent();
    ~InputEvent();

    /** The window this event is addressed to. */
    Window* window;

    /** Event timestamp. */
    float64 timestamp;

    /** Type of the device that triggered the event. */
    eInputDeviceTypes deviceType;

    /** Device that triggered the event. This value is never null. */
    InputDevice* device;

    /** Id of the element which triggered the event. */
    eInputElements elementId;

    /** Digital element's state. Should only be used if element with `elementId` is a digital one. */
    DigitalElementState digitalState;

    /** Analog element's state. Should only be used if element with `elementId` is an analog one. */
    AnalogElementState analogState;

    // Additional fields for different devices

    struct MouseEvent
    {
        /** Specifies if mouse position coordinates, stored in `digitalState`, are relative to the previous position or absolute (depends on mouse capture mode) */
        bool isRelative;
    };

    struct KeyboardEvent
    {
        /** Code for char events. Equal to 0 if it's not a char event  */
        char32_t charCode;

        /** True if char event is repeated */
        bool charRepeated;
    };

    union
    {
        MouseEvent mouseEvent;
        KeyboardEvent keyboardEvent;
    };
};

inline InputEvent::InputEvent()
    : window(nullptr)
    , timestamp(0.0)
    , deviceType(eInputDeviceTypes::UNKNOWN)
    , device(nullptr)
    , elementId(NONE)
{
    keyboardEvent = KeyboardEvent();
}

inline InputEvent::~InputEvent()
{
    // do nothing
}
}