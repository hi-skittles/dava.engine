#include "Infrastructure/TestBed.h"
#include "Tests/GamepadTest.h"

#include <DeviceManager/DeviceManager.h>
#include <Engine/Engine.h>
#include <Input/Gamepad.h>
#include <Input/Mouse.h>
#include <UI/Focus/UIFocusComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <Render/2D/Sprite.h>

using namespace DAVA;

struct Finger
{
    int32 index = 0;
    UIControl* img = nullptr;
    bool isActive = false;
};

Array<Finger, 10> touches;
Vector2 hiddenPos(-100, -100);

auto gamepadButtonsNames =
{ "button_a", "button_b", "button_x", "button_y", "button_left", "button_right",
  "button_up", "button_down", "button_select", "button_start",
  "shift_left", "shift_right", "triger_left", "triger_right",
  "stick_left", "stick_right" };

Map<String, UIControl*> gamepadButtons;

Rect gamepadPos(20, 20, 800, 450);
float32 gamepadStickDeltaMove = 20.f; // 20 pixels

GamepadTest::GamepadTest(TestBed& app)
    : BaseScreen(app, "GamepadTest")
    , app(app)
{
}

void GamepadTest::LoadResources()
{
    BaseScreen::LoadResources();

    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    InputSystem* inputSystem = app.GetEngine().GetContext()->inputSystem;
    gamepadInputToken = inputSystem->AddHandler(eInputDeviceTypes::CLASS_GAMEPAD, MakeFunction(this, &GamepadTest::OnGamepadEvent));

    gamepad = new UIControl(gamepadPos);
    FilePath pathToBack("~res:/TestBed/TestData/GamepadTest/gamepad.png");
    ScopedPtr<Sprite> gamepadSprite(Sprite::CreateFromSourceFile(pathToBack));
    UIControlBackground* bg = gamepad->GetOrCreateComponent<UIControlBackground>();
    bg->SetModification(ESM_VFLIP | ESM_HFLIP);
    bg->SetSprite(gamepadSprite, 0);
    AddControl(gamepad);

    for (auto& buttonOrAxisName : gamepadButtonsNames)
    {
        UIControl* img = new UIControl(gamepadPos);
        auto path = FilePath("~res:/TestBed/TestData/GamepadTest/") + buttonOrAxisName + ".png";
        UIControlBackground* bg = img->GetOrCreateComponent<UIControlBackground>();
        bg->SetModification(ESM_VFLIP | ESM_HFLIP);

        ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile(path));
        bg->SetSprite(sprite, 0);
        gamepadButtons[buttonOrAxisName] = img;
        AddControl(img);
        img->SetVisibilityFlag(false);
    }
}

void GamepadTest::UnloadResources()
{
    InputSystem* inputSystem = app.GetEngine().GetContext()->inputSystem;
    inputSystem->RemoveHandler(gamepadInputToken);

    SafeRelease(gamepad);

    for (auto& gamepadButton : gamepadButtons)
    {
        SafeRelease(gamepadButton.second);
    }

    BaseScreen::UnloadResources();
}

bool GamepadTest::OnGamepadEvent(const DAVA::InputEvent& e)
{
    //Logger::Info("gamepad tid: %2d, x: %.3f, y:%.3f", event->tid, event->point.x, event->point.y);

    DVASSERT(e.deviceType == eInputDevices::GAMEPAD);

    switch (e.elementId)
    {
    case eInputElements::GAMEPAD_START:
        UpdateGamepadElement("button_start", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_A:
        UpdateGamepadElement("button_a", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_B:
        UpdateGamepadElement("button_b", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_X:
        UpdateGamepadElement("button_x", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_Y:
        UpdateGamepadElement("button_y", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_LSHOULDER:
        UpdateGamepadElement("shift_left", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_RSHOULDER:
        UpdateGamepadElement("shift_right", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_LTHUMB:
        UpdateGamepadElement("stick_left", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_RTHUMB:
        UpdateGamepadElement("stick_right", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_AXIS_LTHUMB:
        UpdateGamepadStickX("stick_left", e.analogState.x);
        UpdateGamepadStickY("stick_left", e.analogState.y);
        break;
    case eInputElements::GAMEPAD_AXIS_RTHUMB:
        UpdateGamepadStickX("stick_right", e.analogState.x);
        UpdateGamepadStickY("stick_right", e.analogState.y);
        break;
    case eInputElements::GAMEPAD_AXIS_LTRIGGER:
        UpdateGamepadStickX("triger_left", e.analogState.x);
        UpdateGamepadStickY("triger_left", e.analogState.y);
        break;
    case eInputElements::GAMEPAD_AXIS_RTRIGGER:
        UpdateGamepadStickX("triger_right", e.analogState.x);
        UpdateGamepadStickY("triger_right", e.analogState.y);
        break;
    case eInputElements::GAMEPAD_DPAD_LEFT:
        UpdateGamepadElement("button_left", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_DPAD_RIGHT:
        UpdateGamepadElement("button_right", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_DPAD_UP:
        UpdateGamepadElement("button_up", e.digitalState.IsPressed());
        break;
    case eInputElements::GAMEPAD_DPAD_DOWN:
        UpdateGamepadElement("button_down", e.digitalState.IsPressed());
        break;
    default:
        Logger::Error("not handled gamepad input event element: %d", static_cast<uint32>(e.elementId));
    }
    return false;
}

void GamepadTest::UpdateGamepadElement(String name, bool isVisible)
{
    gamepadButtons[name]->SetVisibilityFlag(isVisible);
}

void GamepadTest::UpdateGamepadStickX(String name, float axisValue)
{
    UIControl* ctrl = gamepadButtons[name];
    Vector2 pos = ctrl->GetPosition();
    if (std::abs(axisValue) >= 0.05f)
    {
        pos.x = gamepadPos.GetPosition().x + (axisValue * gamepadStickDeltaMove);
    }
    else
    {
        pos.x = gamepadPos.GetPosition().x;
    }
    ctrl->SetPosition(pos);
    UpdateGamepadElement(name, pos != gamepadPos.GetPosition());
}

void GamepadTest::UpdateGamepadStickY(String name, float axisValue)
{
    UIControl* ctrl = gamepadButtons[name];
    Vector2 pos = ctrl->GetPosition();
    if (std::abs(axisValue) >= 0.05f)
    {
        pos.y = gamepadPos.GetPosition().y + (axisValue * gamepadStickDeltaMove * -1); // -1 y axis from up to down
    }
    else
    {
        pos.y = gamepadPos.GetPosition().y;
    }
    ctrl->SetPosition(pos);
    UpdateGamepadElement(name, pos != gamepadPos.GetPosition());
}
