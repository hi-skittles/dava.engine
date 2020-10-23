#include "Infrastructure/TestBed.h"
#include "Tests/InputSystemTest.h"

#include <Engine/Engine.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/InputBindingListener.h>
#include <Utils/UTF8Utils.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

static const DAVA::FastName ACTION_1 = DAVA::FastName("ACTION 1");
static const DAVA::FastName ACTION_2 = DAVA::FastName("ACTION 2");
static const DAVA::FastName ACTION_3 = DAVA::FastName("ACTION 3");
static const DAVA::FastName ACTION_4 = DAVA::FastName("ACTION 4");
static const DAVA::FastName ACTION_5 = DAVA::FastName("ACTION 5");

InputSystemTest::InputSystemTest(TestBed& app)
    : BaseScreen(app, "InputSystemTest")
{
}

void InputSystemTest::LoadResources()
{
    BaseScreen::LoadResources();

    const EngineContext* context = GetEngineContext();

    // Create UI
    CreateKeyboardUI("Scancode keyboard", 20.0f, 20.0f);
    CreateMouseUI();
    CreateTouchUI();
    CreateActionsUI();
    CreateInputListenerUI();

    // Subscribe to events
    rawInputToken = GetEngineContext()->inputSystem->AddHandler(eInputDeviceTypes::CLASS_ALL, MakeFunction(this, &InputSystemTest::OnInputEvent));
    context->actionSystem->ActionTriggered.Connect(this, &InputSystemTest::OnAction);
    Engine::Instance()->update.Connect(this, &InputSystemTest::OnUpdate);

    // Bind action set

    ActionSet set;

    DigitalBinding action1;
    action1.actionId = ACTION_1;
    action1.digitalElements[0] = eInputElements::KB_W;
    action1.digitalStates[0] = DigitalElementState::Pressed();
    set.digitalBindings.push_back(action1);

    DigitalBinding action2;
    action2.actionId = ACTION_2;
    action2.digitalElements[0] = eInputElements::KB_SPACE;
    action2.digitalStates[0] = DigitalElementState::JustPressed();
    set.digitalBindings.push_back(action2);

    DigitalBinding action3;
    action3.actionId = ACTION_3;
    action3.digitalElements[0] = eInputElements::KB_SPACE;
    action3.digitalStates[0] = DigitalElementState::JustPressed();
    action3.digitalElements[1] = eInputElements::KB_LSHIFT;
    action3.digitalStates[1] = DigitalElementState::Pressed();
    set.digitalBindings.push_back(action3);

    AnalogBinding action4;
    action4.actionId = ACTION_4;
    action4.analogElementId = eInputElements::MOUSE_POSITION;
    set.analogBindings.push_back(action4);

    AnalogBinding action5;
    action5.actionId = ACTION_5;
    action5.analogElementId = eInputElements::MOUSE_POSITION;
    action5.digitalElements[0] = eInputElements::MOUSE_LBUTTON;
    action5.digitalStates[0] = DigitalElementState::Pressed();
    action5.digitalElements[1] = eInputElements::KB_LCTRL;
    action5.digitalStates[1] = DigitalElementState::Pressed();
    set.analogBindings.push_back(action5);

    keyboard = context->deviceManager->GetKeyboard();
    mouse = context->deviceManager->GetMouse();
    if (keyboard != nullptr && mouse != nullptr)
    {
        context->actionSystem->BindSet(set, keyboard->GetId(), mouse->GetId());
    }
}

