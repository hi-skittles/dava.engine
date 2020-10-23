#if !defined(DISABLE_NATIVE_WEBVIEW)

#include "UI/Private/Android/WebViewControl.Android.h"
#include "UI/UIControlSystem.h"
#include "UI/UIControlBackground.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Math/Rect.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"

#include "Engine/Engine.h"
#include "Engine/Window.h"

extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_DavaWebView_nativeReleaseWeakPtr(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::WebViewControl;

    // Postpone deleting in case some other jobs are posted to main thread
    DAVA::RunOnMainThreadAsync([backendPointer]() {
        std::weak_ptr<WebViewControl>* weak = reinterpret_cast<std::weak_ptr<WebViewControl>*>(static_cast<uintptr_t>(backendPointer));
        delete weak;
    });
}

JNIEXPORT jint JNICALL Java_com_dava_engine_DavaWebView_nativeOnUrlChanged(JNIEnv* env, jclass jclazz, jlong backendPointer, jstring url, jboolean isRedirectedByMouseClick)
{
    using DAVA::WebViewControl;
    std::weak_ptr<WebViewControl>* weak = reinterpret_cast<std::weak_ptr<WebViewControl>*>(static_cast<uintptr_t>(backendPointer));
    if (auto backend = weak->lock())
        return backend->nativeOnUrlChanged(env, url, isRedirectedByMouseClick);
    return static_cast<jint>(DAVA::IUIWebViewDelegate::NO_PROCESS);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaWebView_nativeOnPageLoaded(JNIEnv* env, jclass jclazz, jlong backendPointer)
{
    using DAVA::WebViewControl;
    std::weak_ptr<WebViewControl>* weak = reinterpret_cast<std::weak_ptr<WebViewControl>*>(static_cast<uintptr_t>(backendPointer));
    if (auto backend = weak->lock())
        backend->nativeOnPageLoaded(env);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaWebView_nativeOnExecuteJavaScript(JNIEnv* env, jclass jclazz, jlong backendPointer, jstring result)
{
    using DAVA::WebViewControl;
    std::weak_ptr<WebViewControl>* weak = reinterpret_cast<std::weak_ptr<WebViewControl>*>(static_cast<uintptr_t>(backendPointer));
    if (auto backend = weak->lock())
        backend->nativeOnExecuteJavaScript(env, result);
}

} // extern "C"

namespace DAVA
{
WebViewControl::WebViewControl(Window* w, UIWebView* uiWebView)
    : window(w)
    , uiWebView(uiWebView)
{
}

WebViewControl::~WebViewControl() = default;

void WebViewControl::Initialize(const Rect& rect)
{
    try
    {
        webViewJavaClass.reset(new JNI::JavaClass("com/dava/engine/DavaWebView"));
        release = webViewJavaClass->GetMethod<void>("release");
        openURL = webViewJavaClass->GetMethod<void, jstring>("openURL");
        loadHtmlString = webViewJavaClass->GetMethod<void, jstring>("loadHtmlString");
        openFromBuffer = webViewJavaClass->GetMethod<void, jstring, jstring>("openFromBuffer");
        executeJScript = webViewJavaClass->GetMethod<void, jstring>("executeJScript");
        setRect = webViewJavaClass->GetMethod<void, jfloat, jfloat, jfloat, jfloat>("setRect");
        setVisible = webViewJavaClass->GetMethod<void, jboolean>("setVisible");
        setBackgroundTransparency = webViewJavaClass->GetMethod<void, jboolean>("setBackgroundTransparency");
        setRenderToTexture = webViewJavaClass->GetMethod<void, jboolean>("setRenderToTexture");
        isRenderToTexture = webViewJavaClass->GetMethod<jboolean>("isRenderToTexture");
        update = webViewJavaClass->GetMethod<void>("update");
        deleteCookies = webViewJavaClass->GetStaticMethod<void, jstring>("deleteCookies");
        getCookie = webViewJavaClass->GetStaticMethod<jstring, jstring, jstring>("getCookie");
        getCookies = webViewJavaClass->GetStaticMethod<jstringArray, jstring>("getCookies");
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("[WebViewControl] failed to init java bridge: %s", e.what());
        DVASSERT(false, e.what());
        return;
    }

    std::weak_ptr<WebViewControl>* selfWeakPtr = new std::weak_ptr<WebViewControl>(shared_from_this());
    jobject obj = PlatformApi::Android::CreateNativeControl(window, "com.dava.engine.DavaWebView", selfWeakPtr);
    if (obj != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        javaWebView = env->NewGlobalRef(obj);
        env->DeleteLocalRef(obj);
        SetRect(rect);
    }
    else
    {
        delete selfWeakPtr;
        Logger::Error("[WebViewControl] failed to create java webview");
    }
}

void WebViewControl::OwnerIsDying()
{
    uiWebView = nullptr;
    uiWebViewDelegate = nullptr;
    if (javaWebView != nullptr)
    {
        release(javaWebView);
        JNI::GetEnv()->DeleteGlobalRef(javaWebView);
        javaWebView = nullptr;
    }
}

void WebViewControl::OpenURL(const String& url)
{
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jstr = JNI::StringToJavaString(url, env);

        openURL(javaWebView, jstr);
        env->DeleteLocalRef(jstr);
    }
}

void WebViewControl::LoadHtmlString(const WideString& htmlString)
{
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jstr = JNI::WideStringToJavaString(htmlString, env);

        loadHtmlString(javaWebView, jstr);
        env->DeleteLocalRef(jstr);
    }
}

