#include "Input/Private/Ios/GamepadImpl.Ios.h"
#include "Input/Gamepad.h"
#include "Input/InputElements.h"

#import <GameController/GameController.h>

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
    if ([controller extendedGamepad])
    {
        GCExtendedGamepad* gamepad = [controller extendedGamepad];
        ReadExtendedGamepadElements(gamepad);
    }
    else if ([controller gamepad])
    {
        GCGamepad* gamepad = [controller gamepad];
        ReadGamepadElements(gamepad);
    }
}

void GamepadImpl::ReadExtendedGamepadElements(GCExtendedGamepad* gamepad)
{
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_A, gamepad.buttonA.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_B, gamepad.buttonB.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_X, gamepad.buttonX.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_Y, gamepad.buttonY.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_LSHOULDER, gamepad.leftShoulder.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_RSHOULDER, gamepad.rightShoulder.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_LEFT, gamepad.dpad.left.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_RIGHT, gamepad.dpad.right.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_UP, gamepad.dpad.up.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_DOWN, gamepad.dpad.down.isPressed);

    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTHUMB, gamepad.leftThumbstick.xAxis.value, true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTHUMB, gamepad.leftThumbstick.yAxis.value, false);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTHUMB, gamepad.rightThumbstick.xAxis.value, true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTHUMB, gamepad.rightThumbstick.yAxis.value, false);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_LTRIGGER, gamepad.leftTrigger.value, true);
    gamepadDevice->HandleAxisMovement(eInputElements::GAMEPAD_AXIS_RTRIGGER, gamepad.rightTrigger.value, true);
}

void GamepadImpl::ReadGamepadElements(GCGamepad* gamepad)
{
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_A, gamepad.buttonA.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_B, gamepad.buttonB.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_X, gamepad.buttonX.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_Y, gamepad.buttonY.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_LSHOULDER, gamepad.leftShoulder.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_RSHOULDER, gamepad.rightShoulder.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_LEFT, gamepad.dpad.left.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_RIGHT, gamepad.dpad.right.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_UP, gamepad.dpad.up.isPressed);
    gamepadDevice->HandleButtonPress(eInputElements::GAMEPAD_DPAD_DOWN, gamepad.dpad.down.isPressed);
}

bool GamepadImpl::HandleGamepadAdded(uint32 /*id*/)
{
    if (controller == nullptr)
    {
        NSArray<GCController*>* controllers = [GCController controllers];
        if ([controllers count] != 0)
        {
            controller = (__bridge GCController*)CFBridgingRetain([controllers objectAtIndex:0]);
            DetermineSupportedElements();
        }
    }
    return controller != nullptr;
}

bool GamepadImpl::HandleGamepadRemoved(uint32 id)
{
    bool removed = true;
    for (GCController* c in [GCController controllers])
    {
        if (c == controller)
        {
            removed = false;
            break;
        }
    }
    if (removed && controller != nullptr)
    {
        CFBridgingRelease(controller);
        controller = nullptr;
    }
    return controller != nullptr;
}

void GamepadImpl::DetermineSupportedElements()
{
    if ([controller extendedGamepad])
    {
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_START - eInputElements::GAMEPAD_FIRST, false);
        /*TODO gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_BACK - eInputElements::GAMEPAD_FIRST, false);*/
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_A - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_B - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_X - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_Y - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_LEFT - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_RIGHT - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_UP - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_DOWN - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_LTHUMB - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_RTHUMB - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_LSHOULDER - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_RSHOULDER - eInputElements::GAMEPAD_FIRST, true);

        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_LTRIGGER - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_RTRIGGER - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_LTHUMB - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_RTHUMB - eInputElements::GAMEPAD_FIRST, true);
    }
    else if ([controller gamepad])
    {
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_START - eInputElements::GAMEPAD_FIRST, false);
        /*TODO gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_BACK - eInputElements::GAMEPAD_FIRST, false);*/
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_A - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_B - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_X - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_Y - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_LEFT - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_RIGHT - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_UP - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_DPAD_DOWN - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_LTHUMB - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_RTHUMB - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_LSHOULDER - eInputElements::GAMEPAD_FIRST, true);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_RSHOULDER - eInputElements::GAMEPAD_FIRST, true);

        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_LTRIGGER - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_RTRIGGER - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_LTHUMB - eInputElements::GAMEPAD_FIRST, false);
        gamepadDevice->supportedElements.set(eInputElements::GAMEPAD_AXIS_RTHUMB - eInputElements::GAMEPAD_FIRST, false);
    }
}

} // namespace Private
} // namespace DAVA
