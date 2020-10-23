#include "Tests/StaticWebViewTest.h"

#include "Render/2D/Sprite.h"
#include "UI/UIControlSystem.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/IUIWebViewDelegate.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "Utils/Utils.h"

namespace
{
const String htmlCuteCats =
"<html>"
"  <head>"
"    <link rel='stylesheet' type='text/css' href='test.css'>"
"    <script type = 'text/javascript'>"
"      function doSomething() {"
"          var x = document.getElementById('mydiv');"
"          x.innerHTML += '<h3>Hello from JS</h3>';"
"      }"
"      function pop_it_up(url) {"
"          newwindow=window.open(url,'name','height=200,width=150');"
"          if (window.focus) { newwindow.focus() }"
"          return false;"
"      }"
"    </script>"
"  </head>"
"  <body text='red'>"
"      <a href = \"\" onclick = \"return pop_it_up('')\">Goto empty URL</a>"
"      <h1>Cute cats picture</h1><br/>"
"      <br/>"
"      <img src='cute-cat-picture.jpg'/>"
"      <div id='mydiv'>"
"      </div>"
"  </body>"
"</html>";

const WideString htmlString =
L"<html>"
L"  <head>"
L"  </head>"
L"  <body style='color: #d0e4fe'>"
L"      <h1>This is a WebView</h1>"
L"      <a href='http://www.turion.by'>click me</a><br/>"
L"      <a href='https://wargaming.net'>click me</a><br/>"
L"      <a href='wotblitz://come.here'>custom scheme</a><br/>"
L"  </body>"
L"</html>";

} // unnamed namespace

class MyWebViewDelegate : public IUIWebViewDelegate
{
public:
    eAction URLChanged(UIWebView* webview, const String& newURL, bool isRedirectedByMouseClick) override
    {
        Logger::Debug("MyWebViewDelegate::URLChanged %s: %s", isRedirectedByMouseClick ? "by user" : "by code", newURL.c_str());
        if (newURL.find("wotblitz://") == 0)
            return eAction::NO_PROCESS;
        return eAction::PROCESS_IN_WEBVIEW;
    }

    void OnExecuteJScript(UIWebView* webview, const String& result) override
    {
        Logger::Debug("MyWebViewDelegate::OnExecuteJScript: %s", result.c_str());
    }

    void PageLoaded(UIWebView* webview) override
    {
        Logger::Debug("MyWebViewDelegate::PageLoaded");
    }

    void SwipeGesture(bool left) override
    {
    }
};

StaticWebViewTest::StaticWebViewTest(TestBed& app)
    : BaseScreen(app, "StaticWebViewTest")
{
}

