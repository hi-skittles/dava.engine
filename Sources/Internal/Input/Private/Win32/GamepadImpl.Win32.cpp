#include "Input/Private/Win32/GamepadImpl.Win32.h"

#if defined(__DAVAENGINE_WIN32__)

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

#endif // __DAVAENGINE_WIN32__
