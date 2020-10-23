#pragma once

#include "Base/BaseTypes.h"
#if defined __DAVAENGINE_MACOS__ && !defined DISABLE_NATIVE_WEBVIEW

#include "UI/IWebViewControl.h"

namespace DAVA
{
class Window;
class UIWebView;

// Web View Control - MacOS version.
class WebViewControl : public IWebViewControl
{
public:
    WebViewControl(Window* w, UIWebView* uiWebView);
    virtual ~WebViewControl();

    // Initialize the control.
    void Initialize(const Rect& rect) override;

    // Open the URL requested.
    void OpenURL(const String& urlToOpen) override;
    // Load html page from string
    void LoadHtmlString(const WideString& htlmString) override;
    // Delete all cookies associated with target URL
    void DeleteCookies(const String& targetUrl) override;
    // Execute javascript command, return request ID
    void ExecuteJScript(const String& scriptString) override;

    void OpenFromBuffer(const String& string, const FilePath& basePath) override;

    // Size/pos/visibility changes.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible, bool hierarchic) override;

    void SetDelegate(IUIWebViewDelegate* delegate, UIWebView* webView) override;
    void SetBackgroundTransparency(bool enabled) override;

    void SetRenderToTexture(bool value) override;
    bool IsRenderToTexture() const override;

    void SetImageCache(void* ptr);
    void* GetImageCache() const;

    void RenderToTextureAndSetAsBackgroundSpriteToControl(UIWebView& uiWebViewControl);

private:
    void SetNativeVisible(bool visible);

    void OnWindowVisibilityChanged(Window* w, bool visible);
    void OnWindowDestroyed(Window* w);

#if defined(__DAVAENGINE_STEAM__)
    bool overlayVisible = false;
    void OnSteamOverlayChanged(bool overlayActivated);
#endif

private:
    UIWebView& uiWebViewControl;
    Window* window = nullptr;

    struct WebViewObjCBridge;
    WebViewObjCBridge* bridge = nullptr;

    bool isRenderToTexture = false;
    bool isVisible = true;
};

inline bool WebViewControl::IsRenderToTexture() const
{
    return isRenderToTexture;
}
}

#endif //defined __DAVAENGINE_MACOS__ && !defined DISABLE_NATIVE_WEBVIEW
