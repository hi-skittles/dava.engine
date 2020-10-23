#include "Input/TouchScreen.h"

#include "Engine/Engine.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"
#include "Input/InputSystem.h"
#include "Time/SystemTimer.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
TouchScreen::TouchScreen(uint32 id)
    : InputDevice(id)
    , inputSystem(GetEngineContext()->inputSystem)
    , clicks{}
    , positions{}
    , nativeTouchIds{}
{
    Engine* engine = Engine::Instance();

    engine->endFrame.Connect(this, &TouchScreen::OnEndFrame);

    Private::EngineBackend::Instance()->InstallEventFilter(this, MakeFunction(this, &TouchScreen::HandleMainDispatcherEvent));
}

TouchScreen::~TouchScreen()
{
    Engine* engine = Engine::Instance();

    engine->endFrame.Disconnect(this);

    Private::EngineBackend::Instance()->UninstallEventFilter(this);
}

DigitalElementState TouchScreen::GetTouchStateByIndex(size_t i) const
{
    return GetDigitalElementState(static_cast<eInputElements>(eInputElements::TOUCH_FIRST_CLICK + i));
}

AnalogElementState TouchScreen::GetTouchPositionByIndex(size_t i) const
{
    return GetAnalogElementState(static_cast<eInputElements>(eInputElements::TOUCH_FIRST_POSITION + i));
}

bool TouchScreen::IsElementSupported(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    return IsTouchInputElement(elementId);
}

DigitalElementState TouchScreen::GetDigitalElementState(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(IsTouchClickInputElement(elementId) && IsElementSupported(elementId));
    return clicks[elementId - eInputElements::TOUCH_FIRST_CLICK];
}

AnalogElementState TouchScreen::GetAnalogElementState(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(IsTouchPositionInputElement(elementId) && IsElementSupported(elementId));
    return positions[elementId - eInputElements::TOUCH_FIRST_POSITION];
}

void TouchScreen::OnEndFrame()
{
    // Promote JustPressed & JustReleased states to Pressed/Released accordingly
    for (DigitalElementState& touchState : clicks)
    {
        touchState.OnEndFrame();
    }
}

void TouchScreen::ResetState(Window* window)
{
    int64 timestamp = SystemTimer::GetMs();
    for (size_t i = 0; i < INPUT_ELEMENTS_TOUCH_CLICK_COUNT; ++i)
    {
        DigitalElementState& touchState = clicks[i];
        if (touchState.IsPressed())
        {
            eInputElements elementId = static_cast<eInputElements>(eInputElements::TOUCH_FIRST_CLICK + i);

            touchState.Release();
            nativeTouchIds[i] = 0;

            CreateAndSendTouchClickEvent(elementId, touchState, window, timestamp);

            AnalogElementState& analogState = positions[i];
            analogState.x = 0.0f;
            analogState.y = 0.0f;
        }
    }
}

bool TouchScreen::HandleMainDispatcherEvent(const Private::MainDispatcherEvent& e)
{
    using Private::MainDispatcherEvent;

    if (e.type == MainDispatcherEvent::TOUCH_DOWN)
    {
        return HandleTouchDownEvent(e);
    }
    else if (e.type == MainDispatcherEvent::TOUCH_UP)
    {
        return HandleTouchUpEvent(e);
    }
    else if (e.type == MainDispatcherEvent::TOUCH_MOVE)
    {
        return HandleTouchMoveEvent(e);
    }
    else if (e.type == MainDispatcherEvent::WINDOW_CANCEL_INPUT)
    {
        ResetState(e.window);

        // To send it further
        return false;
    }

    return false;
}

bool TouchScreen::HandleTouchDownEvent(const Private::MainDispatcherEvent& e)
{
    const int touchIndex = GetFirstNonUsedTouchIndex();
    if (touchIndex < 0)
    {
        DVASSERT(false);
        return false;
    }

    // Update digital part

    DigitalElementState& touchState = clicks[touchIndex];
    touchState.Press();

    // Update analog part

    AnalogElementState& analogState = positions[touchIndex];
    analogState.x = e.touchEvent.x;
    analogState.y = e.touchEvent.y;

    // Save native touch id to be able to locate correct touch when TOUCH_UP event comes

    nativeTouchIds[touchIndex] = e.touchEvent.touchId;

    // Send input event

    eInputElements element = static_cast<eInputElements>(eInputElements::TOUCH_FIRST_CLICK + touchIndex);
    CreateAndSendTouchClickEvent(element, touchState, e.window, e.timestamp);

    return true;
}

