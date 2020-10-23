#include "Tests/NotificationTest.h"
#include "Base/Message.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/Update/UIUpdateComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

#include "Engine/Engine.h"

using namespace DAVA;

NotificationScreen::NotificationScreen(TestBed& app)
    : BaseScreen(app, "NotificationScreen")
    , showNotificationText(nullptr)
    , showNotificationTextDelayed(nullptr)
    , cancelDelayedNotifications(nullptr)
    , showNotificationProgress(nullptr)
    , hideNotificationProgress(nullptr)
    , notificationProgress(nullptr)
    , notificationText(nullptr)
    , progress(0)
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void NotificationScreen::LoadResources()
{
    notificationController = GetEngineContext()->localNotificationController;

    BaseScreen::LoadResources();
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    showNotificationText = new UIButton(Rect(10, 10, 450, 60));
    showNotificationText->SetStateFont(0xFF, font);
    showNotificationText->SetStateFontSize(0xFF, 30.f);
    showNotificationText->SetStateFontColor(0xFF, Color::White);
    showNotificationText->SetStateText(0xFF, L"Notify text");

    showNotificationText->GetOrCreateComponent<UIDebugRenderComponent>();
    showNotificationText->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnNotifyText));
    AddControl(showNotificationText);

    showNotificationTextDelayed = new UIButton(Rect(10, 100, 400, 60));
    showNotificationTextDelayed->SetStateFont(0xFF, font);
    showNotificationTextDelayed->SetStateFontSize(0xFF, 30.f);
    showNotificationTextDelayed->SetStateFontColor(0xFF, Color::White);
    showNotificationTextDelayed->SetStateText(0xFF, L"Notify text after X seconds");

    showNotificationTextDelayed->GetOrCreateComponent<UIDebugRenderComponent>();
    showNotificationTextDelayed->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnNotifyTextDelayed));
    AddControl(showNotificationTextDelayed);

    cancelDelayedNotifications = new UIButton(Rect(10, 200, 450, 60));
    cancelDelayedNotifications->SetStateFont(0xFF, font);
    cancelDelayedNotifications->SetStateFontSize(0xFF, 30.f);
    cancelDelayedNotifications->SetStateFontColor(0xFF, Color::White);
    cancelDelayedNotifications->SetStateText(0xFF, L"Cancel all delayed notifications");

    cancelDelayedNotifications->GetOrCreateComponent<UIDebugRenderComponent>();
    cancelDelayedNotifications->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnNotifyCancelDelayed));
    AddControl(cancelDelayedNotifications);

    hideNotificationText = new UIButton(Rect(10, 300, 450, 60));
    hideNotificationText->SetStateFont(0xFF, font);
    hideNotificationText->SetStateFontSize(0xFF, 30.f);
    hideNotificationText->SetStateFontColor(0xFF, Color::White);
    hideNotificationText->SetStateText(0xFF, L"Hide text");

    hideNotificationText->GetOrCreateComponent<UIDebugRenderComponent>();
    hideNotificationText->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnHideText));
    AddControl(hideNotificationText);

    showNotificationProgress = new UIButton(Rect(500, 10, 450, 60));
    showNotificationProgress->SetStateFont(0xFF, font);
    showNotificationProgress->SetStateFontSize(0xFF, 30.f);
    showNotificationProgress->SetStateFontColor(0xFF, Color::White);
    showNotificationProgress->SetStateText(0xFF, L"Notify progress");

    showNotificationProgress->GetOrCreateComponent<UIDebugRenderComponent>();
    showNotificationProgress->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnNotifyProgress));
    AddControl(showNotificationProgress);

    hideNotificationProgress = new UIButton(Rect(500, 100, 450, 60));
    hideNotificationProgress->SetStateFont(0xFF, font);
    hideNotificationProgress->SetStateFontSize(0xFF, 30.f);
    hideNotificationProgress->SetStateFontColor(0xFF, Color::White);
    hideNotificationProgress->SetStateText(0xFF, L"Hide progress");

    hideNotificationProgress->GetOrCreateComponent<UIDebugRenderComponent>();
    hideNotificationProgress->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnHideProgress));
    AddControl(hideNotificationProgress);

    notificationStressTest = new UIButton(Rect(500, 300, 450, 60));
    notificationStressTest->SetStateFont(0xFF, font);
    notificationStressTest->SetStateFontSize(0xFF, 30.f);
    notificationStressTest->SetStateFontColor(0xFF, Color::White);
    notificationStressTest->SetStateText(0xFF, L"Stress test (30 s)");

    notificationStressTest->GetOrCreateComponent<UIDebugRenderComponent>();
    notificationStressTest->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &NotificationScreen::OnNotificationStressTest));
    AddControl(notificationStressTest);

    activateFromNotification = new UIStaticText(Rect(10, 400, 450, 60));
    activateFromNotification->SetTextColor(Color::White);
    activateFromNotification->SetFont(font);
    activateFromNotification->SetFontSize(30.f);
    activateFromNotification->SetMultiline(true);
    activateFromNotification->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(activateFromNotification);

    notificationDelayTextField = new UITextField(Rect(420, 100, 35, 60));
    notificationDelayTextField->GetOrCreateComponent<UIFocusComponent>();
    notificationDelayTextField->SetFont(font);
    notificationDelayTextField->SetFontSize(30.f);
    notificationDelayTextField->GetOrCreateComponent<UIDebugRenderComponent>();
    notificationDelayTextField->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    notificationDelayTextField->SetText(L"5");
    AddControl(notificationDelayTextField);

    notificationController->RequestPermissions();

    SafeRelease(font);

    Engine::Instance()->backgroundUpdate.Connect(this, &NotificationScreen::UpdateNotification);
}

