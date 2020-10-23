#include "Tests/SoundTest.h"

#include "Engine/Engine.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

SoundTest::SoundTest(TestBed& app)
    : BaseScreen(app, "Sound_test")
{
}

void SoundTest::LoadResources()
{
    BaseScreen::LoadResources();

    eventGroup1 = SoundSystem::Instance()->CreateSoundEventFromFile("~res:/TestBed/Sounds/map.ogg", FastName("group-1"), SoundEvent::SOUND_EVENT_CREATE_LOOP);
    eventGroup2 = SoundSystem::Instance()->CreateSoundEventFromFile("~res:/TestBed/Sounds/map.ogg", FastName("group-2"));

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    // Group 1

    UIStaticText* infoTextGroup1 = new UIStaticText(Rect(20, 20, 250, 50));
    infoTextGroup1->SetTextColor(Color::White);
    infoTextGroup1->SetFont(font);
    infoTextGroup1->SetFontSize(14.f);
    infoTextGroup1->SetMultiline(true);
    infoTextGroup1->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    infoTextGroup1->SetText(L"Group 1");
    AddControl(infoTextGroup1);
    SafeRelease(infoTextGroup1);

    UIButton* playSoundButtonGroup1 = new UIButton(Rect(20, 60, 250, 50));
    playSoundButtonGroup1->SetStateFont(0xFF, font);
    playSoundButtonGroup1->SetStateFontSize(0xFF, 14.f);
    playSoundButtonGroup1->SetStateFontColor(0xFF, Color::White);
    playSoundButtonGroup1->SetStateText(0xFF, L"Play sound");
    playSoundButtonGroup1->GetOrCreateComponent<UIDebugRenderComponent>();
    playSoundButtonGroup1->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &SoundTest::OnPlaySoundGroup1));
    AddControl(playSoundButtonGroup1);
    SafeRelease(playSoundButtonGroup1);

    UIButton* buttonSetSpeedGroup1 = new UIButton(Rect(20, 120, 200, 50));
    buttonSetSpeedGroup1->SetStateFont(0xFF, font);
    buttonSetSpeedGroup1->SetStateFontSize(0xFF, 14.f);
    buttonSetSpeedGroup1->SetStateFontColor(0xFF, Color::White);
    buttonSetSpeedGroup1->SetStateText(0xFF, L"Set speed");
    buttonSetSpeedGroup1->GetOrCreateComponent<UIDebugRenderComponent>();
    buttonSetSpeedGroup1->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &SoundTest::OnApplySpeedGroup1));
    AddControl(buttonSetSpeedGroup1);
    SafeRelease(buttonSetSpeedGroup1);

    speedTextFieldGroup1 = new UITextField(Rect(225, 120, 45, 50));
    speedTextFieldGroup1->GetOrCreateComponent<UIFocusComponent>();
    speedTextFieldGroup1->SetFont(font);
    speedTextFieldGroup1->SetFontSize(14.f);
    speedTextFieldGroup1->GetOrCreateComponent<UIDebugRenderComponent>();
    speedTextFieldGroup1->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    speedTextFieldGroup1->SetText(L"1.0");
    AddControl(speedTextFieldGroup1);

    // Group 2

    UIStaticText* infoTextGroup2 = new UIStaticText(Rect(320, 20, 250, 50));
    infoTextGroup2->SetTextColor(Color::White);
    infoTextGroup2->SetFont(font);
    infoTextGroup2->SetFontSize(14.f);
    infoTextGroup2->SetMultiline(true);
    infoTextGroup2->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    infoTextGroup2->SetText(L"Group 2");
    AddControl(infoTextGroup2);
    SafeRelease(infoTextGroup2);

    UIButton* playSoundButtonGroup2 = new UIButton(Rect(320, 60, 250, 50));
    playSoundButtonGroup2->SetStateFont(0xFF, font);
    playSoundButtonGroup2->SetStateFontSize(0xFF, 14.f);
    playSoundButtonGroup2->SetStateFontColor(0xFF, Color::White);
    playSoundButtonGroup2->SetStateText(0xFF, L"Play sound");
    playSoundButtonGroup2->GetOrCreateComponent<UIDebugRenderComponent>();
    playSoundButtonGroup2->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &SoundTest::OnPlaySoundGroup2));
    AddControl(playSoundButtonGroup2);
    SafeRelease(playSoundButtonGroup2);

    UIButton* buttonSetSpeedGroup2 = new UIButton(Rect(320, 120, 200, 50));
    buttonSetSpeedGroup2->SetStateFont(0xFF, font);
    buttonSetSpeedGroup2->SetStateFontSize(0xFF, 14.f);
    buttonSetSpeedGroup2->SetStateFontColor(0xFF, Color::White);
    buttonSetSpeedGroup2->SetStateText(0xFF, L"Set speed");
    buttonSetSpeedGroup2->GetOrCreateComponent<UIDebugRenderComponent>();
    buttonSetSpeedGroup2->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &SoundTest::OnApplySpeedGroup2));
    AddControl(buttonSetSpeedGroup2);
    SafeRelease(buttonSetSpeedGroup2);

    speedTextFieldGroup2 = new UITextField(Rect(525, 120, 45, 50));
    speedTextFieldGroup2->GetOrCreateComponent<UIFocusComponent>();
    speedTextFieldGroup2->SetFont(font);
    speedTextFieldGroup2->SetFontSize(14.f);
    speedTextFieldGroup2->GetOrCreateComponent<UIDebugRenderComponent>();
    speedTextFieldGroup2->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    speedTextFieldGroup2->SetText(L"1.0");
    AddControl(speedTextFieldGroup2);

    Window* primaryWindow = GetPrimaryWindow();
    primaryWindow->focusChanged.Connect(this, &SoundTest::OnWindowFocusChanged);
}

void SoundTest::UnloadResources()
{
    RemoveAllControls();

    if (eventGroup1 != nullptr)
    {
        eventGroup1->Stop(true);
        eventGroup1->Release();
    }
    if (eventGroup2 != nullptr)
    {
        eventGroup2->Stop(true);
        eventGroup2->Release();
    }

    BaseScreen::UnloadResources();

    Window* primaryWindow = GetPrimaryWindow();
    if (primaryWindow != nullptr)
    {
        primaryWindow->focusChanged.Disconnect(this);
    }
}

void SoundTest::OnWindowFocusChanged(DAVA::Window* w, bool hasFocus)
{
    eventGroup1->SetPaused(!hasFocus);
    eventGroup2->SetPaused(!hasFocus);
}

void SoundTest::OnPlaySoundGroup1(BaseObject* sender, void* data, void* callerData)
{
    eventGroup1->Trigger();
}

void SoundTest::OnApplySpeedGroup1(BaseObject* sender, void* data, void* callerData)
{
    float speed = static_cast<float>(std::atof(UTF8Utils::EncodeToUTF8(speedTextFieldGroup1->GetText()).c_str()));
    SoundSystem::Instance()->SetGroupSpeed(FastName("group-1"), speed);
}

void SoundTest::OnPlaySoundGroup2(BaseObject* sender, void* data, void* callerData)
{
    eventGroup2->Trigger();
}

void SoundTest::OnApplySpeedGroup2(BaseObject* sender, void* data, void* callerData)
{
    float speed = static_cast<float>(std::atof(UTF8Utils::EncodeToUTF8(speedTextFieldGroup2->GetText()).c_str()));
    SoundSystem::Instance()->SetGroupSpeed(FastName("group-2"), speed);
}