void InputSystemTest::UnloadResources()
{
    GetEngineContext()->inputSystem->RemoveHandler(rawInputToken);
    GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
    Engine::Instance()->update.Disconnect(this);

    for (auto it = keyboardButtons.begin(); it != keyboardButtons.end(); ++it)
    {
        SafeRelease(it->second);
    }

    for (auto it = keyboardsHeaders.begin(); it != keyboardsHeaders.end(); ++it)
    {
        SafeRelease(*it);
    }

    SafeRelease(mouseHeader);
    SafeRelease(mouseBody);
    for (auto it = mouseButtons.begin(); it != mouseButtons.end(); ++it)
    {
        SafeRelease(it->second);
    }

    SafeRelease(touchHeader);
    for (auto it = touchClickButtons.begin(); it != touchClickButtons.end(); ++it)
    {
        SafeRelease(it->second);
    }
    for (auto it = touchMoveButtons.begin(); it != touchMoveButtons.end(); ++it)
    {
        SafeRelease(it->second);
    }

    for (auto it = actionCounters.begin(); it != actionCounters.end(); ++it)
    {
        SafeRelease(it->second);
    }

    SafeRelease(inputListenerDigitalSingleWithoutModifiersButton);
    SafeRelease(inputListenerDigitalSingleWithModifiersButton);
    SafeRelease(inputListenerDigitalMultipleAnyButton);
    SafeRelease(inputListenerAnalogButton);
    SafeRelease(inputListenerResultField);

    GetEngineContext()->actionSystem->UnbindAllSets();

    InputBindingListener* inputListener = GetEngineContext()->inputListener;
    if (inputListener->IsListening())
    {
        inputListener->StopListening();
    }

    BaseScreen::UnloadResources();
}

