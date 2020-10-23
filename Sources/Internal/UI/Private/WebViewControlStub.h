#pragma once

#include "Base/BaseTypes.h"

#if defined(DISABLE_NATIVE_WEBVIEW) && !defined(ENABLE_CEF_WEBVIEW)

#include "UI/IWebViewControl.h"

namespace DAVA
{
class Window;
class WebViewControl : public IWebViewControl
{
public:
    WebViewControl(Window* w, UIWebView* uiWebView);
    // Initialize the control.
    void Initialize(const Rect& rect) override;

    // Open the URL requested.
    void OpenURL(const String& urlToOpen) override;
    // Load html page from string
    void LoadHtmlString(const WideString& htmlString) override;
    // Delete all cookies associated with target URL
    void DeleteCookies(const String& targetUrl) override;
    // Get cookie for specific domain and name
    String GetCookie(const String& url, const String& name) const override;
    // Get the list of cookies for specific domain
    Map<String, String> GetCookies(const String& url) const override;
    // Execute javascript string in webview
    void ExecuteJScript(const String& scriptString) override;

    void OpenFromBuffer(const String& string, const FilePath& basePath) override;

    // Size/pos/visibility changes.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible, bool hierarchic) override;

    void SetDelegate(IUIWebViewDelegate* delegate, UIWebView* webView) override;

    void SetRenderToTexture(bool value) override;
    bool IsRenderToTexture() const override;
};
};

#endif //DISABLE_NATIVE_WEBVIEW && !ENABLE_CEF_WEBVIEW