void StaticWebViewTest::LoadResources()
{
    webviewDelegate = new MyWebViewDelegate;

    webView1 = new UIWebView(Rect(5, 5, 400, 300));
    webView1->SetVisibilityFlag(true);
    webView1->SetRenderToTexture(true);
    webView1->GetOrCreateComponent<UIDebugRenderComponent>();
    webView1->SetDelegate(webviewDelegate);
    webView1->OpenURL("http://en.cppreference.com/");
    webView1->GetOrCreateComponent<UIFocusComponent>();
    AddControl(webView1);

    webView2 = new UIWebView(Rect(410, 50, 400, 300));
    webView2->SetVisibilityFlag(true);
    webView2->GetOrCreateComponent<UIDebugRenderComponent>();
    webView2->OpenFromBuffer(htmlCuteCats, "~res:/TestBed/TestData/TransparentWebViewTest/");
    webView2->GetOrCreateComponent<UIFocusComponent>();
    AddControl(webView2);

    webView3 = new UIWebView(Rect(820, 70, 400, 300));
    webView3->SetVisibilityFlag(true);
    webView3->SetRenderToTexture(true);
    webView3->GetOrCreateComponent<UIDebugRenderComponent>();
    webView3->SetDelegate(webviewDelegate);
    webView3->LoadHtmlString(htmlString);
    webView3->GetOrCreateComponent<UIFocusComponent>();
    AddControl(webView3);

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    const float32 w = 40;

    overlapedImage = new UIControl(Rect(500, 0, 300, 300));
    FilePath imgPath("~res:/TestBed/TestData/UI/Rotation.png");
    ScopedPtr<Sprite> sprite(Sprite::CreateFromSourceFile(imgPath));
    UIControlBackground* overlapedImageBg = overlapedImage->GetOrCreateComponent<UIControlBackground>();
    overlapedImageBg->SetSprite(sprite, 0);
    overlapedImage->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(overlapedImage);

    FilePath srcDir("~res:/TestBed/TestData/TransparentWebViewTest/");
    FilePath cpyDir = FileSystem::Instance()->GetCurrentDocumentsDirectory() + "TransparentWebViewTest/";
    FileSystem::Instance()->DeleteDirectory(cpyDir);
    FileSystem::Instance()->CreateDirectory(cpyDir);
    FilePath srcFile = srcDir + "test.html";
    FilePath cpyFile = cpyDir + "test.html";
    FileSystem::Instance()->CopyFile(srcFile, cpyFile);

    setStaticButton = CreateUIButton(font, Rect(0 + 300, 510, 300, w), L"Render To Texture", &StaticWebViewTest::OnButtonSetStatic);
    setNormalButton = CreateUIButton(font, Rect(0 + 300 * 2, 510, 300, w), L"Normal View", &StaticWebViewTest::OnButtonSetNormal);
    add10ToAlfaButton = CreateUIButton(font, Rect(0 + 300 * 1, 510 + w, 300, w), L"+10 to Alfa", &StaticWebViewTest::OnButtonAdd10ToAlfa);
    minus10FromAlfaButton = CreateUIButton(font, Rect(0 + 300 * 2, 510 + w, 300, w), L"-10 to Alfa", &StaticWebViewTest::OnButtonMinus10FromAlfa);
    checkTransparancyButton = CreateUIButton(font, Rect(0 + 300 * 1, 510 + w * 2, 300, w), L"set Transparent Background", &StaticWebViewTest::OnButtonCheckTransparancy);
    uncheckTransparancyButton = CreateUIButton(font, Rect(0 + 300 * 2, 510 + w * 2, 300, w), L"unset Transparent Background", &StaticWebViewTest::OnButtonUncheckTransparancy);
    executeJSButton = CreateUIButton(font, Rect(0 + 300 * 1, 510 + w * 3, 300, w), L"exec JS", &StaticWebViewTest::OnButtonExecJS);
    loadHTMLString = CreateUIButton(font, Rect(0 + 300 * 2, 510 + w * 3, 300, w), L"load HTML String", &StaticWebViewTest::OnLoadHTMLString);
    setVisibleButton = CreateUIButton(font, Rect(0 + 300 * 1, 510 + w * 4, 300, w), L"Show", &StaticWebViewTest::OnButtonVisible);
    setHideButton = CreateUIButton(font, Rect(0 + 300 * 2, 510 + w * 4, 300, w), L"Hide", &StaticWebViewTest::OnButtonHide);

    SafeRelease(font);

    BaseScreen::LoadResources();
}

void StaticWebViewTest::UnloadResources()
{
    RemoveAllControls();

    SafeRelease(webView1);
    SafeRelease(webView2);
    SafeRelease(webView3);

    SafeDelete(webviewDelegate);

    SafeRelease(setStaticButton);
    SafeRelease(setNormalButton);
    SafeRelease(add10ToAlfaButton);
    SafeRelease(minus10FromAlfaButton);
    SafeRelease(checkTransparancyButton);
    SafeRelease(uncheckTransparancyButton);
    SafeRelease(executeJSButton);
    SafeRelease(loadHTMLString);
    SafeRelease(setVisibleButton);
    SafeRelease(setHideButton);
}