void WebViewControl::OpenFromBuffer(const String& htmlString, const FilePath& basePath)
{
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jstr = JNI::StringToJavaString(htmlString, env);
        jstring jbase = JNI::StringToJavaString(basePath.AsURL(), env);

        openFromBuffer(javaWebView, jstr, jbase);
        env->DeleteLocalRef(jstr);
        env->DeleteLocalRef(jbase);
    }
}

void WebViewControl::ExecuteJScript(const String& jsScript)
{
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jstr = JNI::StringToJavaString(jsScript, env);

        executeJScript(javaWebView, jstr);
        env->DeleteLocalRef(jstr);
    }
}

void WebViewControl::SetRect(const Rect& rect)
{
    if (javaWebView != nullptr)
    {
        Rect rc = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(rect);
        rc.dx = std::max(0.0f, rc.dx);
        rc.dy = std::max(0.0f, rc.dy);

        setRect(javaWebView, rc.x, rc.y, rc.dx, rc.dy);
    }
}

void WebViewControl::SetVisible(bool visible, bool /*hierarchic*/)
{
    if (javaWebView != nullptr)
    {
        setVisible(javaWebView, visible ? JNI_TRUE : JNI_FALSE);
    }
}

void WebViewControl::SetBackgroundTransparency(bool enabled)
{
    if (javaWebView != nullptr)
    {
        setBackgroundTransparency(javaWebView, enabled ? JNI_TRUE : JNI_FALSE);
    }
}

void WebViewControl::SetDelegate(IUIWebViewDelegate* webViewDelegate, UIWebView* /*uiWebView*/)
{
    uiWebViewDelegate = webViewDelegate;
}

void WebViewControl::SetRenderToTexture(bool enabled)
{
    if (javaWebView != nullptr)
    {
        setRenderToTexture(javaWebView, enabled ? JNI_TRUE : JNI_FALSE);
    }
}

bool WebViewControl::IsRenderToTexture() const
{
    return javaWebView != nullptr ? isRenderToTexture(javaWebView) == JNI_TRUE : false;
}

void WebViewControl::Update()
{
    if (javaWebView != nullptr)
    {
        update(javaWebView);
    }
}

void WebViewControl::DeleteCookies(const String& url)
{
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jurl = JNI::StringToJavaString(url, env);
        deleteCookies(jurl);
        env->DeleteLocalRef(jurl);
    }
}

String WebViewControl::GetCookie(const String& url, const String& name) const
{
    String cookie;
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jurl = JNI::StringToJavaString(url, env);
        jstring jname = JNI::StringToJavaString(name, env);
        jstring jcookie = getCookie(jurl, jname);
        cookie = JNI::JavaStringToString(jcookie);
        env->DeleteLocalRef(jurl);
        env->DeleteLocalRef(jname);
        env->DeleteLocalRef(jcookie);
    }
    return cookie;
}

Map<String, String> WebViewControl::GetCookies(const String& url) const
{
    Map<String, String> result;
    if (javaWebView != nullptr)
    {
        JNIEnv* env = JNI::GetEnv();
        jstring jurl = JNI::StringToJavaString(url, env);
        jstringArray jcookies = getCookies(jurl);

        jsize n = env->GetArrayLength(jcookies);
        for (jsize i = 0; i < n; ++i)
        {
            jstring jitem = static_cast<jstring>(env->GetObjectArrayElement(jcookies, i));
            String item = JNI::JavaStringToString(jitem);
            env->DeleteLocalRef(jitem);

            Vector<String> cookie;
            Split(item, "=", cookie);

            result[cookie[0]] = cookie[1];
        }

        env->DeleteLocalRef(jurl);
        env->DeleteLocalRef(jcookies);
    }
    return result;
}

jint WebViewControl::nativeOnUrlChanged(JNIEnv* env, jstring jurl, jboolean jisRedirectedByMouseClick)
{
    String url = JNI::JavaStringToString(jurl, env);

    bool isRedirectedByMouseClick = jisRedirectedByMouseClick == JNI_TRUE;
    IUIWebViewDelegate::eAction action = IUIWebViewDelegate::PROCESS_IN_WEBVIEW;
    RunOnMainThread([this, url, isRedirectedByMouseClick, &action]() {
        action = OnUrlChanged(url, isRedirectedByMouseClick);
    });

    return static_cast<jint>(action);
}

void WebViewControl::nativeOnPageLoaded(JNIEnv* env)
{
    RunOnMainThreadAsync([this]() {
        OnPageLoaded();
    });
}

void WebViewControl::nativeOnExecuteJavaScript(JNIEnv* env, jstring jresult)
{
    String result = JNI::JavaStringToString(jresult, env);
    RunOnMainThreadAsync([this, result]() {
        OnExecuteJavaScript(result);
    });
}

IUIWebViewDelegate::eAction WebViewControl::OnUrlChanged(const String& url, bool isRedirectedByMouseClick)
{
    IUIWebViewDelegate::eAction action = IUIWebViewDelegate::PROCESS_IN_WEBVIEW;
    if (uiWebViewDelegate != nullptr)
    {
        action = uiWebViewDelegate->URLChanged(uiWebView, url, isRedirectedByMouseClick);
    }
    return action;
}

void WebViewControl::OnPageLoaded()
{
    if (uiWebViewDelegate != nullptr)
    {
        uiWebViewDelegate->PageLoaded(uiWebView);
    }
}

void WebViewControl::OnExecuteJavaScript(const String& result)
{
    if (uiWebViewDelegate != nullptr)
    {
        uiWebViewDelegate->OnExecuteJScript(uiWebView, result);
    }
}

} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
#endif // !DISABLE_NATIVE_WEBVIEW
