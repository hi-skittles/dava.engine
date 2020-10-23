#include "Tests/WebViewTest.h"
#include "UI/Update/UIUpdateComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

const WideString htmlContent =
L"<html>"
L"<head></head>"
L"<body text='white'>"
L"Some test text"
L"</body>"
L"</html>";

WebViewTest::WebViewTest(TestBed& app)
    : BaseScreen(app, "WebViewTest")
    , webView(nullptr)
    , bgStubPanel(nullptr)
    , updateWait(false)
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void WebViewTest::LoadResources()
{
    BaseScreen::LoadResources();

    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));
    webView = new UIWebView(Rect(10, 10, 400, 200));
    webView->LoadHtmlString(htmlContent);
    webView->SetBackgroundTransparency(true);
    AddControl(webView);

    bgStubPanel = new UIControl(Rect(10, 10, 400, 200));
    UIControlBackground* background = bgStubPanel->GetOrCreateComponent<UIControlBackground>();
    background->SetDrawType(UIControlBackground::DRAW_FILL);
    background->SetColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
    background->SetColor(Color(1, 0, 0, 1));
    bgStubPanel->SetVisibilityFlag(false);
    AddControl(bgStubPanel);

    ScopedPtr<UIButton> visibleBtn(new UIButton(Rect(440, 10, 200, 50)));
    visibleBtn->SetStateFont(0xFF, font);
    visibleBtn->SetStateText(0xFF, L"Show/hide with sleep");
    visibleBtn->GetOrCreateComponent<UIDebugRenderComponent>();
    visibleBtn->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &WebViewTest::OnVisibleClick));
    AddControl(visibleBtn);
}

void WebViewTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    SafeRelease(webView);
    SafeRelease(bgStubPanel);
}

void WebViewTest::Update(float32 delta)
{
    // Simulate time lag between screens or tabs with loading resources
    if (updateWait)
    {
        Thread::Sleep(500);
        updateWait = false;
    }

    BaseScreen::Update(delta);
}

void WebViewTest::OnVisibleClick(BaseObject* sender, void* data, void* callerData)
{
    webView->SetVisibilityFlag(!webView->GetVisibilityFlag());
    bgStubPanel->SetVisibilityFlag(!webView->GetVisibilityFlag());

    updateWait = true;
}
