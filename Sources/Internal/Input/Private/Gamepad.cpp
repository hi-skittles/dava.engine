#include "Input/Gamepad.h"

#include <algorithm>

#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"
#include "Input/InputEvent.h"
#include "Input/InputSystem.h"

#if defined(__DAVAENGINE_ANDROID__)
#include "Input/Private/Android/GamepadImpl.Android.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "Input/Private/Win10/GamepadImpl.Win10.h"
#elif defined(__DAVAENGINE_WIN32__)
#include "Input/Private/Win32/GamepadImpl.Win32.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "Input/Private/Mac/GamepadImpl.Macos.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "Input/Private/Ios/GamepadImpl.Ios.h"
#elif defined(__DAVAENGINE_LINUX__)
#include "Input/Private/Linux/GamepadImpl.Linux.h"
#else
#error "GamepadDevice: unknown platform"
#endif

namespace DAVA
{
Gamepad::Gamepad(uint32 id)
    : InputDevice(id)
    , inputSystem(GetEngineContext()->inputSystem)
    , impl(new Private::GamepadImpl(this))
    , buttons{}
    , axes{}
{
    Engine* engine = Engine::Instance();

    endFrameConnectionToken = engine->endFrame.Connect(this, &Gamepad::OnEndFrame);

    Private::EngineBackend::Instance()->InstallEventFilter(this, MakeFunction(this, &Gamepad::HandleMainDispatcherEvent));
}

Gamepad::~Gamepad()
{
    Engine* engine = Engine::Instance();

    engine->endFrame.Disconnect(endFrameConnectionToken);

    Private::EngineBackend::Instance()->UninstallEventFilter(this);
}

DigitalElementState Gamepad::GetStartButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_START);
}

DigitalElementState Gamepad::GetAButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_A);
}

DigitalElementState Gamepad::GetBButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_B);
}

DigitalElementState Gamepad::GetXButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_X);
}

DigitalElementState Gamepad::GetYButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_Y);
}

DigitalElementState Gamepad::GetLeftDPadButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_DPAD_LEFT);
}

DigitalElementState Gamepad::GetRightDPadButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_DPAD_RIGHT);
}

DigitalElementState Gamepad::GetUpDPadButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_DPAD_UP);
}

DigitalElementState Gamepad::GetDownDPadButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_DPAD_DOWN);
}

DigitalElementState Gamepad::GetLeftThumbButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_LTHUMB);
}

DigitalElementState Gamepad::GetRightThumbButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_RTHUMB);
}

DigitalElementState Gamepad::GetLeftShoulderButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_LSHOULDER);
}

DigitalElementState Gamepad::GetRightShoulderButtonState() const
{
    return GetDigitalElementState(eInputElements::GAMEPAD_RSHOULDER);
}

AnalogElementState Gamepad::GetLeftTriggerAxis() const
{
    return GetAnalogElementState(eInputElements::GAMEPAD_AXIS_LTRIGGER);
}

AnalogElementState Gamepad::GetRightTriggerAxis() const
{
    return GetAnalogElementState(eInputElements::GAMEPAD_AXIS_RTRIGGER);
}

AnalogElementState Gamepad::GetLeftThumbAxis() const
{
    return GetAnalogElementState(eInputElements::GAMEPAD_AXIS_LTHUMB);
}

AnalogElementState Gamepad::GetRightThumbAxis() const
{
    return GetAnalogElementState(eInputElements::GAMEPAD_AXIS_RTHUMB);
}

bool Gamepad::IsElementSupported(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(IsGamepadAxisInputElement(elementId) || IsGamepadButtonInputElement(elementId));
    return supportedElements[elementId - eInputElements::GAMEPAD_FIRST];
}

DigitalElementState Gamepad::GetDigitalElementState(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(IsGamepadButtonInputElement(elementId));
    return buttons[elementId - eInputElements::GAMEPAD_FIRST_BUTTON];
}

AnalogElementState Gamepad::GetAnalogElementState(eInputElements elementId) const
{
    DVASSERT(Thread::IsMainThread());

    DVASSERT(IsGamepadAxisInputElement(elementId));
    return axes[elementId - eInputElements::GAMEPAD_FIRST_AXIS];
}