void InputSystemTest::CreateKeyboardUI(String header, float32 x, float32 y)
{
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    const float32 keyboardButtonWidth = 20.0f;
    const float32 keyboardButtonHeight = 20.0f;

    const float32 headerHeight = 15.0f;

    UIStaticText* headerText = new UIStaticText(Rect(x, y, 250, headerHeight));
    headerText->SetTextColor(Color::White);
    headerText->SetFont(font);
    headerText->SetFontSize(8.f);
    headerText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    headerText->SetUtf8Text(header);
    AddControl(headerText);

    keyboardsHeaders.push_back(headerText);

    y += headerHeight;

    const float32 initialX = x;
    const float32 initialY = y;

    float32 rightmostX = initialX;

    CreateKeyboardUIButton(eInputElements::KB_ESCAPE, L"ESC", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F1, L"F1", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F2, L"F2", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F3, L"F3", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F4, L"F4", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F5, L"F5", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F6, L"F6", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F7, L"F7", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F8, L"F8", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F9, L"F9", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F10, L"F10", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F11, L"F11", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F12, L"F12", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = initialX;

    CreateKeyboardUIButton(eInputElements::KB_GRAVE, L"`", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_1, L"1", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_2, L"2", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_3, L"3", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_4, L"4", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_5, L"5", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_6, L"6", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_7, L"7", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_8, L"8", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_9, L"9", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_0, L"0", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_MINUS, L"-", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_EQUALS, L"+", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_BACKSPACE, L"<-", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = initialX;

    CreateKeyboardUIButton(eInputElements::KB_TAB, L"TAB", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_Q, L"Q", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_W, L"W", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_E, L"E", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_R, L"R", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_T, L"T", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_Y, L"Y", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_U, L"U", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_I, L"I", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_O, L"O", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_P, L"P", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_LBRACKET, L"{", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RBRACKET, L"}", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_ENTER, L"ENTER", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = initialX;

    CreateKeyboardUIButton(eInputElements::KB_CAPSLOCK, L"CAPS", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_A, L"A", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_S, L"S", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_D, L"D", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_F, L"F", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_G, L"G", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_H, L"H", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_J, L"J", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_K, L"K", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_L, L"L", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_SEMICOLON, L":", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_APOSTROPHE, L"\"", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_BACKSLASH, L"\\", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = initialX;

    CreateKeyboardUIButton(eInputElements::KB_LSHIFT, L"SHFT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NONUSBACKSLASH, L"\\", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_Z, L"Z", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_X, L"X", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_C, L"C", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_V, L"V", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_B, L"B", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_N, L"N", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_M, L"M", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_COMMA, L"<", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_PERIOD, L">", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_SLASH, L"?", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RSHIFT, L"Shift", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = initialX;

    CreateKeyboardUIButton(eInputElements::KB_LCTRL, L"CTRL", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_LWIN, L"WIN", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_LALT, L"ALT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_LCMD, L"CMD", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_SPACE, L"SPACE", font, &x, y, keyboardButtonWidth * 4, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RCMD, L"CMD", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RALT, L"ALT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RWIN, L"WIN", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_MENU, L"MENU", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RCTRL, L"CTRL", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    const float32 middleSectionStartX = rightmostX + keyboardButtonWidth;

    y = initialY;
    x = middleSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_PRINTSCREEN, L"PSCR", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_SCROLLLOCK, L"SCROLLLOCK", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_PAUSE, L"PAUSE", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = middleSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_INSERT, L"INSERT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_HOME, L"HOME", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_PAGEUP, L"PGUP", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = middleSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_DELETE, L"DEL", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_END, L"END", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_PAGEDOWN, L"PGDOWN", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += 2.0f * (keyboardButtonHeight + 1.0f);
    x = middleSectionStartX + keyboardButtonWidth + 1.0f;
    CreateKeyboardUIButton(eInputElements::KB_UP, L"UP", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = middleSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_LEFT, L"LEFT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_DOWN, L"DOWN", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_RIGHT, L"RIGHT", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    const float32 numpadSectionStartX = rightmostX + keyboardButtonWidth;

    y = initialY;
    x = numpadSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_NUMLOCK, L"NUMLOCK", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_DIVIDE, L"/", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_MULTIPLY, L"*", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_MINUS, L"-", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = numpadSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_7, L"7", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_8, L"8", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_9, L"9", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_PLUS, L"+", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = numpadSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_4, L"4", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_5, L"5", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_6, L"6", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = numpadSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_1, L"1", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_2, L"2", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_3, L"3", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_ENTER, L"ENTER", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);

    rightmostX = std::max(x, rightmostX);
    y += keyboardButtonHeight + 1.0f;
    x = numpadSectionStartX;
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_0, L"0", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
    CreateKeyboardUIButton(eInputElements::KB_NUMPAD_DELETE, L"DEL", font, &x, y, keyboardButtonWidth, keyboardButtonHeight);
}

void InputSystemTest::CreateMouseUI()
{
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    const float32 x = 530;

    mouseHeader = new UIStaticText(Rect(x, 20, 250, 15));
    mouseHeader->SetTextColor(Color::White);
    mouseHeader->SetFont(font);
    mouseHeader->SetFontSize(10.f);
    mouseHeader->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    mouseHeader->SetUtf8Text("Mouse");
    AddControl(mouseHeader);

    mouseBody = new UIButton(Rect(x, 40, 74, 100));
    mouseBody->SetStateFont(0xFF, font);
    mouseBody->SetStateFontColor(0xFF, Color::White);
    mouseBody->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(mouseBody);

    UIButton* mousePositionButton = new UIButton(Rect(x + 100, 40, 100, 15));
    mousePositionButton->SetStateFont(0xFF, font);
    mousePositionButton->SetStateFontColor(0xFF, Color::White);
    mousePositionButton->GetOrCreateComponent<UIDebugRenderComponent>();
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_POSITION)] = mousePositionButton;
    AddControl(mousePositionButton);

    UIButton* wheelButton = new UIButton(Rect(x + 100, 65, 100, 15));
    wheelButton->SetStateFont(0xFF, font);
    wheelButton->SetStateFontColor(0xFF, Color::White);
    wheelButton->GetOrCreateComponent<UIDebugRenderComponent>();
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_WHEEL)] = wheelButton;
    AddControl(wheelButton);

    UIButton* leftButton = new UIButton(Rect(x + 15.0f, 40, 15, 70));
    leftButton->SetStateFont(0xFF, font);
    leftButton->SetStateFontColor(0xFF, Color::White);
    leftButton->GetOrCreateComponent<UIDebugRenderComponent>();
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_LBUTTON)] = leftButton;
    AddControl(leftButton);

    UIButton* middleButton = new UIButton(Rect(x + 32.0f, 60, 10, 20));
    middleButton->SetStateFont(0xFF, font);
    middleButton->SetStateFontColor(0xFF, Color::White);
    middleButton->GetOrCreateComponent<UIDebugRenderComponent>();
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_MBUTTON)] = middleButton;
    AddControl(middleButton);

    UIButton* rightButton = new UIButton(Rect(x + 45.0f, 40, 15, 70));
    rightButton->SetStateFont(0xFF, font);
    rightButton->SetStateFontColor(0xFF, Color::White);
    rightButton->GetOrCreateComponent<UIDebugRenderComponent>();
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_RBUTTON)] = rightButton;
    AddControl(rightButton);

    UIButton* ext1Button = new UIButton(Rect(x + 100, 90, 100, 15));
    ext1Button->SetStateFont(0xFF, font);
    ext1Button->SetStateFontColor(0xFF, Color::White);
    ext1Button->GetOrCreateComponent<UIDebugRenderComponent>();
    ext1Button->SetStateText(0xFF, L"Ext1 button");
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_EXT1BUTTON)] = ext1Button;
    AddControl(ext1Button);

    UIButton* ext2Button = new UIButton(Rect(x + 100, 115, 100, 15));
    ext2Button->SetStateFont(0xFF, font);
    ext2Button->SetStateFontColor(0xFF, Color::White);
    ext2Button->GetOrCreateComponent<UIDebugRenderComponent>();
    ext2Button->SetStateText(0xFF, L"Ext2 button");
    mouseButtons[static_cast<uint32>(eInputElements::MOUSE_EXT2BUTTON)] = ext2Button;
    AddControl(ext2Button);
}

