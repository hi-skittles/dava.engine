#include "Tests/MicroWebBrowserTest.h"
#include <UI/Focus/UIFocusComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <UI/Update/UIUpdateComponent.h>
#include <MemoryManager/MemoryManager.h>

MicroWebBrowserTest::MicroWebBrowserTest(TestBed& app)
    : BaseScreen(app, "MicroWebBrowserTest")
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void MicroWebBrowserTest::LoadResources()
{
    ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));
    DVASSERT(font);

    Rect screenRect = GetRect();

    Rect textFieldRect;
    textFieldRect.x = 10.0f;
    textFieldRect.y = 10.0f;
    textFieldRect.dx = screenRect.dx - 230.0f;
    textFieldRect.dy = 50.0f;

    textField.Set(new UITextField(textFieldRect));
    textField->SetFont(font);
    textField->SetFontSize(14.f);
    textField->GetOrCreateComponent<UIDebugRenderComponent>();
    textField->SetTextColor(Color(0.0, 1.0, 0.0, 1.0));
    textField->GetOrCreateComponent<UIFocusComponent>();
    textField->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    textField->SetDelegate(this);
    AddControl(textField.Get());

    Rect loadPageRect = textFieldRect;
    loadPageRect.x += textFieldRect.dx + 10.0f;
    loadPageRect.dx = 200.0f;

    ScopedPtr<UIButton> loadPage(new UIButton(loadPageRect));
    loadPage->SetStateFont(0xFF, font);
    loadPage->SetStateFontSize(0xFF, 14.f);
    loadPage->SetStateFontColor(0xFF, Color::White);
    loadPage->SetStateText(0xFF, L"Load");
    loadPage->GetOrCreateComponent<UIDebugRenderComponent>();
    loadPage->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &MicroWebBrowserTest::OnLoadPage));
    AddControl(loadPage);

    Rect webViewRect;
    webViewRect.x = textFieldRect.x;
    webViewRect.y = textFieldRect.y + textFieldRect.dy + 5.0f;
    webViewRect.dx = (loadPageRect.x + loadPageRect.dx) - webViewRect.x;
    webViewRect.dy = screenRect.dy * 0.8f;

    webView.Set(new UIWebView(webViewRect));
    webView->SetVisibilityFlag(true);
    webView->SetRenderToTexture(true);
    webView->GetOrCreateComponent<UIDebugRenderComponent>();
    webView->SetInputEnabled(true);
    webView->GetOrCreateComponent<UIFocusComponent>();
    AddControl(webView.Get());

    fpsText = new UIStaticText(Rect(10.0f, screenRect.dy - 50.0f, 100.0f, 45.0f));
    fpsText->SetTextColor(Color::White);
    fpsText->SetFont(font);
    fpsText->SetFontSize(14.f);
    fpsText->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    fpsText->SetUtf8Text("FPS: ");
    AddControl(fpsText);
    
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memoryText = new UIStaticText(Rect(110.0f, screenRect.dy - 50.0f, 200.0f, 45.0f));
    memoryText->SetTextColor(Color::White);
    memoryText->SetFont(font);
    memoryText->SetFontSize(14.f);
    memoryText->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    memoryText->SetUtf8Text("Memory usage: ");
    AddControl(memoryText);
#endif

    BaseScreen::LoadResources();
}

void MicroWebBrowserTest::UnloadResources()
{
    webView = nullptr;
    textField = nullptr;

    SafeRelease(fpsText);
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    SafeRelease(memoryText);
#endif

    BaseScreen::UnloadResources();
}

void MicroWebBrowserTest::OnLoadPage(BaseObject* obj, void* data, void* callerData)
{
    WideString url = textField->GetText();
    if (!url.empty())
    {
        webView->OpenURL(UTF8Utils::EncodeToUTF8(url));
    }
}

void MicroWebBrowserTest::TextFieldShouldReturn(UITextField* /*textField*/)
{
    OnLoadPage(nullptr, nullptr, nullptr);
}

void MicroWebBrowserTest::Update(float elapsedTime)
{
    fpsMeter.Update(elapsedTime);
    if (fpsMeter.IsFpsReady())
    {
        fpsText->SetUtf8Text(Format("FPS: %u", static_cast<uint32>(fpsMeter.GetFps())));
    }
    
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    float32 memoryUsageMb = static_cast<float32>(MemoryManager::Instance()->GetSystemMemoryUsage()) / (1024 * 1024);
    memoryText->SetUtf8Text(Format("Memory usage: %.2f MB", memoryUsageMb));
#endif
}
