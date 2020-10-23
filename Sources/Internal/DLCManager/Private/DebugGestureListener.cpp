#include "DLCManager/Private/DebugGestureListener.h"
#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Input/Mouse.h"
#include "DeviceManager/DeviceManager.h"
#include "Logger/Logger.h"

namespace DAVA
{
DebugGestureListener::DebugGestureListener()
    : history(8)
{
    if (GetEngineContext()->inputSystem != nullptr)
    {
        AddListenerOnMouseAndTouch();
    }
    else
    {
        Engine::Instance()->gameLoopStarted.Connect(this, &DebugGestureListener::OnGameLoopStarted);
    }
}

DebugGestureListener::~DebugGestureListener()
{
    Engine::Instance()->gameLoopStarted.Disconnect(this);

    InputSystem* inputSys = GetEngineContext()->inputSystem;
    if (inputSys)
    {
        inputSys->RemoveHandler(handlerToken);
    }
    handlerToken = 0;
}

void DebugGestureListener::AddListenerOnMouseAndTouch()
{
    InputSystem* inputSystem = GetEngineContext()->inputSystem;
    if (inputSystem)
    {
        eInputDevices deviceMask = eInputDeviceTypes::MOUSE | eInputDeviceTypes::TOUCH_SURFACE;
        handlerToken = inputSystem->AddHandler(deviceMask, Function<bool(const InputEvent&)>(this, &DebugGestureListener::OnMouseOrTouch));
    }
    else
    {
        Logger::Error("can't add handler for debug gesture listener");
    }
}

void DebugGestureListener::OnGameLoopStarted()
{
    AddListenerOnMouseAndTouch();
}

bool DebugGestureListener::OnMouseOrTouch(const InputEvent& ev)
{
    if (ev.deviceType == eInputDeviceTypes::MOUSE
        && ev.elementId == MOUSE_LBUTTON
        && ev.digitalState.IsJustReleased())
    {
        InputEvent& inEvent = history.next();
        inEvent = ev;
        DeviceManager* devManager = GetEngineContext()->deviceManager;
        Mouse* mouse = devManager->GetMouse();
        inEvent.analogState = mouse->GetPosition();
    }
    else if (ev.deviceType == eInputDeviceTypes::TOUCH_SURFACE
             && ev.elementId == TOUCH_FIRST
             && ev.digitalState.IsJustReleased())
    {
        InputEvent& inEvent = history.next();
        inEvent = ev;
        DeviceManager* devManager = GetEngineContext()->deviceManager;
        TouchScreen* touchScreen = devManager->GetTouchScreen();
        inEvent.analogState = touchScreen->GetTouchPositionByIndex(0);
    }
    else
    {
        return false;
    }

    //this is a screen of your android device, tap inside squares in correct order
    // 0, 0 +-----------------------------------------------------------> 1024
    // + +-------------------------------------------------------------+
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | |        1          |          3          |          5        |
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | +-------------------------------------------------------------+
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | |        4          |          2          |          6        |
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // | |                   |                     |                   |
    // + +-------------------+---------------------+-------------------+
    // V
    // 768

    Size2f windowSize = GetPrimaryWindow()->GetSize();

    const float32 row = 2.f;
    const float32 column = 3.f;

    const Vector2 cellSize(windowSize.dx / column, windowSize.dy / row);
    const Vector2 cellCenter(cellSize * 0.5f);

    const float radius = cellCenter.Length();

    // top row
    const Vector2 first(cellSize * Vector2(0, 0) + cellCenter);
    const Vector2 third(cellSize * Vector2(1, 0) + cellCenter);
    const Vector2 five_(cellSize * Vector2(2, 0) + cellCenter);

    // bottom row
    const Vector2 four_(cellSize * Vector2(0, 1) + cellCenter);
    const Vector2 two__(cellSize * Vector2(1, 1) + cellCenter);
    const Vector2 six__(cellSize * Vector2(2, 1) + cellCenter);

    const std::array<Vector2, 6> positions{ first, two__, third, four_, five_, six__ };

    // check last 6 events in history match
    bool match = true;
    for (uint32 i = 0; i < 6; ++i)
    {
        auto it = history.rbegin() + i;
        Vector2 historyPos(it->analogState.x, it->analogState.y);

        auto secIt = positions.rbegin() + i;
        Vector2 needPos(*secIt);

        Vector2 delta = needPos - historyPos;
        if (delta.Length() > radius)
        {
            match = false;
            break;
        }
    }

    if (match)
    {
        debugGestureMatch.Emit();
    }

    return match; // let other input listeners handle this event if not match
}

} // end namespace DAVA