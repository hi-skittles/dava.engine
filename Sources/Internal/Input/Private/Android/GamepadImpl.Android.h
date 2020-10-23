#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class Gamepad;
namespace Private
{
struct MainDispatcherEvent;
class GamepadImpl final
{
public:
    GamepadImpl(Gamepad* gamepadDevice);

    void Update();

    void HandleGamepadMotion(const MainDispatcherEvent& e);
    void HandleGamepadButton(const MainDispatcherEvent& e);
    void HandleAxisHat(int axis, float value);

    bool HandleGamepadAdded(uint32 id);
    bool HandleGamepadRemoved(uint32 id);

    void DetermineSupportedElements();

    Gamepad* gamepadDevice = nullptr;
    uint32 gamepadId = 0;
};

} // namespace Private
} // namespace DAVA