void InputSystemTest::CreateTouchUI()
{
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    float32 x = 20.0f;
    float32 y = 185.0f;

    touchHeader = new UIStaticText(Rect(x, y, 250, 15));
    touchHeader->SetTextColor(Color::White);
    touchHeader->SetFont(font);
    touchHeader->SetFontSize(12.f);
    touchHeader->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    touchHeader->SetUtf8Text("Touch");
    AddControl(touchHeader);

    y += 17.0f;

    const float32 clickButtonSizeX = 32.8f;
    const float32 clickButtonSizeY = 20.0f;
    for (size_t i = 0; i < INPUT_ELEMENTS_TOUCH_CLICK_COUNT; ++i)
    {
        std::wstringstream ss;
        ss << i;

        UIButton* touchClickButton = new UIButton(Rect(x, y, clickButtonSizeX, clickButtonSizeY));
        touchClickButton->SetStateFont(0xFF, font);
        touchClickButton->SetStateFontSize(0xFF, 12.f);
        touchClickButton->SetStateFontColor(0xFF, Color::White);
        touchClickButton->GetOrCreateComponent<UIDebugRenderComponent>();
        touchClickButton->SetStateText(0xFF, ss.str());
        touchClickButtons[static_cast<uint32>(eInputElements::TOUCH_FIRST_CLICK + i)] = touchClickButton;
        AddControl(touchClickButton);

        UIButton* touchMoveButton = new UIButton(Rect(x, y + clickButtonSizeY + 1.0f, clickButtonSizeX, clickButtonSizeY * 2.0f));
        touchMoveButton->SetStateFont(0xFF, font);
        touchMoveButton->SetStateFontSize(0xFF, 12.f);
        touchMoveButton->SetStateFontColor(0xFF, Color::White);
        touchMoveButton->GetOrCreateComponent<UIDebugRenderComponent>();
        touchMoveButton->SetStateText(0xFF, L"0\n0");
        touchMoveButton->SetStateTextMultiline(0xFF, true);
        touchMoveButtons[static_cast<uint32>(eInputElements::TOUCH_FIRST_POSITION + i)] = touchMoveButton;
        AddControl(touchMoveButton);

        x += clickButtonSizeX + 1.0f;
    }
}

