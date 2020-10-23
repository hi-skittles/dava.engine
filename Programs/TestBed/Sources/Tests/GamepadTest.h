#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
namespace DAVA
{
struct InputEvent;
}

class GamepadTest : public BaseScreen
{
public:
    GamepadTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnResetClick(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnLogClick(DAVA::BaseObject* sender, void* data, void* callerData);

    void ResetCounters();

    bool InputEventLogHandler(const DAVA::InputEvent& inputEvent);
    bool InputEventHandler(const DAVA::InputEvent& inputEvent);
    bool OnGamepadEvent(const DAVA::InputEvent& e);

    void OnGestureEvent(DAVA::UIEvent* event);

    void UpdateGamepadElement(DAVA::String name, bool isVisible);
    void UpdateGamepadStickX(DAVA::String name, float axisValue);
    void UpdateGamepadStickY(DAVA::String name, float axisValue);

    TestBed& app;
    DAVA::UIControl* gamepad = nullptr;
    DAVA::uint32 gamepadInputToken = 0;
};
