#include "Tests/FullscreenTest.h"
#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <Engine/Window.h>
#include <Entity/ComponentUtils.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/InputSystem.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

FullscreenTest::FullscreenTest(TestBed& app)
    : BaseScreen(app, "FullscreenTest")
{
}

void FullscreenTest::LoadResources()
{
    BaseScreen::LoadResources();

    inputHandlerToken = GetEngineContext()->inputSystem->AddHandler(eInputDevices::CLASS_KEYBOARD, MakeFunction(this, &FullscreenTest::OnToggleFullscreen));
    GetPrimaryWindow()->sizeChanged.Connect(this, &FullscreenTest::OnWindowSizeChanged);
    UIControlBackground* background = GetOrCreateComponent<UIControlBackground>();
    background->SetColor(Color::White);

    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    float y = 35;

    // Screen mode buttons
    ScopedPtr<UIButton> btn(new UIButton(Rect(10, y, 150, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Windowed");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(0);
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::OnSelectModeClick));
    AddControl(btn);

    btn.reset(new UIButton(Rect(170, y, 150, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Fullsreen");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(1);
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::OnSelectModeClick));
    AddControl(btn);

    // Screen mode info
    btn.reset(new UIButton(Rect(520, y, 80, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Refresh");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(99);
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::OnSelectModeClick));
    AddControl(btn);

    currentModeText = new UIStaticText(Rect(300, y, 220, 50));
    currentModeText->SetFont(font);
    currentModeText->SetTextColor(Color::White);
    AddControl(currentModeText);

    y += 70;

    // pinning mode
    btn.reset(new UIButton(Rect(10, y, 150, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Toggle Mouse Visibility");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(0);
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::OnPinningClick));
    AddControl(btn);

    btn.reset(new UIButton(Rect(170, y, 150, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Toggle Mouse Pinning");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(1);
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::OnPinningClick));
    AddControl(btn);

    pinningText = new UIStaticText(Rect(300, y, 220, 20));
    pinningText->SetFont(font);
    pinningText->SetMultiline(true);
    pinningText->SetTextColor(Color::White);
    AddControl(pinningText);

    pinningMousePosText = new UIStaticText(Rect(300, y + 50, 220, 20));
    pinningMousePosText->SetFont(font);
    pinningMousePosText->SetTextColor(Color(0.5f, 0.5f, .0f, 1.0f));
    pinningMousePosText->SetVisibilityFlag(false);
    AddControl(pinningMousePosText);

    y = 170;

    // UI3DView test
    ui3dview = new UI3DView(Rect(10, y, 320, 240));
    ui3dview->GetOrCreateComponent<UIDebugRenderComponent>();

    ScopedPtr<Scene> scene(new Scene());
    scene->LoadScene("~res:/TestBed/3d/Objects/monkey.sc2");

    ScopedPtr<Camera> camera(new Camera());
    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;
    float32 aspect = static_cast<float32>(vcs->GetVirtualScreenSize().dy) / vcs->GetVirtualScreenSize().dx;
    camera->SetupPerspective(70.f, aspect, 0.5f, 2500.f);
    camera->SetLeft(Vector3(1, 0, 0));
    camera->SetUp(Vector3(0, 0, 1.f));
    camera->SetTarget(Vector3(0, 0, 0));
    camera->SetPosition(Vector3(0, -10, 1));

    ScopedPtr<Entity> cameraEntity(new Entity());
    cameraEntity->AddComponent(new CameraComponent(camera));
    cameraEntity->AddComponent(new RotationControllerComponent());
    scene->AddNode(cameraEntity);

    rotationControllerSystem = new RotationControllerSystem(scene);
    scene->AddSystem(rotationControllerSystem,
                     ComponentUtils::MakeMask<CameraComponent>() | ComponentUtils::MakeMask<RotationControllerComponent>(),
                     Scene::SCENE_SYSTEM_REQUIRE_PROCESS | Scene::SCENE_SYSTEM_REQUIRE_INPUT);

    scene->AddCamera(camera);
    scene->SetCurrentCamera(camera);
    ui3dview->SetScene(scene);
    AddControl(ui3dview);

    viewScalePlus = new UIButton(Rect(340, y, 145, 50));
    viewScalePlus->SetStateFont(0xFF, font);
    viewScalePlus->SetStateText(0xFF, L"3dView FBO Scale +0.1");
    viewScalePlus->GetOrCreateComponent<UIDebugRenderComponent>();
    viewScalePlus->SetTag(0);
    viewScalePlus->SetDisabled(true);
    viewScalePlus->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    viewScalePlus->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    viewScalePlus->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::On3DViewControllClick));
    AddControl(viewScalePlus);

    y += 60;

    currentScaleText = new UIStaticText(Rect(340, y, 145, 20));
    currentScaleText->SetFont(font);
    currentScaleText->SetTextColor(Color::White);
    currentScaleText->SetText(Format(L"%f", ui3dview->GetFrameBufferScaleFactor()));
    AddControl(currentScaleText);

    y += 30;

    viewScaleMinus = new UIButton(Rect(340, y, 145, 50));
    viewScaleMinus->SetStateFont(0xFF, font);
    viewScaleMinus->SetStateText(0xFF, L"3dView FBO Scale -0.1");
    viewScaleMinus->GetOrCreateComponent<UIDebugRenderComponent>();
    viewScaleMinus->SetTag(1);
    viewScaleMinus->SetDisabled(true);
    viewScaleMinus->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    viewScaleMinus->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    viewScaleMinus->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::On3DViewControllClick));
    AddControl(viewScaleMinus);

    y = 170 + 250;

    btn.reset(new UIButton(Rect(10, y, 150, 40)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"3dView FBO On");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(2);
    btn->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::On3DViewControllClick));
    AddControl(btn);

    btn.reset(new UIButton(Rect(170, y, 150, 40)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"3dView FBO Off");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetTag(3);
    btn->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FullscreenTest::On3DViewControllClick));
    AddControl(btn);

    GetPrimaryWindow()->sizeChanged.Connect(this, [this](Window*, Size2f, Size2f) { UpdateMode(); });

    // Scale factor test
    y = 170;

    btn.reset(new UIButton(Rect(500, y, 145, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Whole Scale +0.1");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &FullscreenTest::OnMulUp));
    AddControl(btn);

    y += 60;

    currentScaleText = new UIStaticText(Rect(500, y, 145, 20));
    currentScaleText->SetFont(font);
    currentScaleText->SetTextColor(Color::White);
    currentScaleText->SetText(Format(L"%f", GetPrimaryWindow()->GetSurfaceScale()));
    AddControl(currentScaleText);

    y += 30;

    btn.reset(new UIButton(Rect(500, y, 145, 50)));
    btn->SetStateFont(0xFF, font);
    btn->SetStateText(0xFF, L"Whole Scale -0.1");
    btn->GetOrCreateComponent<UIDebugRenderComponent>();
    btn->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    btn->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &FullscreenTest::OnMulDown));
    AddControl(btn);

    UpdateMode();
}