void InputSystemTest::CreateActionsUI()
{
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    float32 y = 370.0f;
    const float32 yDelta = 30.0f;

    //

    UIStaticText* staticText = new UIStaticText(Rect(10, y, 200, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("Action 1 (W pressed):");
    AddControl(staticText);

    staticText = new UIStaticText(Rect(215, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("0");
    AddControl(staticText);

    actionCounters[ACTION_1] = staticText;

    staticText = new UIStaticText(Rect(270, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("not active");
    AddControl(staticText);

    digitalActionsStatus[ACTION_1] = staticText;

    y += yDelta;

    //

    staticText = new UIStaticText(Rect(10, y, 200, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("Action 2 (Space just pressed):");
    AddControl(staticText);

    staticText = new UIStaticText(Rect(215, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("0");
    AddControl(staticText);

    actionCounters[ACTION_2] = staticText;

    staticText = new UIStaticText(Rect(270, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("not active");
    AddControl(staticText);

    digitalActionsStatus[ACTION_2] = staticText;

    y += yDelta;

    //

    staticText = new UIStaticText(Rect(10, y, 200, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("Action 3 (Left Shift pressed, Space just pressed):");
    AddControl(staticText);

    staticText = new UIStaticText(Rect(215, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("0");
    AddControl(staticText);

    actionCounters[ACTION_3] = staticText;

    staticText = new UIStaticText(Rect(270, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("not active");
    AddControl(staticText);

    digitalActionsStatus[ACTION_3] = staticText;

    y += yDelta;

    //

    staticText = new UIStaticText(Rect(10, y, 200, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("Action 4 (Mouse Cursor):");
    AddControl(staticText);

    staticText = new UIStaticText(Rect(215, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("0");
    AddControl(staticText);

    actionCounters[ACTION_4] = staticText;

    staticText = new UIStaticText(Rect(270, y, 130, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("not active");
    AddControl(staticText);

    analogActionsStatus[ACTION_4] = staticText;

    y += yDelta;

    //

    staticText = new UIStaticText(Rect(10, y, 200, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("Action 5 (Mouse Cursor, Left Ctrl pressed, Mouse Left Button pressed):");
    AddControl(staticText);

    staticText = new UIStaticText(Rect(215, y, 50, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    staticText->SetUtf8Text("0");
    AddControl(staticText);

    actionCounters[ACTION_5] = staticText;

    staticText = new UIStaticText(Rect(270, y, 130, 30));
    staticText->SetTextColor(Color::White);
    staticText->SetFont(font);
    staticText->SetFontSize(9.f);
    staticText->SetMultiline(true);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    staticText->SetUtf8Text("not active");
    AddControl(staticText);

    analogActionsStatus[ACTION_5] = staticText;

    y += yDelta;

    //
}

void InputSystemTest::CreateInputListenerUI()
{
    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    const float32 x = 530.0f;
    float32 y = 370.0f;
    const float32 yDelta = 30.0f;

    inputListenerDigitalSingleWithoutModifiersButton = new UIButton(Rect(x, y, 200, 30));
    inputListenerDigitalSingleWithoutModifiersButton->SetStateFont(0xFF, font);
    inputListenerDigitalSingleWithoutModifiersButton->SetStateFontSize(0xFF, 11.f);
    inputListenerDigitalSingleWithoutModifiersButton->SetStateFontColor(0xFF, Color::White);
    inputListenerDigitalSingleWithoutModifiersButton->GetOrCreateComponent<UIDebugRenderComponent>();
    inputListenerDigitalSingleWithoutModifiersButton->SetStateText(0xFF, L"Listen: digital single without modifiers");
    inputListenerDigitalSingleWithoutModifiersButton->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &InputSystemTest::OnInputListenerButtonPressed));
    AddControl(inputListenerDigitalSingleWithoutModifiersButton);

    y += yDelta;

    inputListenerDigitalSingleWithModifiersButton = new UIButton(Rect(x, y, 200, 30));
    inputListenerDigitalSingleWithModifiersButton->SetStateFont(0xFF, font);
    inputListenerDigitalSingleWithModifiersButton->SetStateFontSize(0xFF, 11.f);
    inputListenerDigitalSingleWithModifiersButton->SetStateFontColor(0xFF, Color::White);
    inputListenerDigitalSingleWithModifiersButton->GetOrCreateComponent<UIDebugRenderComponent>();
    inputListenerDigitalSingleWithModifiersButton->SetStateText(0xFF, L"Listen: digital single with modifiers");
    inputListenerDigitalSingleWithModifiersButton->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &InputSystemTest::OnInputListenerButtonPressed));
    AddControl(inputListenerDigitalSingleWithModifiersButton);

    y += yDelta;

    inputListenerDigitalMultipleAnyButton = new UIButton(Rect(x, y, 200, 30));
    inputListenerDigitalMultipleAnyButton->SetStateFont(0xFF, font);
    inputListenerDigitalMultipleAnyButton->SetStateFontSize(0xFF, 11.f);
    inputListenerDigitalMultipleAnyButton->SetStateFontColor(0xFF, Color::White);
    inputListenerDigitalMultipleAnyButton->GetOrCreateComponent<UIDebugRenderComponent>();
    inputListenerDigitalMultipleAnyButton->SetStateText(0xFF, L"Listen: digital multiple any");
    inputListenerDigitalMultipleAnyButton->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &InputSystemTest::OnInputListenerButtonPressed));
    AddControl(inputListenerDigitalMultipleAnyButton);

    y += yDelta;

    inputListenerAnalogButton = new UIButton(Rect(x, y, 200, 30));
    inputListenerAnalogButton->SetStateFont(0xFF, font);
    inputListenerAnalogButton->SetStateFontSize(0xFF, 11.f);
    inputListenerAnalogButton->SetStateFontColor(0xFF, Color::White);
    inputListenerAnalogButton->GetOrCreateComponent<UIDebugRenderComponent>();
    inputListenerAnalogButton->SetStateText(0xFF, L"Listen: analog");
    inputListenerAnalogButton->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, Message(this, &InputSystemTest::OnInputListenerButtonPressed));
    AddControl(inputListenerAnalogButton);

    y += yDelta;

    inputListenerResultField = new UIStaticText(Rect(x, y, 200, 30));
    inputListenerResultField->SetTextColor(Color::White);
    inputListenerResultField->SetFont(font);
    inputListenerResultField->SetFontSize(11.f);
    inputListenerResultField->SetMultiline(true);
    inputListenerResultField->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    inputListenerResultField->SetUtf8Text("Listened input will be shown here");
    AddControl(inputListenerResultField);
}

void InputSystemTest::CreateKeyboardUIButton(eInputElements key, WideString text, FTFont* font, float32* x, float32 y, float32 w, float32 h)
{
    UIButton* button = new UIButton(Rect(*x, y, w, h));
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 8.f);
    button->SetStateFontColor(0xFF, Color::White);
    button->SetStateText(0xFF, text);
    button->GetOrCreateComponent<UIDebugRenderComponent>();

    keyboardButtons[static_cast<uint32>(key)] = button;
    AddControl(button);

    *x = *x + w + 1.0f;
}

void InputSystemTest::HighlightDigitalButton(DAVA::UIButton* button, DAVA::DigitalElementState state)
{
    if (button == nullptr)
    {
        return;
    }

    if (state.IsPressed())
    {
        button->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else
    {
        button->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color(1.0f, 0.0f, 0.0f, 1.0f));
    }
}

bool InputSystemTest::OnInputEvent(InputEvent const& event)
{
    if (event.deviceType == eInputDeviceTypes::KEYBOARD && event.keyboardEvent.charCode > 0)
    {
        return false;
    }

    Logger::Info("Input event, element: %s", GetInputElementInfo(event.elementId).name.c_str());

    if (event.deviceType == eInputDeviceTypes::KEYBOARD)
    {
        UIButton* scancodeButton = keyboardButtons[event.elementId];

        HighlightDigitalButton(scancodeButton, event.digitalState);
    }
    else if (event.deviceType == eInputDeviceTypes::MOUSE)
    {
        UIButton* mouseButton = mouseButtons[event.elementId];

        if (event.elementId == eInputElements::MOUSE_POSITION)
        {
            std::wstringstream ss;
            ss << "pos: " << static_cast<int32>(event.analogState.x) << L", " << static_cast<int32>(event.analogState.y);
            mouseButton->SetStateText(0xFF, ss.str());
        }
        else if (event.elementId == eInputElements::MOUSE_WHEEL)
        {
            std::wstringstream ss;
            ss << "wheel: " << event.analogState.x << L", " << event.analogState.y;
            mouseButton->SetStateText(0xFF, ss.str());
        }
        else
        {
            HighlightDigitalButton(mouseButton, event.digitalState);
        }
    }
    else if (event.deviceType == eInputDeviceTypes::TOUCH_SURFACE)
    {
        if (IsTouchClickInputElement(event.elementId))
        {
            UIButton* touchButton = touchClickButtons[event.elementId];
            HighlightDigitalButton(touchButton, event.digitalState);
        }

        // Position will be changed in Update
    }

    return false;
}

void InputSystemTest::OnAction(DAVA::Action action)
{
    UIStaticText* staticTextCounter = actionCounters[action.actionId];
    int counter = std::atoi(staticTextCounter->GetUtf8Text().c_str()) + 1;
    staticTextCounter->SetUtf8Text(std::to_string(counter));
}

void InputSystemTest::OnInputListenerButtonPressed(DAVA::BaseObject* sender, void* data, void* callerData)
{
    DAVA::eInputBindingListenerModes mode;
    if (sender == inputListenerDigitalSingleWithoutModifiersButton)
    {
        mode = DAVA::eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS;
    }
    else if (sender == inputListenerDigitalSingleWithModifiersButton)
    {
        mode = DAVA::eInputBindingListenerModes::DIGITAL_SINGLE_WITH_MODIFIERS;
    }
    else if (sender == inputListenerDigitalMultipleAnyButton)
    {
        mode = DAVA::eInputBindingListenerModes::DIGITAL_MULTIPLE_ANY;
    }
    else
    {
        mode = DAVA::eInputBindingListenerModes::ANALOG;
    }

    GetEngineContext()->inputListener->Listen(mode, MakeFunction(this, &InputSystemTest::OnInputListeningEnded));
    inputListenerResultField->SetUtf8Text("Listening...");
}

void InputSystemTest::OnInputListeningEnded(bool cancelled, DAVA::Vector<DAVA::InputEvent> input)
{
    if (cancelled)
    {
        inputListenerResultField->SetUtf8Text("Stopped listening");
    }
    else
    {
        // Combine input elements into a string

        std::stringstream stringStream;
        for (size_t i = 0; i < input.size(); ++i)
        {
            if (input[i].deviceType == eInputDeviceTypes::KEYBOARD)
            {
                String reprTranslated = GetEngineContext()->deviceManager->GetKeyboard()->TranslateElementToUTF8String(input[i].elementId);
                String reprDefault = GetInputElementInfo(input[i].elementId).name;
                stringStream << reprTranslated << "(" << reprDefault << ")";
            }
            else
            {
                String repr = GetInputElementInfo(input[i].elementId).name;
                stringStream << repr.c_str();
            }

            if (i != input.size() - 1)
            {
                stringStream << " + ";
            }
        }

        inputListenerResultField->SetUtf8Text(stringStream.str());
    }
}

void InputSystemTest::OnUpdate(float32 delta)
{
    TouchScreen* touch = GetEngineContext()->deviceManager->GetTouchScreen();

    if (touch != nullptr)
    {
        for (size_t i = 0; i < INPUT_ELEMENTS_TOUCH_POSITION_COUNT; ++i)
        {
            eInputElements elementId = static_cast<eInputElements>(eInputElements::TOUCH_FIRST_POSITION + i);

            UIButton* touchButton = touchMoveButtons[elementId];

            AnalogElementState state = touch->GetAnalogElementState(elementId);

            std::wstringstream ss;
            ss << static_cast<int>(state.x) << "\n" << static_cast<int>(state.y);
            touchButton->SetStateText(0xFF, ss.str());
        }
    }

    if (keyboard != nullptr && mouse != nullptr)
    {
        ActionSystem* actionSystem = GetEngineContext()->actionSystem;

        for (auto& actionStatus : digitalActionsStatus)
        {
            FastName actionId = actionStatus.first;
            UIStaticText* status = actionStatus.second;
            bool active = actionSystem->GetDigitalActionState(actionId);
            if (active)
            {
                status->SetUtf8Text("active");
            }
            else
            {
                status->SetUtf8Text("not active");
            }
        }

        for (auto& actionStatus : analogActionsStatus)
        {
            FastName actionId = actionStatus.first;
            UIStaticText* status = actionStatus.second;
            AnalogActionState state = actionSystem->GetAnalogActionState(actionId);
            int32 x = static_cast<int32>(state.x);
            int32 y = static_cast<int32>(state.y);
            String coords = "x: " + std::to_string(x) + ", y: " + std::to_string(y);
            if (state.active)
            {
                status->SetUtf8Text("active, " + coords);
            }
            else
            {
                status->SetUtf8Text("not active, " + coords);
            }
        }
    }
}
