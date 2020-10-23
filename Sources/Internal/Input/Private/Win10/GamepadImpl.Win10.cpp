#include "Input/Private/Win10/GamepadImpl.Win10.h"
#include "Input/Gamepad.h"

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
    using ::Windows::Gaming::Input::GamepadReading;
    using ::Windows::Gaming::Input::GamepadButtons;

    GamepadReading reading = gamepad->GetCurrentReading();

    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_START, (reading.Buttons & GamepadButtons::Menu) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_A, (reading.Buttons & GamepadButtons::A) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_B, (reading.Buttons & GamepadButtons::B) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_X, (reading.Buttons & GamepadButtons::X) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_Y, (reading.Buttons & GamepadButtons::Y) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_LEFT, (reading.Buttons & GamepadButtons::DPadLeft) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_RIGHT, (reading.Buttons & GamepadButtons::DPadRight) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_UP, (reading.Buttons & GamepadButtons::DPadUp) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_DOWN, (reading.Buttons & GamepadButtons::DPadDown) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_LTHUMB, (reading.Buttons & GamepadButtons::LeftThumbstick) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_RTHUMB, (reading.Buttons & GamepadButtons::RightThumbstick) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_LSHOULDER, (reading.Buttons & GamepadButtons::LeftShoulder) != GamepadButtons::None);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_RSHOULDER, (reading.Buttons & GamepadButtons::RightShoulder) != GamepadButtons::None);

    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTHUMB, static_cast<float32>(reading.LeftThumbstickX), true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTHUMB, static_cast<float32>(reading.LeftThumbstickY), false);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTHUMB, static_cast<float32>(reading.RightThumbstickX), true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTHUMB, static_cast<float32>(reading.RightThumbstickY), false);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTRIGGER, static_cast<float32>(reading.LeftTrigger), true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTRIGGER, static_cast<float32>(reading.RightTrigger), true);

    gamepadDevice->HandleBackButtonPress((reading.Buttons & GamepadButtons::View) != GamepadButtons::None);
}

bool GamepadImpl::HandleGamepadAdded(uint32 /*id*/)
{
    // Only one connected gamepad is supported by dava.engine for now.
    // So choose first gamepad from gamepad list.

    using ::Windows::Foundation::Collections::IVectorView;
    using ::Windows::Gaming::Input::Gamepad;

    if (gamepad == nullptr)
    {
        IVectorView<Gamepad ^> ^ gamepads = Gamepad::Gamepads;
        if (gamepads->Size != 0)
        {
            gamepad = gamepads->GetAt(0);
            DetermineSupportedElements();
        }
    }
    return gamepad != nullptr;
}

bool GamepadImpl::HandleGamepadRemoved(uint32 /*id*/)
{
    // Only one connected gamepad is supported by dava.engine for now.
    // So check whether currently using gamepad is removed

    using ::Windows::Foundation::Collections::IVectorView;
    using ::Windows::Gaming::Input::Gamepad;

    bool removed = true;
    IVectorView<Gamepad ^> ^ gamepads = Gamepad::Gamepads;
    for (unsigned int i = 0, n = gamepads->Size; i < n; ++i)
    {
        if (gamepads->GetAt(i) == gamepad)
        {
            removed = false;
            break;
        }
    }
    if (removed)
    {
        gamepad = nullptr;
    }
    return gamepad != nullptr;
}

void GamepadImpl::DetermineSupportedElements()
{
    // Assume that all elements supported
    gamepadDevice->supportedElements.set();
}

} // namespace Private
} // namespace DAVA