void FullscreenTest::UnloadResources()
{
    GetEngineContext()->inputSystem->RemoveHandler(inputHandlerToken);
    GetPrimaryWindow()->sizeChanged.Disconnect(this);

    if (ui3dview->GetScene())
    {
        ui3dview->GetScene()->RemoveSystem(rotationControllerSystem);
    }
    SafeDelete(rotationControllerSystem);
    SafeRelease(ui3dview);
    SafeRelease(currect3dScaleText);
    SafeRelease(currentModeText);
    SafeRelease(pinningText);
    SafeRelease(pinningMousePosText);

    // TODO: UIControls and others should be deleted when window is destroyed, not later
    if (GetPrimaryWindow() != nullptr)
    {
        GetPrimaryWindow()->sizeChanged.Disconnect(this);
    }

    BaseScreen::UnloadResources();
}

void FullscreenTest::OnSelectModeClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = static_cast<UIButton*>(sender);
    switch (btn->GetTag())
    {
    case 0:
        GetPrimaryWindow()->SetFullscreenAsync(eFullscreen::Off);
        break;
    case 1:
        GetPrimaryWindow()->SetFullscreenAsync(eFullscreen::On);
        break;
    case 99:
        UpdateMode();
        break;
    }
}

void FullscreenTest::OnMulUp(BaseObject* sender, void* data, void* callerData)
{
    float32 mul = GetPrimaryWindow()->GetSurfaceScale();
    mul += 0.1f;

    GetPrimaryWindow()->SetSurfaceScaleAsync(mul);
}

void FullscreenTest::OnMulDown(BaseObject* sender, void* data, void* callerData)
{
    float32 mul = GetPrimaryWindow()->GetSurfaceScale();
    mul -= 0.1f;

    GetPrimaryWindow()->SetSurfaceScaleAsync(mul);
}

