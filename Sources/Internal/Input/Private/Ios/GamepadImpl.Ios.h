#pragma once

#include "Base/BaseTypes.h"

DAVA_FORWARD_DECLARE_OBJC_CLASS(GCController);
DAVA_FORWARD_DECLARE_OBJC_CLASS(GCExtendedGamepad);
DAVA_FORWARD_DECLARE_OBJC_CLASS(GCGamepad);

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
    void ReadExtendedGamepadElements(GCExtendedGamepad* gamepad);
    void ReadGamepadElements(GCGamepad* gamepad);

    void HandleGamepadMotion(const MainDispatcherEvent&)
    {
    }
    void HandleGamepadButton(const MainDispatcherEvent&)
    {
    }

    bool HandleGamepadAdded(uint32 id);
    bool HandleGamepadRemoved(uint32 id);

    void DetermineSupportedElements();

    Gamepad* gamepadDevice = nullptr;
    GCController* controller = nullptr;
};

} // namespace Private
} // namespace DAVA