void Gamepad::Update()
{
    impl->Update();

    Window* window = GetPrimaryWindow();
    for (uint32 i = eInputElements::GAMEPAD_FIRST_BUTTON; i <= eInputElements::GAMEPAD_LAST_BUTTON; ++i)
    {
        uint32 index = i - eInputElements::GAMEPAD_FIRST_BUTTON;
        if (buttonChangedMask[index])
        {
            InputEvent inputEvent;
            inputEvent.window = window;
            inputEvent.deviceType = eInputDeviceTypes::CLASS_GAMEPAD;
            inputEvent.device = this;
            inputEvent.elementId = static_cast<eInputElements>(i);
            inputEvent.digitalState = buttons[index];
            inputSystem->DispatchInputEvent(inputEvent);
        }
    }
    buttonChangedMask.reset();

    for (uint32 i = eInputElements::GAMEPAD_FIRST_AXIS; i <= eInputElements::GAMEPAD_LAST_AXIS; ++i)
    {
        uint32 index = i - eInputElements::GAMEPAD_FIRST_AXIS;
        if (axisChangedMask[index])
        {
            InputEvent inputEvent;
            inputEvent.window = window;
            inputEvent.deviceType = eInputDeviceTypes::CLASS_GAMEPAD;
            inputEvent.device = this;
            inputEvent.elementId = static_cast<eInputElements>(i);
            inputEvent.analogState = axes[index];
            inputSystem->DispatchInputEvent(inputEvent);
        }
    }
    axisChangedMask.reset();
}

void Gamepad::OnEndFrame()
{
    for (DigitalElementState& buttonState : buttons)
    {
        buttonState.OnEndFrame();
    }
}

bool Gamepad::HandleMainDispatcherEvent(const Private::MainDispatcherEvent& e)
{
    using Private::MainDispatcherEvent;

    if (e.type == MainDispatcherEvent::WINDOW_CANCEL_INPUT)
    {
        ResetState(e.window);
    }

    return false;
}

void Gamepad::HandleGamepadAdded(const Private::MainDispatcherEvent& e)
{
    uint32 deviceId = e.gamepadEvent.deviceId;
    impl->HandleGamepadAdded(deviceId);
}

void Gamepad::HandleGamepadRemoved(const Private::MainDispatcherEvent& e)
{
    uint32 deviceId = e.gamepadEvent.deviceId;
    impl->HandleGamepadRemoved(deviceId);
}

void Gamepad::HandleGamepadMotion(const Private::MainDispatcherEvent& e)
{
    impl->HandleGamepadMotion(e);
}

void Gamepad::HandleGamepadButton(const Private::MainDispatcherEvent& e)
{
    impl->HandleGamepadButton(e);
}

void Gamepad::HandleButtonPress(eInputElements element, bool pressed)
{
    uint32 index = element - eInputElements::GAMEPAD_FIRST_BUTTON;
    DigitalElementState& buttonState = buttons[index];
    if (buttonState.IsPressed() != pressed)
    {
        pressed ? buttonState.Press() : buttonState.Release();
        buttonChangedMask.set(index);
    }
}

void Gamepad::HandleBackButtonPress(bool pressed)
{
    using namespace DAVA::Private;

    DigitalElementState& di = backButton;
    if (di.IsPressed() != pressed)
    {
        pressed ? di.Press() : di.Release();

        if (backButton.IsJustPressed())
        {
            EngineBackend::Instance()->GetDispatcher()->PostEvent(MainDispatcherEvent(MainDispatcherEvent::BACK_NAVIGATION));
        }
    }
}

void Gamepad::HandleAxisMovement(eInputElements element, float32 newValue, bool horizontal)
{
    // TODO: use some threshold for comparisons below

    uint32 index = element - eInputElements::GAMEPAD_FIRST_AXIS;
    if (horizontal)
    {
        if (newValue != axes[index].x)
        {
            axes[index].x = newValue;
            axisChangedMask.set(index);
        }
    }
    else
    {
        if (newValue != axes[index].y)
        {
            axes[index].y = newValue;
            axisChangedMask.set(index);
        }
    }
}

void Gamepad::ResetState(Window*)
{
    for (uint32 i = eInputElements::GAMEPAD_FIRST_BUTTON; i <= eInputElements::GAMEPAD_LAST_BUTTON; ++i)
    {
        HandleButtonPress(static_cast<eInputElements>(i), false);
    }

    for (uint32 i = eInputElements::GAMEPAD_FIRST_AXIS; i <= eInputElements::GAMEPAD_LAST_AXIS; ++i)
    {
        eInputElements axis = static_cast<eInputElements>(i);
        HandleAxisMovement(axis, 0.0f, true);
        HandleAxisMovement(axis, 0.0f, false);
    }
}
} // namespace DAVA
