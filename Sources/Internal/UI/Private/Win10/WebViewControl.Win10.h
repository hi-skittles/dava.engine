#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__) && !defined(DISABLE_NATIVE_WEBVIEW)

#include "UI/IWebViewControl.h"
#include "UI/IUIWebViewDelegate.h"

namespace DAVA
{
class Window;
class Sprite;
class CorePlatformWinUAP;
class PrivateWebViewWinUAP;
class Window;

// Web View Control for WinUAP
class WebViewControl : public IWebViewControl,
                       public std::enable_shared_from_this<WebViewControl>
{
    struct WebViewProperties
    {
        enum
        {
            NAVIGATE_NONE = 0,
            NAVIGATE_OPEN_URL,
            NAVIGATE_LOAD_HTML,
            NAVIGATE_OPEN_BUFFER
        };

        void ClearChangedFlags();

        Rect rect;
        Rect rectInWindowSpace;
        bool visible = false;
        bool renderToTexture = false;
        bool backgroundTransparency = false;
        WideString htmlString; // for LoadHtmlString
        String urlOrHtml; // for OpenURL or OpenFromBuffer
        FilePath basePath; // for OpenFromBuffer
        String jsScript; // for ExecuteJScript

        bool createNew : 1;
        bool anyPropertyChanged : 1;
        bool rectChanged : 1;
        bool visibleChanged : 1;
        bool renderToTextureChanged : 1;
        bool backgroundTransparencyChanged : 1;
        bool execJavaScript : 1;
        uint32 navigateTo : 2;
    };

public:
    WebViewControl(Window* w, UIWebView* uiWebView);
    ~WebViewControl() override;

    // Initialize the control.
    void Initialize(const Rect& rect) override;
    void OwnerIsDying() override;

    // Open the URL requested.
    void OpenURL(const String& url) override;
    // Load html page from string
    void LoadHtmlString(const WideString& htmlString) override;
    void OpenFromBuffer(const String& htmlString, const FilePath& basePath) override;
    // Execute javascript string in webview
    void ExecuteJScript(const String& scriptString) override;

    // Delete all cookies associated with target URL
    void DeleteCookies(const String& url) override;
    // Get cookie for specific domain and name
    String GetCookie(const String& url, const String& name) const override;
    // Get the list of cookies for specific domain
    Map<String, String> GetCookies(const String& url) const override;

    // Size/pos/visibility changes.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool isVisible, bool hierarchic) override;
    void SetBackgroundTransparency(bool enabled) override;

    void SetDelegate(IUIWebViewDelegate* webViewDelegate, UIWebView* uiWebView) override;

    void SetRenderToTexture(bool value) override;
    bool IsRenderToTexture() const override;

    void Update() override;

private:
    void CreateNativeControl();
    void InstallEventHandlers();

    void ProcessProperties(const WebViewProperties& props);
    void ApplyChangedProperties(const WebViewProperties& props);

    void SetNativePositionAndSize(const Rect& rect, bool offScreen);
    void SetNativeBackgroundTransparency(bool enabled);
    void NativeNavigateTo(const WebViewProperties& props);
    void NativeExecuteJavaScript(const String& jsScript);

    Rect VirtualToWindow(const Rect& srcRect) const;

    void RenderToTexture();
    Sprite* CreateSpriteFromPreviewData(uint8* imageData, int32 width, int32 height) const;

private: // WebView event handlers
    void OnNavigationStarting(Windows::UI::Xaml::Controls::WebView ^ sender, Windows::UI::Xaml::Controls::WebViewNavigationStartingEventArgs ^ args);
    void OnNavigationCompleted(Windows::UI::Xaml::Controls::WebView ^ sender, Windows::UI::Xaml::Controls::WebViewNavigationCompletedEventArgs ^ args);
    void OnUnsupportedUriSchemeIdentified(::Windows::UI::Xaml::Controls::WebView ^, ::Windows::UI::Xaml::Controls::WebViewUnsupportedUriSchemeIdentifiedEventArgs ^ args);

    IUIWebViewDelegate::eAction HandleUriNavigation(::Windows::Foundation::Uri ^ uri);

    // Signal handlers
    void OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize);
    void OnWindowDestroyed(Window* w);

private:
    // clang-format off
    Window* window = nullptr;
    UIWebView* uiWebView = nullptr;
    IUIWebViewDelegate* webViewDelegate = nullptr;
    Windows::UI::Xaml::Controls::WebView^ nativeWebView = nullptr;
    Windows::UI::Color defaultBkgndColor;   // Original WebView's background color used on transparency turned off
    bool programmaticUrlNavigation = false; // Flag indicating that WebView has followed URL provided by code not by user click

    bool renderToTexture = false;
    bool visible = false;                   // Native control initially is invisible
    Rect rectInWindowSpace;

    Windows::Foundation::EventRegistrationToken tokenNavigationStarting;
    Windows::Foundation::EventRegistrationToken tokenNavigationCompleted;
    Windows::Foundation::EventRegistrationToken tokenUnsupportedUriSchemeIdentified;

    WebViewProperties properties;

    // clang-format on
};

} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__ && !DISABLE_NATIVE_WEBVIEW
