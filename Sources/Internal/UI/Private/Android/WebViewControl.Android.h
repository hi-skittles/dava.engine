#pragma once

#if !defined(DISABLE_NATIVE_WEBVIEW)

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/PlatformApiAndroid.h"

#include "Functional/Function.h"
#include "UI/IWebViewControl.h"
#include "UI/IUIWebViewDelegate.h"

namespace DAVA
{
class Rect;
class FilePath;

class WebViewControl : public IWebViewControl,
                       public std::enable_shared_from_this<WebViewControl>
{
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
    void ExecuteJScript(const String& jsScript) override;

    // Delete all cookies associated with target URL
    void DeleteCookies(const String& url) override;
    // Get cookie for specific domain and name
    String GetCookie(const String& url, const String& name) const override;
    // Get the list of cookies for specific domain
    Map<String, String> GetCookies(const String& url) const override;

    // Size/pos/visibility changes.
    void SetRect(const Rect& rect) override;
    void SetVisible(bool visible, bool hierarchic) override;
    void SetBackgroundTransparency(bool enabled) override;

    void SetDelegate(IUIWebViewDelegate* webViewDelegate, UIWebView* webView) override;

    void SetRenderToTexture(bool enabled) override;
    bool IsRenderToTexture() const override;

    void Update() override;

    jint nativeOnUrlChanged(JNIEnv* env, jstring jurl, jboolean jisRedirectedByMouseClick);
    void nativeOnPageLoaded(JNIEnv* env);
    void nativeOnExecuteJavaScript(JNIEnv* env, jstring jresult);

private:
    IUIWebViewDelegate::eAction OnUrlChanged(const String& url, bool isRedirectedByMouseClick);
    void OnPageLoaded();
    void OnExecuteJavaScript(const String& result);

private:
    Window* window = nullptr;
    UIWebView* uiWebView = nullptr;
    IUIWebViewDelegate* uiWebViewDelegate = nullptr;
    jobject javaWebView = nullptr;

    std::unique_ptr<JNI::JavaClass> webViewJavaClass;
    Function<void(jobject)> release;
    Function<void(jobject, jstring)> openURL;
    Function<void(jobject, jstring)> loadHtmlString;
    Function<void(jobject, jstring, jstring)> openFromBuffer;
    Function<void(jobject, jstring)> executeJScript;
    Function<void(jobject, jfloat, jfloat, jfloat, jfloat)> setRect;
    Function<void(jobject, jboolean)> setVisible;
    Function<void(jobject, jboolean)> setBackgroundTransparency;
    Function<void(jobject, jboolean)> setRenderToTexture;
    Function<jboolean(jobject)> isRenderToTexture;
    Function<void(jobject)> update;
    Function<void(jstring)> deleteCookies;
    Function<jstring(jstring, jstring)> getCookie;
    Function<jstringArray(jstring)> getCookies;
};

} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
#endif // !DISABLE_NATIVE_WEBVIEW
