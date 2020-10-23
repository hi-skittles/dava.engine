#include "Tests/UILoggingTest.h"
#include "Analytics/Analytics.h"
#include "Analytics/LoggingBackend.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "Engine/Engine.h"

using namespace DAVA;

Analytics::Core& GetCore()
{
    return *GetEngineContext()->analyticsCore;
}

UILoggingTest::UILoggingTest(TestBed& app)
    : BaseScreen(app, "UI_Logging_test")
{
    auto backend = std::make_unique<Analytics::LoggingBackend>("~doc:/AnalyticsLog.txt");
    GetCore().AddBackend("LoggingBackend", std::move(backend));
}

void UILoggingTest::LoadResources()
{
    BaseScreen::LoadResources();

    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    Rect btnRect(10.0f, 10.0f, 200.0f, 40.0f);
    switchButton.Set(CreateUIButton(font, btnRect, &UILoggingTest::OnSwitch));
    AddControl(switchButton.Get());
    UpdateSwithButton();

    RefPtr<KeyedArchive> config(new KeyedArchive);
    config->LoadFromYamlFile("~res:/TestBed/AnalyticsConfig.yaml");
    GetCore().SetConfig(config.Get());
}

void UILoggingTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    switchButton.Set(nullptr);
}

UIButton* UILoggingTest::CreateUIButton(Font* font, const Rect& rect,
                                        void (UILoggingTest::*onClick)(BaseObject*, void*, void*))
{
    using namespace DAVA;

    UIButton* button = new UIButton(rect);

    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 12.f);
    button->SetStateFontColor(0xFF, Color::White);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, onClick));

    return button;
}

void UILoggingTest::OnSwitch(DAVA::BaseObject* obj, void* data, void* callerData)
{
    Analytics::Core& core = GetCore();

    if (core.IsStarted())
    {
        core.Stop();
    }
    else
    {
        core.Start();
    }

    UpdateSwithButton();
}

void UILoggingTest::UpdateSwithButton()
{
    Analytics::Core& core = GetCore();
    WideString text = core.IsStarted() ? L"Stop UI Logging" : L"Start UI Logging";
    switchButton->SetStateText(0xFF, text);
}