bool TouchScreen::HandleTouchUpEvent(const Private::MainDispatcherEvent& e)
{
    int touchIndex = GetTouchIndexFromNativeTouchId(e.touchEvent.touchId);
    if (touchIndex == -1)
    {
        // We can receive touch up event without registred touch
        // E.g. when we touch a screen then press a Win button twice on Windows 10
        // First press leads to touch state reseting (in OnWindowFocusChanged),
        // and second one brings back focus to our window and we start receiving events with the same touch id
        // Don't want to handle these
        return false;
    }

    // Update digital part

    DigitalElementState& touchState = clicks[touchIndex];
    touchState.Release();

    // Reset native id for this touch

    nativeTouchIds[touchIndex] = 0;

    // Update analog part

    AnalogElementState& analogState = positions[touchIndex];
    analogState.x = e.touchEvent.x;
    analogState.y = e.touchEvent.y;

    // Send input event

    eInputElements element = static_cast<eInputElements>(eInputElements::TOUCH_FIRST_CLICK + touchIndex);
    CreateAndSendTouchClickEvent(element, touchState, e.window, e.timestamp);

    // If it's an up event, reset position AFTER sending the input event
    // (so that users can request it during handling and get correct position)
    analogState.x = 0.0f;
    analogState.y = 0.0f;

    return true;
}

bool TouchScreen::HandleTouchMoveEvent(const Private::MainDispatcherEvent& e)
{
    int touchIndex = GetTouchIndexFromNativeTouchId(e.touchEvent.touchId);
    if (touchIndex == -1)
    {
        // We can receive touch up event without registred touch
        // E.g. when we touch a screen then press a Win button twice on Windows 10
        // First press leads to touch state reseting (in OnWindowFocusChanged),
        // and second one brings back focus to our window and we start receiving events with the same touch id
        // Don't want to handle these
        return false;
    }

    // Update analog part

    AnalogElementState& analogState = positions[touchIndex];
    analogState.x = e.touchEvent.x;
    analogState.y = e.touchEvent.y;

    // Send input event

    InputEvent inputEvent;
    inputEvent.window = e.window;
    inputEvent.timestamp = static_cast<float64>(e.timestamp / 1000.0f);
    inputEvent.deviceType = eInputDeviceTypes::TOUCH_SURFACE;
    inputEvent.device = this;
    inputEvent.analogState = analogState;
    inputEvent.elementId = static_cast<eInputElements>(eInputElements::TOUCH_FIRST_POSITION + touchIndex);

    inputSystem->DispatchInputEvent(inputEvent);

    return true;
}

void TouchScreen::CreateAndSendTouchClickEvent(eInputElements elementId, DigitalElementState state, Window* window, int64 timestamp)
{
    InputEvent inputEvent;
    inputEvent.window = window;
    inputEvent.timestamp = static_cast<float64>(timestamp / 1000.0f);
    inputEvent.deviceType = eInputDeviceTypes::TOUCH_SURFACE;
    inputEvent.device = this;
    inputEvent.digitalState = state;
    inputEvent.elementId = elementId;

    inputSystem->DispatchInputEvent(inputEvent);
}

int TouchScreen::GetTouchIndexFromNativeTouchId(uint32 nativeTouchId) const
{
    int touchIndex = -1;
    for (int i = 0; i < INPUT_ELEMENTS_TOUCH_CLICK_COUNT; ++i)
    {
        if (nativeTouchIds[i] == nativeTouchId)
        {
            touchIndex = i;
            break;
        }
    }

    return touchIndex;
}

int TouchScreen::GetFirstNonUsedTouchIndex() const
{
    for (int i = 0; i < INPUT_ELEMENTS_TOUCH_CLICK_COUNT; ++i)
    {
        if (clicks[i].IsReleased())
        {
            return i;
        }
    }

    return -1;
}
} // namespace DAVA
