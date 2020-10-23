#include "UI/UIWebView.h"

#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "UI/UIControlSystem.h"
#include "UI/Update/UIUpdateComponent.h"

#if defined(DISABLE_NATIVE_WEBVIEW) && !defined(ENABLE_CEF_WEBVIEW)
#include "UI/Private/WebViewControlStub.h"
#elif defined(ENABLE_CEF_WEBVIEW)
#include "UI/Private/CEF/WebViewControl.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "UI/Private/Mac/WebViewControl.Macos.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "UI/Private/Ios/WebViewControl.Ios.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "UI/Private/Win10/WebViewControl.Win10.h"
#elif defined(__DAVAENGINE_ANDROID__)
#include "UI/Private/Android/WebViewControl.Android.h"
#else
#error UIWEbView control is not implemented for this platform yet!
#endif

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIWebView)
{
    ReflectionRegistrator<UIWebView>::Begin()[M::DisplayName("Web View")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIWebView* o) { o->Release(); })
    .Field("dataDetectorTypes", &UIWebView::GetDataDetectorTypes, &UIWebView::SetDataDetectorTypes)[M::EnumT<eDataDetectorType>(), M::DisplayName("Data Detector Type")]
    .End();
}

UIWebView::UIWebView(const Rect& rect)
    : UIControl(rect)
    , webViewControl(std::make_shared<WebViewControl>(Engine::Instance()->PrimaryWindow(), this))
    , isNativeControlVisible(false)
{
    Rect newRect = GetAbsoluteRect();
    webViewControl->Initialize(newRect);
    UpdateControlRect();

    UpdateNativeControlVisible(false); // will be displayed in OnActive.
    SetDataDetectorTypes(DATA_DETECTOR_LINKS);

    GetOrCreateComponent<UIUpdateComponent>()->SetUpdateInvisible(true);
}

UIWebView::~UIWebView()
{
    webViewControl->OwnerIsDying();
}

void UIWebView::SetDelegate(IUIWebViewDelegate* delegate)
{
    webViewControl->SetDelegate(delegate, this);
}

void UIWebView::OpenFile(const FilePath& path)
{
    // Open files in a browser via a buffer necessary because
    // the reference type file:// is not supported in Windows 10
    // for security reasons
    ScopedPtr<File> file(File::Create(path, File::OPEN | File::READ));
    DVASSERT(file, "[UIWebView] Failed to open file");
    String data;
    if (file && file->ReadString(data) > 0)
    {
        // First we should resolve full path to the file and only then get
        // directory path from resolved path
        String fullPath = path.GetAbsolutePathname();
        FilePath dir(fullPath);
        dir = dir.GetDirectory();
        webViewControl->OpenFromBuffer(data, dir);
    }
    else
    {
        Logger::Error("[UIWebView] Failed to read content from %s", path.GetStringValue().c_str());
    }
}

void UIWebView::OpenURL(const String& urlToOpen)
{
    webViewControl->OpenURL(urlToOpen);
}

void UIWebView::LoadHtmlString(const WideString& htmlString)
{
    webViewControl->LoadHtmlString(htmlString);
}

String UIWebView::GetCookie(const String& targetUrl, const String& name) const
{
    return webViewControl->GetCookie(targetUrl, name);
}

Map<String, String> UIWebView::GetCookies(const String& targetUrl) const
{
    return webViewControl->GetCookies(targetUrl);
}

void UIWebView::DeleteCookies(const String& targetUrl)
{
    webViewControl->DeleteCookies(targetUrl);
}

void UIWebView::ExecuteJScript(const String& scriptString)
{
    webViewControl->ExecuteJScript(scriptString);
}

void UIWebView::OpenFromBuffer(const String& string, const FilePath& basePath)
{
    webViewControl->OpenFromBuffer(string, basePath);
}

void UIWebView::OnVisible()
{
    UIControl::OnVisible();
    UpdateNativeControlVisible(true);
}

void UIWebView::OnInvisible()
{
    UIControl::OnInvisible();
    UpdateNativeControlVisible(false);
}

void UIWebView::OnActive()
{
    UIControl::OnActive();
    UpdateControlRect();
}

void UIWebView::SetPosition(const Vector2& position)
{
    UIControl::SetPosition(position);
    UpdateControlRect();
}

void UIWebView::SetSize(const Vector2& newSize)
{
    UIControl::SetSize(newSize);
    UpdateControlRect();
}

void UIWebView::SetScalesPageToFit(bool isScalesToFit)
{
    webViewControl->SetScalesPageToFit(isScalesToFit);
}

void UIWebView::SetBackgroundTransparency(bool enabled)
{
    webViewControl->SetBackgroundTransparency(enabled);
}

// Enable/disable bounces.
void UIWebView::SetBounces(bool value)
{
    webViewControl->SetBounces(value);
}

bool UIWebView::GetBounces() const
{
    return webViewControl->GetBounces();
}

void UIWebView::SetGestures(bool value)
{
    webViewControl->SetGestures(value);
}

void UIWebView::UpdateControlRect()
{
    Rect rect = GetAbsoluteRect();

    webViewControl->SetRect(rect);
}

void UIWebView::SetRenderToTexture(bool value)
{
    value = false; //-V763 for now disable this functionality
    webViewControl->SetRenderToTexture(value);
}

bool UIWebView::IsRenderToTexture() const
{
    return webViewControl->IsRenderToTexture();
}

void UIWebView::SetNativeControlVisible(bool isVisible)
{
    UpdateNativeControlVisible(isVisible);
}

bool UIWebView::GetNativeControlVisible() const
{
    return isNativeControlVisible;
}

void UIWebView::UpdateNativeControlVisible(bool value)
{
    isNativeControlVisible = value;
    webViewControl->SetVisible(value, true);
}

void UIWebView::SetDataDetectorTypes(int32 value)
{
    dataDetectorTypes = value;
    webViewControl->SetDataDetectorTypes(value);
}

int32 UIWebView::GetDataDetectorTypes() const
{
    return dataDetectorTypes;
}

UIWebView* UIWebView::Clone()
{
    UIWebView* webView = new UIWebView(GetRect());
    webView->CopyDataFrom(this);
    return webView;
}

void UIWebView::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);

    UIWebView* webView = DynamicTypeCheck<UIWebView*>(srcControl);
    SetNativeControlVisible(webView->GetNativeControlVisible());
    SetDataDetectorTypes(webView->GetDataDetectorTypes());
}

void UIWebView::Draw(const UIGeometricData& geometricData)
{
    webViewControl->WillDraw();
    UIControl::Draw(geometricData);
    webViewControl->Draw(geometricData);
    webViewControl->DidDraw();
}

void UIWebView::Input(UIEvent* currentInput)
{
    webViewControl->Input(currentInput);
    UIControl::Input(currentInput);
}

void UIWebView::Update(float32 timeElapsed)
{
    webViewControl->Update();
    UIControl::Update(timeElapsed);
}

} // namespace DAVA
