#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"

#include "Input/Private/Android/GamepadImpl.Android.h"
#include "Input/Gamepad.h"
#include "Input/InputElements.h"

#include <android/input.h>
#include <android/keycodes.h>

namespace DAVA
{
namespace Private
{
GamepadImpl::GamepadImpl(Gamepad* gamepadDevice)
    : gamepadDevice(gamepadDevice)
{
}

void GamepadImpl::Update()
{
}

void GamepadImpl::HandleGamepadMotion(const MainDispatcherEvent& e)
{
    uint32 axis = e.gamepadEvent.axis;
    float32 value = e.gamepadEvent.value;

    if (axis == AMOTION_EVENT_AXIS_HAT_X || axis == AMOTION_EVENT_AXIS_HAT_Y)
    {
        HandleAxisHat(axis, value);
        return;
    }

    // On game pads with two analog joysticks, axis AXIS_RX is often reinterpreted as absolute X position and
    // axis AXIS_RZ is reinterpreted as absolute Y position of the second joystick instead.
    eInputElements element = eInputElements::NONE;
    bool verticalAxis = false;
    switch (axis)
    {
    case AMOTION_EVENT_AXIS_X:
        element = eInputElements::GAMEPAD_AXIS_LTHUMB;
        break;
    case AMOTION_EVENT_AXIS_Y:
        verticalAxis = true;
        element = eInputElements::GAMEPAD_AXIS_LTHUMB;
        break;
    case AMOTION_EVENT_AXIS_Z:
    case AMOTION_EVENT_AXIS_RX:
        element = eInputElements::GAMEPAD_AXIS_RTHUMB;
        break;
    case AMOTION_EVENT_AXIS_RY:
    case AMOTION_EVENT_AXIS_RZ:
        verticalAxis = true;
        element = eInputElements::GAMEPAD_AXIS_RTHUMB;
        break;
    case AMOTION_EVENT_AXIS_LTRIGGER:
    case AMOTION_EVENT_AXIS_BRAKE:
        element = eInputElements::GAMEPAD_AXIS_LTRIGGER;
        break;
    case AMOTION_EVENT_AXIS_RTRIGGER:
    case AMOTION_EVENT_AXIS_GAS:
        element = eInputElements::GAMEPAD_AXIS_RTRIGGER;
        break;
    default:
        return;
    }

    // Android joystick Y-axis position is normalized to a range [-1, 1] where -1 for up or far and 1 for down or near.
    // Historically dava.engine's clients expect Y-axis value -1 for down or near and 1 for up and far so negate Y-axes.
    switch (axis)
    {
    case AMOTION_EVENT_AXIS_Y:
    case AMOTION_EVENT_AXIS_RY:
    case AMOTION_EVENT_AXIS_RZ:
        value = -value;
        break;
    default:
        break;
    }

    gamepadDevice->HandleAxisMovement(element, value, !verticalAxis);
}

void GamepadImpl::HandleGamepadButton(const MainDispatcherEvent& e)
{
    bool pressed = e.type == MainDispatcherEvent::GAMEPAD_BUTTON_DOWN;

    if (e.gamepadEvent.button == AKEYCODE_BACK)
    {
        gamepadDevice->HandleBackButtonPress(pressed);
        return;
    }

    eInputElements element = eInputElements::NONE;
    switch (e.gamepadEvent.button)
    {
    case AKEYCODE_DPAD_UP:
        element = eInputElements::GAMEPAD_DPAD_UP;
        break;
    case AKEYCODE_DPAD_DOWN:
        element = eInputElements::GAMEPAD_DPAD_DOWN;
        break;
    case AKEYCODE_DPAD_LEFT:
        element = eInputElements::GAMEPAD_DPAD_LEFT;
        break;
    case AKEYCODE_DPAD_RIGHT:
        element = eInputElements::GAMEPAD_DPAD_RIGHT;
        break;
    case AKEYCODE_BUTTON_A:
        element = eInputElements::GAMEPAD_A;
        break;
    case AKEYCODE_BUTTON_B:
        element = eInputElements::GAMEPAD_B;
        break;
    case AKEYCODE_BUTTON_X:
        element = eInputElements::GAMEPAD_X;
        break;
    case AKEYCODE_BUTTON_Y:
        element = eInputElements::GAMEPAD_Y;
        break;
    case AKEYCODE_BUTTON_L1:
    case AKEYCODE_BUTTON_L2:
        element = eInputElements::GAMEPAD_LSHOULDER;
        break;
    case AKEYCODE_BUTTON_R1:
    case AKEYCODE_BUTTON_R2:
        element = eInputElements::GAMEPAD_RSHOULDER;
        break;
    case AKEYCODE_BUTTON_THUMBL:
        element = eInputElements::GAMEPAD_LTHUMB;
        break;
    case AKEYCODE_BUTTON_THUMBR:
        element = eInputElements::GAMEPAD_RTHUMB;
        break;
    case AKEYCODE_BUTTON_START:
        element = eInputElements::GAMEPAD_START;
        break;
    default:
        return;
    }

    gamepadDevice->HandleButtonPress(element, pressed);
}

void GamepadImpl::HandleAxisHat(int axis, float value)
{
    // Some controllers report D-pad presses as axis movement AXIS_HAT_X and AXIS_HAT_Y

    static const eInputElements elemX[] = { eInputElements::GAMEPAD_DPAD_LEFT, eInputElements::GAMEPAD_DPAD_RIGHT };
    static const eInputElements elemY[] = { eInputElements::GAMEPAD_DPAD_UP, eInputElements::GAMEPAD_DPAD_DOWN };

    bool pressed = value != 0.f;
    const eInputElements* elem = axis == AMOTION_EVENT_AXIS_HAT_X ? elemX : elemY;
    if (pressed)
    {
        uint32 index = elem[value > 0.f] - eInputElements::GAMEPAD_FIRST_BUTTON;
        DigitalElementState& dpadElem(gamepadDevice->buttons[index]);

        dpadElem.Press();
        gamepadDevice->buttonChangedMask.set(index);
    }
    else
    {
        for (uint32 i = 0; i < 2; ++i)
        {
            uint32 index = elem[i] - eInputElements::GAMEPAD_FIRST_BUTTON;
            DigitalElementState& dpadElem(gamepadDevice->buttons[index]);
            if (dpadElem.IsPressed())
            {
                dpadElem.Release();
                gamepadDevice->buttonChangedMask.set(index);
                break;
            }
        }
    }
}

bool GamepadImpl::HandleGamepadAdded(uint32 id)
{
    if (gamepadId == 0)
    {
        gamepadId = id;
        DetermineSupportedElements();
    }
    return gamepadId != 0;
}

bool GamepadImpl::HandleGamepadRemoved(uint32 id)
{
    if (gamepadId == id)
    {
        gamepadId = 0;
    }
    return gamepadId != 0;
}

void GamepadImpl::DetermineSupportedElements()
{
    // Assume that all elements supported
    gamepadDevice->supportedElements.set();
}

} // namespace Private
} // namespace DAVA