void NotificationScreen::UnloadResources()
{
    Engine::Instance()->backgroundUpdate.Disconnect(this);

    notificationController->Remove(notificationProgress);
    notificationProgress = nullptr;

    notificationController->Remove(notificationText);
    notificationText = nullptr;

    BaseScreen::UnloadResources();

    RemoveAllControls();
    SafeRelease(activateFromNotification);
    SafeRelease(showNotificationText);
    SafeRelease(showNotificationProgress);
    SafeRelease(hideNotificationProgress);
    SafeRelease(notificationDelayTextField);
    SafeRelease(notificationStressTest);
}

void NotificationScreen::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    UpdateNotification(timeElapsed);
}

void NotificationScreen::Draw(const UIGeometricData& geometricData)
{
}

void NotificationScreen::UpdateNotification(float32 timeElapsed)
{
    if (nullptr == notificationProgress)
        return;

    static float32 timeCounter = 0;
    timeCounter += timeElapsed;

    if (0.25 <= timeCounter)
    {
        timeCounter = 0;

        if (100 == progress)
        {
            progress = 0;
        }

        notificationProgress->SetProgressCurrent(progress++);
    }
}

void NotificationScreen::OnNotifyText(BaseObject* obj, void* data, void* callerData)
{
    if (nullptr == notificationText)
    {
        notificationText = notificationController->CreateNotificationText();
        notificationText->Update();

        notificationText->SetAction(Message(this, &NotificationScreen::OnNotificationTextPressed));
    }
    else
    {
        notificationText->Show();
    }

    notificationText->SetTitle("Application is on foreground!");
    notificationText->SetText("This text appeared at button press ");

    hideNotificationText->GetOrCreateComponent<UIDebugRenderComponent>();
}

void NotificationScreen::OnNotifyTextDelayed(BaseObject* obj, void* data, void* callerData)
{
    int delayInSeconds = std::atoi(UTF8Utils::EncodeToUTF8(notificationDelayTextField->GetText()).c_str());
    notificationController->PostDelayedNotification("Test Delayed notification Title", "Some text", delayInSeconds);
}

void NotificationScreen::OnNotifyCancelDelayed(BaseObject* obj, void* data, void* callerData)
{
    notificationController->RemoveAllDelayedNotifications();
}

void NotificationScreen::OnHideText(BaseObject* obj, void* data, void* callerData)
{
    if (notificationText && notificationText->IsVisible())
    {
        notificationText->Hide();
        hideNotificationText->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(false);
    }
    activateFromNotification->SetText(L"");
}

void NotificationScreen::OnNotifyProgress(BaseObject* obj, void* data, void* callerData)
{
    if (nullptr == notificationProgress)
    {
        notificationProgress = notificationController->CreateNotificationProgress("", "", 100, 0);
        notificationProgress->SetAction(Message(this, &NotificationScreen::OnNotificationProgressPressed));
    }
    else
    {
        notificationProgress->Show();
    }

    notificationProgress->SetTitle("Fake Download Progress");
    notificationProgress->SetText("You pressed the button");

    hideNotificationProgress->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(true);
}

void NotificationScreen::OnHideProgress(BaseObject* obj, void* data, void* callerData)
{
    if (notificationProgress && notificationProgress->IsVisible())
    {
        notificationProgress->Hide();
        hideNotificationProgress->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(false);
    }
}

void NotificationScreen::OnNotificationTextPressed(BaseObject* obj, void* data, void* callerData)
{
    activateFromNotification->SetText(L"Application activate after NotificationTextPressed\n Press Hide text button");
}

void NotificationScreen::OnNotificationProgressPressed(BaseObject* obj, void* data, void* callerData)
{
}

void NotificationScreen::OnNotificationStressTest(BaseObject* obj, void* data, void* callerData)
{
    Logger::Info("Starting notification stress test");
    for (int i = 0; i < 550; ++i)
    {
        notificationController->RemoveAllDelayedNotifications();
        notificationController->PostDelayedNotification("Stress test", "Notification stress test", 30);
    }
    Logger::Info("Notification stress test finished");
}
