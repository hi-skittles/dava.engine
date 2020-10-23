#pragma once

#include "Input/InputSystemTypes.h"
#include "Input/InputElements.h"
#include "Math/Math2D.h"

namespace DAVA
{
// TODO: add support for virtual (i.e. user defined) devices
// TODO: think about adding functions to engine context to get specific input devices: e.g. `DAVA::Context::GetKeyboard()` instead of `GetEngineContext()->deviceManager->GetKeyboard()`

/**
    \ingroup input

    Represents a virtual or a real device used for input.
    This class is responsible for:
        - Storing device's state for others to request it when needed
        - Handling native input events, transforming it to an `InputEvent` and sending it to the `InputSystem`

    Device instances can be obtained using `DeviceManager` methods: `GetKeyboard`, `GetMouse`, `GetTouchScreen`, and `GetGamepad`.

    Derived classes are welcome to introduce helper method for easier and more readable access to its elements,
    e.g. a mouse can provide GetPosition() method which can be a wrapper around GetAnalogElementState(eInputElements::MOUSE_POSITION), etc.
*/

// Forward declaration
class Window;

class InputDevice
{
public:
    /** Create InputDevice instance with specified `id` */
    explicit InputDevice(uint32 id);

    virtual ~InputDevice();

    /** Return unique device id */
    uint32 GetId() const;

    /** Return `true` if an element with specified `elementId` is supported by the device
        (i.e. its state can be requested with either `GetDigitalElementState` or `GetAnalogElementState`).
    */
    virtual bool IsElementSupported(eInputElements elementId) const = 0;

    /**
        Get state of a digital element with specified `elementId`.

        \pre Device should support specified digital element.
    */
    virtual DigitalElementState GetDigitalElementState(eInputElements elementId) const = 0;

    /**
        Get state of an analog element with specified `elementId`.

        \pre Device should support specified analog element.
    */
    virtual AnalogElementState GetAnalogElementState(eInputElements elementId) const = 0;

protected:
    virtual void ResetState(Window* window) = 0;

private:
    void OnWindowCreated(Window* window);
    void OnWindowDestroyed(Window* window);
    void OnWindowFocusChanged(Window* window, bool focused);
    void OnWindowSizeChanged(Window* window, Size2f, Size2f);

private:
    const uint32 id;
};
} // namespace DAVA