void FullscreenTest::On3DViewControllClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = static_cast<UIButton*>(sender);
    switch (btn->GetTag())
    {
    case 0: // + scale
    {
        float32 mul = ui3dview->GetFrameBufferScaleFactor();
        if (mul < 2.0f)
        {
            mul += 0.1f;
        }
        ui3dview->SetFrameBufferScaleFactor(mul);
        currentScaleText->SetText(Format(L"%f", mul));
        break;
    }
    case 1: // - scale
    {
        float32 mul = ui3dview->GetFrameBufferScaleFactor();
        if (mul > 0.2f)
        {
            mul -= 0.1f;
        }
        ui3dview->SetFrameBufferScaleFactor(mul);
        currentScaleText->SetText(Format(L"%f", mul));
        break;
    }
    case 2: // turn on
        ui3dview->SetDrawToFrameBuffer(true);
        viewScalePlus->SetDisabled(false);
        viewScaleMinus->SetDisabled(false);
        break;
    case 3: // turn off
        ui3dview->SetDrawToFrameBuffer(false);
        viewScalePlus->SetDisabled(true);
        viewScaleMinus->SetDisabled(true);
        break;
    }
}

void FullscreenTest::OnPinningClick(DAVA::BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = static_cast<UIButton*>(sender);
    switch (btn->GetTag())
    {
    case 0:
        GetPrimaryWindow()->SetCursorVisibility(false);
        break;
    case 1:
        GetPrimaryWindow()->SetCursorCapture(eCursorCapture::PINNING);
        break;
    default:
        break;
    }

    UpdateMode();
}

bool FullscreenTest::OnToggleFullscreen(const DAVA::InputEvent& inputEvent)
{
    if (inputEvent.deviceType == eInputDevices::KEYBOARD && inputEvent.keyboardEvent.charCode == 0)
    {
        if (inputEvent.digitalState.IsJustReleased())
        {
            Window* window = GetPrimaryWindow();

            const bool altPressed = inputEvent.device->GetDigitalElementState(eInputElements::KB_LALT).IsPressed() || inputEvent.device->GetDigitalElementState(eInputElements::KB_RALT).IsPressed();
            if ((inputEvent.elementId == eInputElements::KB_ENTER || inputEvent.elementId == eInputElements::KB_NUMPAD_ENTER) && altPressed)
            {
                eFullscreen mode = window->GetFullscreen();
                mode = mode == eFullscreen::On ? eFullscreen::Off : eFullscreen::On;
                window->SetFullscreenAsync(mode);
            }
            else if (inputEvent.elementId == eInputElements::KB_P)
            {
                eCursorCapture mode = window->GetCursorCapture();
                mode = mode == eCursorCapture::OFF ? eCursorCapture::PINNING : eCursorCapture::OFF;
                window->SetCursorCapture(mode);
            }
        }
    }

    return false;
}

void FullscreenTest::UpdateMode()
{
    Window* w = GetPrimaryWindow();
    if (w->GetFullscreen() == eFullscreen::On)
    {
        currentModeText->SetText(L"Fullscreen");
    }
    else
    {
        currentModeText->SetText(L"Windowed");
    }

    WideString outStr;
    if (w->GetCursorCapture() == eCursorCapture::PINNING)
    {
        outStr += L"Mouse Capture = PINNING";
        outStr += L"\n";
        outStr += L"Mouse visibility = OFF";
        outStr += L"\n";
        outStr += L"press Middle Mouse Button to turn off";
        pinningMousePosText->SetVisibilityFlag(true);
    }
    else
    {
        outStr += L"Mouse Capture = OFF";
        outStr += L"\n";
        if (w->GetCursorVisibility())
        {
            outStr += L"Mouse visibility = ON";
            pinningMousePosText->SetVisibilityFlag(false);
        }
        else
        {
            outStr += L"Mouse visibility = OFF";
            outStr += L"\n";
            outStr += L"press Middle Mouse Button to turn off";
            pinningMousePosText->SetVisibilityFlag(true);
        }
    }
    pinningText->SetText(outStr.c_str());
}

bool FullscreenTest::SystemInput(UIEvent* currentInput)
{
    if (currentInput->device == eInputDevices::MOUSE)
    {
        Window* w = GetPrimaryWindow();
        switch (currentInput->phase)
        {
        case UIEvent::Phase::BEGAN:
            if (currentInput->mouseButton == eMouseButtons::MIDDLE)
            {
                w->SetCursorCapture(eCursorCapture::OFF);
                w->SetCursorVisibility(true);
            }
            break;

        case UIEvent::Phase::MOVE:
        case UIEvent::Phase::DRAG:
            pinningMousePosText->SetText(Format(L"dx: %f, dy: %f, rel=%d", currentInput->physPoint.dx, currentInput->physPoint.dy, currentInput->isRelative));
            break;

        default:
            break;
        }

        UpdateMode();
    }

    return BaseScreen::SystemInput(currentInput);
}

void FullscreenTest::OnWindowSizeChanged(DAVA::Window*, DAVA::Size2f windowSize, DAVA::Size2f surfaceSize)
{
    currentScaleText->SetText(Format(L"%f", GetPrimaryWindow()->GetSurfaceScale()));
}
