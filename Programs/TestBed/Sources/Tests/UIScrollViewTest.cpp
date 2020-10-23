#include "Tests/UIScrollViewTest.h"

#include "Render/2D/Sprite.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "UI/UIControlBackground.h"

using namespace DAVA;

UIScrollViewTest::UIScrollViewTest(TestBed& app)
    : BaseScreen(app, "UIScrollViewTest")
{
}

void UIScrollViewTest::LoadResources()
{
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    DefaultUIPackageBuilder builder;
    UIPackageLoader().LoadPackage("~res:/TestBed/UI/Test/ScrollScreen.yaml", &builder);
    AddControl(builder.GetPackage()->GetControl("ScreenContent"));
    scrollView = DynamicTypeCheck<UIScrollView*>(FindByName("Scrollview"));

    UIControl* innerControl = FindByName("UIControl1");
    if (innerControl)
    {
        innerControl->GetComponent<UIControlBackground>()->SetSprite("~res:/TestBed/Gfx/UI/HorizontalScroll", 0);
        innerControl->GetComponent<UIControlBackground>()->SetDrawType(UIControlBackground::DRAW_SCALE_TO_RECT);
    }

    UIControl* control = FindByName("HorizontalScrollbar");
    if (control)
    {
        UIScrollBar* horizontalScrollbar = DynamicTypeCheck<UIScrollBar*>(control);
        ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile("~res:/TestBed/TestData/UI/HorizontalScroll.png"));

        UIControlBackground* sliderBg = horizontalScrollbar->GetSlider()->GetOrCreateComponent<UIControlBackground>();
        sliderBg->SetSprite(sprite, 0);
        sliderBg->SetDrawType(UIControlBackground::DRAW_STRETCH_HORIZONTAL);
        sliderBg->SetLeftRightStretchCap(10);
        horizontalScrollbar->SetOrientation(UIScrollBar::ORIENTATION_HORIZONTAL);
    }

    control = FindByName("VerticalScrollbar");
    if (control)
    {
        UIScrollBar* verticalScrollbar = DynamicTypeCheck<UIScrollBar*>(control);
        ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile("~res:/TestBed/TestData/UI/VerticalScroll.png"));
        UIControlBackground* sliderBg = verticalScrollbar->GetSlider()->GetOrCreateComponent<UIControlBackground>();
        sliderBg->SetSprite(sprite, 0);
        sliderBg->SetDrawType(UIControlBackground::DRAW_STRETCH_VERTICAL);
        sliderBg->SetTopBottomStretchCap(10);
        verticalScrollbar->SetOrientation(UIScrollBar::ORIENTATION_VERTICAL);
    }

    UIControl* testControl4 = new UIControl(Rect(1200.f, 1400.f, 250.f, 250.f));
    UIControlBackground* testControl4Bg = testControl4->GetOrCreateComponent<UIControlBackground>();
    testControl4->GetOrCreateComponent<UIDebugRenderComponent>();
    testControl4Bg->SetColor(Color(0.3333f, 0.3333f, 0.5555f, 1.0000f));
    testControl4Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl4->SetName("CONTROL_4");
    testControl4->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UIScrollViewTest::ButtonPressed));
    scrollView->AddControlToContainer(testControl4);
    SafeRelease(testControl4);

    UIControl* testControlChild = new UIControl(Rect(100.f, 100.f, 150.f, 150.f));
    UIControlBackground* testControlChildBg = testControlChild->GetOrCreateComponent<UIControlBackground>();
    testControlChild->GetOrCreateComponent<UIDebugRenderComponent>();
    testControlChildBg->SetColor(Color(0.3333f, 0.3333f, 0.5555f, 1.0000f));
    testControlChildBg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControlChild->SetName("CONTROL_3");
    testControlChild->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UIScrollViewTest::ButtonPressed));

    UIControl* testControl = new UIControl(Rect(50.f, 0.f, 150.f, 150.f));
    UIControlBackground* testControlBg = testControl->GetOrCreateComponent<UIControlBackground>();
    testControl->GetOrCreateComponent<UIDebugRenderComponent>();
    testControlBg->SetColor(Color(0.3333f, 0.6667f, 0.4980f, 1.0000f));
    testControlBg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl->SetName("CONTROL_2");
    testControl->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UIScrollViewTest::ButtonPressed));
    testControl->AddControl(testControlChild);

    UIButton* testButton = new UIButton(Rect(10.f, 50.f, 250.f, 100.f));
    testButton->CreateBackgroundForState(STATE_NORMAL);
    testButton->GetOrCreateComponent<UIDebugRenderComponent>();
    testButton->SetStateFont(STATE_NORMAL, font);
    testButton->SetStateFontSize(STATE_NORMAL, 14.f);
    testButton->SetStateFontColor(STATE_NORMAL, Color::White);
    testButton->SetStateText(STATE_NORMAL, L"First button");
    testButton->GetBackground()->SetColor(Color(0.6667f, 0.6667f, 0.4980f, 1.0000f));
    testButton->GetBackground()->SetDrawType(UIControlBackground::DRAW_FILL);
    testButton->SetName("CONTROL_1");
    testButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UIScrollViewTest::ButtonPressed));
    testButton->AddControl(testControl);

    scrollView->AddControlToContainer(testButton);
    // Check how controls with negative coodinates are pushed inside scrollContainer
    UIControl* testControl5 = new UIControl(Rect(-50.f, 500.f, 150.f, 150.f));
    UIControlBackground* testControl5Bg = testControl5->GetOrCreateComponent<UIControlBackground>();
    testControl5->GetOrCreateComponent<UIDebugRenderComponent>();
    testControl5Bg->SetColor(Color(0.3333f, 0.3333f, 0.5555f, 1.0000f));
    testControl5Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl5->SetName("CONTROL_5");

    UIControl* testControl6 = new UIControl(Rect(-50.f, -50.f, 25.f, 25.f));
    UIControlBackground* testControl6Bg = testControl6->GetOrCreateComponent<UIControlBackground>();
    testControl6->GetOrCreateComponent<UIDebugRenderComponent>();
    testControl6Bg->SetColor(Color(0.3333f, 0.6667f, 0.4980f, 1.0000f));
    testControl6Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl6->SetName("CONTROL_6");

    testControl5->AddControl(testControl6);

    UIControl* testControl7 = new UIControl(Rect(-100.f, 15.f, 35.f, 35.f));
    UIControlBackground* testControl7Bg = testControl7->GetOrCreateComponent<UIControlBackground>();
    testControl7->GetOrCreateComponent<UIDebugRenderComponent>();
    testControl7Bg->SetColor(Color(0.6667f, 0.3333f, 0.7880f, 1.0000f));
    testControl7Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl7->SetName("CONTROL_7");

    testControl6->AddControl(testControl7);
    SafeRelease(testControl6);
    SafeRelease(testControl7);

    UIControl* testControl8 = new UIControl(Rect(-70.f, 50.f, 25.f, 25.f));
    UIControlBackground* testControl8Bg = testControl8->GetOrCreateComponent<UIControlBackground>();
    testControl8->GetOrCreateComponent<UIDebugRenderComponent>();
    testControl8Bg->SetColor(Color(0.6667f, 0.3333f, 0.4980f, 1.0000f));
    testControl8Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    testControl8->SetName("CONTROL_8");

    testControl5->AddControl(testControl8);
    SafeRelease(testControl8);

    scrollView->AddControlToContainer(testControl5);
    scrollView->RecalculateContentSize();
    SafeRelease(testControl5);

    SafeRelease(testControlChild);
    SafeRelease(testControl);
    SafeRelease(testButton);

    testMessageText = new UIStaticText(Rect(10, 10, 300, 30));
    UIControlBackground* testMessageTextBg = testMessageText->GetOrCreateComponent<UIControlBackground>();
    testMessageText->SetFont(font);
    testMessageText->SetFontSize(14.f);
    testMessageText->SetTextColor(Color(0.0, 1.0, 0.0, 1.0));
    testMessageTextBg->SetColor(Color(0.5, 0.0, 0.25, 1.0));
    testMessageTextBg->SetDrawType(UIControlBackground::DRAW_FILL);
    AddControl(testMessageText);

    finishTestBtn = new UIButton(Rect(10, 310, 300, 30));
    finishTestBtn->SetStateFont(0xFF, font);
    finishTestBtn->SetStateFontSize(0xFF, 14.f);
    finishTestBtn->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    finishTestBtn->SetStateText(0xFF, L"Finish test");

    finishTestBtn->GetOrCreateComponent<UIDebugRenderComponent>();
    finishTestBtn->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &UIScrollViewTest::ButtonPressed));
    AddControl(finishTestBtn);

    SafeRelease(font);

    BaseScreen::LoadResources();
}

void UIScrollViewTest::UnloadResources()
{
    RemoveAllControls();
    SafeRelease(finishTestBtn);
    SafeRelease(testMessageText);

    BaseScreen::UnloadResources();
}

void UIScrollViewTest::ButtonPressed(BaseObject* obj, void* data, void* callerData)
{
    UIControl* control = dynamic_cast<UIControl*>(obj);
    if (control)
    {
        String msg = Format("Tap on control - %s", control->GetName().c_str());
        testMessageText->SetText(UTF8Utils::EncodeToWideString(msg));
    }
    else
    {
        testMessageText->SetText(L"");
    }
}
