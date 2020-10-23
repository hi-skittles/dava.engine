#include "UI/Private/WebViewControlStub.h"

#if defined(DISABLE_NATIVE_WEBVIEW) && !defined(ENABLE_CEF_WEBVIEW)

namespace DAVA
{
WebViewControl::WebViewControl(Window* /*w*/, UIWebView* /*uiWebView*/)
{
}

void WebViewControl::Initialize(const Rect&)
{
}

void WebViewControl::OpenURL(const String&)
{
}

void WebViewControl::LoadHtmlString(const WideString&)
{
}

void WebViewControl::DeleteCookies(const String&)
{
}

String WebViewControl::GetCookie(const String&, const String&) const
{
    return String();
}

Map<String, String> WebViewControl::GetCookies(const String&) const
{
    return Map<String, String>();
}

void WebViewControl::ExecuteJScript(const String&)
{
}

void WebViewControl::OpenFromBuffer(const String&, const FilePath&)
{
}

void WebViewControl::SetRect(const Rect&)
{
}

void WebViewControl::SetVisible(bool, bool)
{
}

void WebViewControl::SetDelegate(IUIWebViewDelegate*, UIWebView*)
{
}

void WebViewControl::SetRenderToTexture(bool)
{
}

bool WebViewControl::IsRenderToTexture() const
{
    return false;
}
} // namespace DAVA

#endif //DISABLE_NATIVE_WEBVIEW && !ENABLE_CEF_WEBVIEW
