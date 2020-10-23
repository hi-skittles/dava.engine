#pragma once

#include "Base/BaseTypes.h"
#include "Base/Token.h"
#include "Input/InputDevice.h"

#include <bitset>

namespace DAVA
{
class InputSystem;

namespace Private
{
class GamepadImpl;
struct MainDispatcherEvent;
}

// TODO: Add unit tests for Gamepad class
// TODO: Add support for multiple gamepads in device manager

/**
    \ingroup input

    Class for working with gamepads.
*/
class Gamepad final : public InputDevice
{
    friend class DeviceManager; // For creation
    friend class Private::GamepadImpl;

public:
    /**
        Return state of a start button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_START)`.
     */
    DigitalElementState GetStartButtonState() const;

    /**
        Return state of an A button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_A)`.
     */
    DigitalElementState GetAButtonState() const;

    /**
        Return state of a B button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_B)`.
     */
    DigitalElementState GetBButtonState() const;

    /**
        Return state of a X button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_X)`.
     */
    DigitalElementState GetXButtonState() const;

    /**
        Return state of an Y button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_Y)`.
     */
    DigitalElementState GetYButtonState() const;

    /**
        Return state of a DPad left button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_DPAD_LEFT)`.
     */
    DigitalElementState GetLeftDPadButtonState() const;

    /**
        Return state of a DPad right button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_DPAD_RIGHT)`.
     */
    DigitalElementState GetRightDPadButtonState() const;

    /**
        Return state of a DPad up button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_DPAD_UP)`.
     */
    DigitalElementState GetUpDPadButtonState() const;

    /**
        Return state of a DPad down button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_DPAD_DOWN)`.
     */
    DigitalElementState GetDownDPadButtonState() const;

    /**
        Return state of a left thumb button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_LTHUMB)`.
     */
    DigitalElementState GetLeftThumbButtonState() const;

    /**
        Return state of a right thumb button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_RTHUMB)`.
     */
    DigitalElementState GetRightThumbButtonState() const;

    /**
        Return state of a left shoulder button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_LSHOULDER)`.
     */
    DigitalElementState GetLeftShoulderButtonState() const;

    /**
        Return state of a right shoulder button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::GAMEPAD_RSHOULDER)`.
     */
    DigitalElementState GetRightShoulderButtonState() const;

    /**
        Return left trigger position.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::GAMEPAD_AXIS_LTRIGGER)`.
     */
    AnalogElementState GetLeftTriggerAxis() const;

    /**
        Return right trigger position.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::GAMEPAD_AXIS_RTRIGGER)`.
     */
    AnalogElementState GetRightTriggerAxis() const;

    /**
        Return left thumb position.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::GAMEPAD_AXIS_LTHUMB)`.
     */
    AnalogElementState GetLeftThumbAxis() const;

    /**
        Return right thumb position.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::GAMEPAD_AXIS_RTHUMB)`.
     */
    AnalogElementState GetRightThumbAxis() const;

    // InputDevice overrides

    bool IsElementSupported(eInputElements elementId) const override;
    DigitalElementState GetDigitalElementState(eInputElements elementId) const override;
    AnalogElementState GetAnalogElementState(eInputElements elementId) const override;

private:
    explicit Gamepad(uint32 id);
    ~Gamepad();

    void Update();
    void OnEndFrame();

    bool HandleMainDispatcherEvent(const Private::MainDispatcherEvent& e);

    void HandleGamepadAdded(const Private::MainDispatcherEvent& e);
    void HandleGamepadRemoved(const Private::MainDispatcherEvent& e);

    void HandleGamepadMotion(const Private::MainDispatcherEvent& e);
    void HandleGamepadButton(const Private::MainDispatcherEvent& e);

    void HandleButtonPress(eInputElements element, bool pressed);
    void HandleBackButtonPress(bool pressed);
    void HandleAxisMovement(eInputElements element, float32 newValue, bool horizontal);

    void ResetState(Window* window) override;

    InputSystem* inputSystem = nullptr;
    std::unique_ptr<Private::GamepadImpl> impl;

    static const uint32 BUTTON_COUNT = static_cast<uint32>(eInputElements::GAMEPAD_LAST_BUTTON - eInputElements::GAMEPAD_FIRST_BUTTON + 1);
    static const uint32 AXIS_COUNT = static_cast<uint32>(eInputElements::GAMEPAD_LAST_AXIS - eInputElements::GAMEPAD_FIRST_AXIS + 1);
    Array<DigitalElementState, BUTTON_COUNT> buttons;
    Array<AnalogElementState, AXIS_COUNT> axes;
    DigitalElementState backButton;

    std::bitset<BUTTON_COUNT> buttonChangedMask;
    std::bitset<AXIS_COUNT> axisChangedMask;
    std::bitset<BUTTON_COUNT + AXIS_COUNT> supportedElements;

    Token endFrameConnectionToken;
};
} // namespace DAVA
