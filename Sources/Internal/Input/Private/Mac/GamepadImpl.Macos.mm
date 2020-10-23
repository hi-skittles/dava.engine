#include "Input/Private/Mac/GamepadImpl.Macos.h"

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

bool GamepadImpl::HandleGamepadAdded(uint32 /*id*/)
{
    return false;
}

bool GamepadImpl::HandleGamepadRemoved(uint32 /*id*/)
{
    return false;
}

} // namespace Private
} // namespace DAVA
