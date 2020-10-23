#pragma once

#include "Input/InputDevice.h"
#include "Engine/EngineTypes.h"

namespace DAVA
{
class InputSystem;
class Window;

namespace Private
{
struct MainDispatcherEvent;
}

/**
    \ingroup input
    
    Represents mouse input device.
*/
class Mouse final : public InputDevice
{
    friend class DeviceManager; // For creation

public:
    /**
        Return state of a left button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::MOUSE_LBUTTON)`.
    */
    DigitalElementState GetLeftButtonState() const;

    /**
        Return state of a right button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::MOUSE_RBUTTON)`.
    */
    DigitalElementState GetRightButtonState() const;

    /**
        Return state of a middle button.

        \note It's essentialy a shorter and more readable placeholder for `GetDigitalElementState(eInputElements::MOUSE_MBUTTON)`.
    */
    DigitalElementState GetMiddleButtonState() const;

    /**
        Return mouse position.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::MOUSE_POSITION)`.
    */
    AnalogElementState GetPosition() const;

    /**
        Return mouse wheel delta.

        \note It's essentialy a shorter and more readable placeholder for `GetAnalogElementState(eInputElements::MOUSE_WHEEL)`.
    */
    AnalogElementState GetWheelDelta() const;

    /**
        Return first button which is pressed right now, in this order: left, right, middle, ext1, ext2.
        If all buttons are released, eInputElements::NONE will be returned.
    */
    eInputElements GetFirstPressedButton() const;

    // InputDevice overrides

    bool IsElementSupported(eInputElements elementId) const override;
    DigitalElementState GetDigitalElementState(eInputElements elementId) const override;
    AnalogElementState GetAnalogElementState(eInputElements elementId) const override;

private:
    explicit Mouse(uint32 id);
    ~Mouse() override;

    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

    void OnEndFrame();
    void OnCursorCaptureChanged(Window*, eCursorCapture mode);

    bool HandleEvent(const Private::MainDispatcherEvent& e);

    void ResetState(Window* window) override;
    void HandleMouseClick(const Private::MainDispatcherEvent& e);
    void HandleMouseWheel(const Private::MainDispatcherEvent& e);
    void HandleMouseMove(const Private::MainDispatcherEvent& e);

    void CreateAndSendButtonInputEvent(eInputElements elementId, DigitalElementState state, Window* window, int64 timestamp, bool isRelative);

private:
    InputSystem* inputSystem = nullptr;

    Array<DigitalElementState, INPUT_ELEMENTS_MOUSE_BUTTON_COUNT> buttons;
    AnalogElementState mousePosition;
    AnalogElementState mouseWheelDelta;
    bool isRelative = false;
};
} // namespace DAVA