void StaticWebViewTest::OnButtonSetStatic(BaseObject*, void*, void*)
{
    webView1->SetRenderToTexture(true);
    webView2->SetRenderToTexture(true);
    webView3->SetRenderToTexture(true);
}

void StaticWebViewTest::OnButtonSetNormal(BaseObject*, void*, void*)
{
    webView1->SetRenderToTexture(false);
    webView2->SetRenderToTexture(false);
    webView3->SetRenderToTexture(false);
}

void StaticWebViewTest::OnButtonAdd10ToAlfa(BaseObject* obj, void* data, void* callerData)
{
    UIControlBackground* back = webView1->GetOrCreateComponent<UIControlBackground>();
    Sprite* spr = back ? back->GetSprite() : nullptr;
    if (spr)
    {
        Color color = back->GetColor();
        color.a += 0.1f;
        color.a = Min(1.0f, color.a);
        back->SetColor(color);
    }
}

void StaticWebViewTest::OnButtonMinus10FromAlfa(BaseObject* obj, void* data, void* callerData)
{
    UIControlBackground* back = webView1->GetOrCreateComponent<UIControlBackground>();
    Sprite* spr = back ? back->GetSprite() : nullptr;
    if (spr)
    {
        Color color = back->GetColor();
        color.a -= 0.1f;
        color.a = Max(0.f, color.a);
        back->SetColor(color);
    }
}

void StaticWebViewTest::OnButtonCheckTransparancy(BaseObject* obj, void* data, void* callerData)
{
    webView1->SetBackgroundTransparency(true);
    webView2->SetBackgroundTransparency(true);
    webView3->SetBackgroundTransparency(true);
}

void StaticWebViewTest::OnButtonUncheckTransparancy(BaseObject* obj, void* data, void* callerData)
{
    webView1->SetBackgroundTransparency(false);
    webView2->SetBackgroundTransparency(false);
    webView3->SetBackgroundTransparency(false);
}

UIButton* StaticWebViewTest::CreateUIButton(Font* font, const Rect& rect, const WideString& str,
                                            void (StaticWebViewTest::*targetFunction)(BaseObject*, void*, void*))
{
    UIButton* button = new UIButton(rect);
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 20.f);
    button->SetStateText(0xFF, str);
    button->SetStateFontColor(0xFF, Color::White);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, targetFunction));
    AddControl(button);
    return button;
}

void StaticWebViewTest::OnButtonExecJS(BaseObject* obj, void*, void*)
{
    webView1->ExecuteJScript(
    "document.body.innerHTML = \"<H1>Hi from JS!</H1>"
    "<P>Test only test</P>\"");
    webView2->ExecuteJScript("doSomething()");
}

void StaticWebViewTest::OnLoadHTMLString(BaseObject* obj, void*, void*)
{
    static bool switchHtml = false;
    switchHtml = !switchHtml;

    if (switchHtml)
    {
        webView1->LoadHtmlString(
        L"<HTML>"
        L"   <HEAD></HEAD>"
        L"<BODY bgcolor='#E6E6FA'>"
        L"   <H1>Hi</H1>"
        L"   <P>This is HTML document with explicitly set background color</P>"
        L"</BODY>"
        L"</HTML>");
    }
    else
    {
        webView1->LoadHtmlString(
        L"<HTML>"
        L"   <HEAD></HEAD>"
        L"<BODY text='blue'>"
        L"   <H1>Hi</H1>"
        L"   <P>This is HTML document with background color not set</P>"
        L"</BODY>"
        L"</HTML>");
    }
    webView3->LoadHtmlString(htmlString);
}

void StaticWebViewTest::OnButtonVisible(BaseObject*, void*, void*)
{
    webView1->SetVisibilityFlag(true);
    webView2->SetVisibilityFlag(true);
    webView3->SetVisibilityFlag(true);
}

void StaticWebViewTest::OnButtonHide(BaseObject*, void*, void*)
{
    webView1->SetVisibilityFlag(false);
    webView2->SetVisibilityFlag(false);
    webView3->SetVisibilityFlag(false);
}
