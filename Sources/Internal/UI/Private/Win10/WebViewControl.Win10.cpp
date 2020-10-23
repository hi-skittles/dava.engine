#include "UI/Private/Win10/WebViewControl.Win10.h"

#if defined(__DAVAENGINE_WIN_UAP__) && !defined(DISABLE_NATIVE_WEBVIEW)

#include <ppltasks.h>
#include <collection.h>

#include "Base/RefPtr.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiWin10.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Render/2D/Sprite.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/Image.h"
#include "UI/UIControlBackground.h"
#include "UI/UIControlSystem.h"
#include "UI/UIWebView.h"
#include "Utils/Random.h"
#include "Utils/StringFormat.h"
#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"

namespace DAVA
{
// clang-format off
/*
    UriResolver is intended for mapping resource paths in html source to local resource files

    UriResolver is used in WebViewControl's OpenFromBuffer method which allows to load
    HTML string specifying location of resource files (css, images, etc).
*/
private ref class UriResolver sealed : public ::Windows::Web::IUriToStreamResolver
{
internal:
    UriResolver(const String& htmlData, const FilePath& basePath);

public:
    virtual ::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream^>^ UriToStreamAsync(::Windows::Foundation::Uri^ uri);

private:
    ::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream ^> ^ GetStreamFromFilePathAsync(const FilePath& filePath);
    ::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream^>^ GetStreamFromStringAsync(Platform::String^ s);

    Platform::String^ htmlData;
    FilePath basePath;
};

UriResolver::UriResolver(const String& htmlData_, const FilePath& basePath_)
    : htmlData(ref new Platform::String(UTF8Utils::EncodeToWideString(htmlData_).c_str()))
    , basePath(basePath_)
{
}

::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream^>^ UriResolver::UriToStreamAsync(::Windows::Foundation::Uri^ uri)
{
    DVASSERT(uri != nullptr);

    Platform::String^ dummy = uri->Path;
    if (0 == Platform::String::CompareOrdinal(uri->Path, L"/johny23"))
    {   // Create stream from HTML data
        return GetStreamFromStringAsync(htmlData);
    }

    FilePath path = basePath + RTStringToString(uri->Path);
    return GetStreamFromFilePathAsync(path);
}

::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream ^> ^ UriResolver::GetStreamFromFilePathAsync(const FilePath& filePath)
{
    using ::concurrency::create_async;
    using ::Windows::Storage::StorageFile;
    using ::Windows::Storage::FileAccessMode;
    using ::Windows::Storage::Streams::IInputStream;
    using ::Windows::Storage::Streams::IRandomAccessStream;
    using ::Windows::Storage::Streams::InMemoryRandomAccessStream;

    String fileNameStr = filePath.GetAbsolutePathname();
    std::replace(fileNameStr.begin(), fileNameStr.end(), '/', '\\');
    Platform::String^ fileName = StringToRTString(fileNameStr);

    return create_async([ this, fileName ]() -> IInputStream^ {
        try
        {
            StorageFile^ storageFile = WaitAsync(StorageFile::GetFileFromPathAsync(fileName));
            IRandomAccessStream^ stream = WaitAsync(storageFile->OpenAsync(FileAccessMode::Read));
            return static_cast<IInputStream^>(stream);
        }
        catch (Platform::COMException^ e)
        {
            Logger::Error("[WebView] failed to load file %s: %s (0x%08x)",
                          RTStringToString(fileName).c_str(),
                          RTStringToString(e->Message).c_str(),
                          e->HResult);
            return ref new InMemoryRandomAccessStream();
        }
    });
}

::Windows::Foundation::IAsyncOperation<::Windows::Storage::Streams::IInputStream^>^ UriResolver::GetStreamFromStringAsync(Platform::String^ s)
{
    using ::concurrency::create_async;
    using ::Windows::Storage::Streams::DataWriter;
    using ::Windows::Storage::Streams::IInputStream;
    using ::Windows::Storage::Streams::InMemoryRandomAccessStream;

    InMemoryRandomAccessStream^ stream = ref new InMemoryRandomAccessStream();
    DataWriter^ writer = ref new DataWriter(stream->GetOutputStreamAt(0));

    writer->WriteString(s);
    WaitAsync(writer->StoreAsync());

    return create_async([stream]() -> IInputStream^ { return stream; });
}
// clang-format on
//////////////////////////////////////////////////////////////////////////

void WebViewControl::WebViewProperties::ClearChangedFlags()
{
    createNew = false;
    anyPropertyChanged = false;
    rectChanged = false;
    visibleChanged = false;
    renderToTextureChanged = false;
    backgroundTransparencyChanged = false;
    execJavaScript = false;
    navigateTo = WebViewProperties::NAVIGATE_NONE;
}

WebViewControl::WebViewControl(Window* w, UIWebView* uiWebView)
    : window(w)
    , uiWebView(uiWebView)
    , properties()
{
}

WebViewControl::~WebViewControl()
{
    nativeWebView = nullptr;
}

void WebViewControl::OwnerIsDying()
{
    using ::Windows::UI::Xaml::Controls::WebView;

    uiWebView = nullptr;
    webViewDelegate = nullptr;
    if (window != nullptr)
    {
        if (nativeWebView != nullptr)
        {
            auto self{ shared_from_this() };
            window->RunOnUIThreadAsync([this, self]() {
                nativeWebView->NavigationStarting -= tokenNavigationStarting;
                nativeWebView->NavigationCompleted -= tokenNavigationCompleted;
                nativeWebView->UnsupportedUriSchemeIdentified -= tokenUnsupportedUriSchemeIdentified;
                PlatformApi::Win10::RemoveXamlControl(window, nativeWebView);
            });
        }
        window->sizeChanged.Disconnect(this);
        Engine::Instance()->windowDestroyed.Disconnect(this);
    }
}

void WebViewControl::Initialize(const Rect& rect)
{
    properties.createNew = true;

    properties.rect = rect;
    properties.rectInWindowSpace = VirtualToWindow(rect);
    properties.rectChanged = true;
    properties.anyPropertyChanged = true;

    window->sizeChanged.Connect(this, &WebViewControl::OnWindowSizeChanged);
    Engine::Instance()->windowDestroyed.Connect(this, &WebViewControl::OnWindowDestroyed);
}

void WebViewControl::OpenURL(const String& urlToOpen)
{
    properties.urlOrHtml = urlToOpen;
    properties.navigateTo = WebViewProperties::NAVIGATE_OPEN_URL;
    properties.anyPropertyChanged = true;
}

void WebViewControl::LoadHtmlString(const WideString& htmlString)
{
    properties.htmlString = htmlString;
    properties.navigateTo = WebViewProperties::NAVIGATE_LOAD_HTML;
    properties.anyPropertyChanged = true;
}

void WebViewControl::OpenFromBuffer(const String& htmlString, const FilePath& basePath)
{
    properties.urlOrHtml = htmlString;
    properties.basePath = basePath;
    properties.navigateTo = WebViewProperties::NAVIGATE_OPEN_BUFFER;
    properties.anyPropertyChanged = true;
}

void WebViewControl::ExecuteJScript(const String& scriptString)
{
    properties.jsScript = scriptString;
    properties.execJavaScript = true;
    properties.anyPropertyChanged = true;
}

void WebViewControl::SetRect(const Rect& rect)
{
    if (properties.rect != rect)
    {
        properties.rect = rect;
        properties.rectInWindowSpace = VirtualToWindow(rect);
        properties.rectChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void WebViewControl::SetVisible(bool isVisible, bool /*hierarchic*/)
{
    if (properties.visible != isVisible)
    {
        properties.visible = isVisible;
        properties.visibleChanged = true;
        properties.anyPropertyChanged = true;
        if (!isVisible)
        { // Immediately hide native control if it has been already created
            auto self{ shared_from_this() };
            if (window != nullptr)
            {
                window->RunOnUIThreadAsync([this, self]() {
                    if (nativeWebView != nullptr)
                    {
                        SetNativePositionAndSize(rectInWindowSpace, true);
                    }
                });
            }
        }
    }
}

void WebViewControl::SetBackgroundTransparency(bool enabled)
{
    if (properties.backgroundTransparency != enabled)
    {
        properties.backgroundTransparency = enabled;
        properties.backgroundTransparencyChanged = true;
        properties.anyPropertyChanged = true;
    }
}

void WebViewControl::SetDelegate(IUIWebViewDelegate* webViewDelegate_, UIWebView* /*uiWebView*/)
{
    webViewDelegate = webViewDelegate_;
}

void WebViewControl::SetRenderToTexture(bool value)
{
    if (properties.renderToTexture != value)
    {
        properties.renderToTexture = value;
        properties.renderToTextureChanged = true;
        properties.anyPropertyChanged = true;
        if (!value)
        { // Immediately hide native control if it has been already created
            auto self{ shared_from_this() };
            window->RunOnUIThreadAsync([this, self]() {
                if (nativeWebView != nullptr)
                {
                    SetNativePositionAndSize(rectInWindowSpace, true);
                }
            });
        }
    }
}

bool WebViewControl::IsRenderToTexture() const
{
    return properties.renderToTexture;
}

void WebViewControl::Update()
{
    if (properties.createNew || properties.anyPropertyChanged)
    {
        auto self{ shared_from_this() };
        WebViewProperties props(properties);
        window->RunOnUIThreadAsync([this, self, props]() {
            ProcessProperties(props);
        });

        properties.createNew = false;
        properties.ClearChangedFlags();
    }
}

void WebViewControl::CreateNativeControl()
{
    using ::Windows::UI::Xaml::Visibility;
    using ::Windows::UI::Xaml::Controls::WebView;

    nativeWebView = ref new WebView();
    defaultBkgndColor = nativeWebView->DefaultBackgroundColor;
    InstallEventHandlers();

    nativeWebView->MinWidth = 0.0;
    nativeWebView->MinHeight = 0.0;
    nativeWebView->Visibility = Visibility::Visible;

    PlatformApi::Win10::AddXamlControl(window, nativeWebView);
    SetNativePositionAndSize(rectInWindowSpace, true); // After creation move native control offscreen
}

void WebViewControl::InstallEventHandlers()
{
    using ::Windows::Foundation::TypedEventHandler;
    using namespace ::Windows::UI::Xaml::Controls;

    // clang-format off
    std::weak_ptr<WebViewControl> self_weak(shared_from_this());
    // Install event handlers through lambdas as it seems only ref class's member functions can be event handlers directly
    auto navigationStarting = ref new TypedEventHandler<WebView^, WebViewNavigationStartingEventArgs^>([this, self_weak](WebView^ sender, WebViewNavigationStartingEventArgs^ args) {
        if (auto self = self_weak.lock())
            OnNavigationStarting(sender, args);
    });
    auto navigationCompleted = ref new TypedEventHandler<WebView^, WebViewNavigationCompletedEventArgs^>([this, self_weak](WebView^ sender, WebViewNavigationCompletedEventArgs^ args) {
        if (auto self = self_weak.lock())
            OnNavigationCompleted(sender, args);
    });
    auto unsupportedUriSchemeIdentified = ref new TypedEventHandler<WebView^, WebViewUnsupportedUriSchemeIdentifiedEventArgs^>([this, self_weak](WebView^ sender, WebViewUnsupportedUriSchemeIdentifiedEventArgs^ args) {
        if (auto self = self_weak.lock())
            OnUnsupportedUriSchemeIdentified(sender, args);
    });
    tokenNavigationStarting = nativeWebView->NavigationStarting += navigationStarting;
    tokenNavigationCompleted = nativeWebView->NavigationCompleted += navigationCompleted;
    tokenUnsupportedUriSchemeIdentified = nativeWebView->UnsupportedUriSchemeIdentified += unsupportedUriSchemeIdentified;
    // clang-format on
}

void WebViewControl::OnNavigationStarting(::Windows::UI::Xaml::Controls::WebView ^, ::Windows::UI::Xaml::Controls::WebViewNavigationStartingEventArgs ^ args)
{
    if (args->Uri != nullptr)
    {
        IUIWebViewDelegate::eAction whatToDo = HandleUriNavigation(args->Uri);
        switch (whatToDo)
        {
        case IUIWebViewDelegate::NO_PROCESS:
            args->Cancel = true;
            break;
        case IUIWebViewDelegate::PROCESS_IN_SYSTEM_BROWSER:
            ::Windows::System::Launcher::LaunchUriAsync(args->Uri);
            args->Cancel = true;
            break;
        default:
            break;
        }
    }
}

void WebViewControl::OnNavigationCompleted(::Windows::UI::Xaml::Controls::WebView ^ sender, ::Windows::UI::Xaml::Controls::WebViewNavigationCompletedEventArgs ^ args)
{
    String url;
    if (args->Uri != nullptr)
    {
        url = UTF8Utils::EncodeToUTF8(args->Uri->AbsoluteCanonicalUri->Data());
    }
    programmaticUrlNavigation = false;

    if (args->IsSuccess)
    {
        Logger::FrameworkDebug("[WebView] OnNavigationCompleted: url=%s", url.c_str());
    }
    else
    {
        Logger::Error("[WebView] OnNavigationCompleted failed: err_status=%d, url=%s", static_cast<int>(args->WebErrorStatus), url.c_str());
    }

    if (renderToTexture)
    {
        RenderToTexture();
    }

    auto self{ shared_from_this() };
    RunOnMainThreadAsync([this, self]() {
        if (uiWebView != nullptr && webViewDelegate != nullptr)
        {
            webViewDelegate->PageLoaded(uiWebView);
        }
    });
}

void WebViewControl::OnUnsupportedUriSchemeIdentified(::Windows::UI::Xaml::Controls::WebView ^, ::Windows::UI::Xaml::Controls::WebViewUnsupportedUriSchemeIdentifiedEventArgs ^ args)
{
    if (args->Uri != nullptr)
    {
        IUIWebViewDelegate::eAction whatToDo = HandleUriNavigation(args->Uri);
        args->Handled = whatToDo == IUIWebViewDelegate::NO_PROCESS;
        programmaticUrlNavigation = false;
    }
}

IUIWebViewDelegate::eAction WebViewControl::HandleUriNavigation(::Windows::Foundation::Uri ^ uri)
{
    String url = UTF8Utils::EncodeToUTF8(uri->AbsoluteCanonicalUri->Data());
    Logger::FrameworkDebug("[WebView] HandleUriNavigation: url=%s", url.c_str());

    // Delegate can hide UIWebView control which possesses this native control, but UI thread is blocked.
    // Also during delegate invocation user can click link multiple times and webview later will follow them.
    // Try to hide native webview while asking delegate to reduce cases described above.
    using ::Windows::UI::Xaml::Visibility;
    nativeWebView->Visibility = Visibility::Collapsed;

    bool redirectedByMouse = !programmaticUrlNavigation;
    IUIWebViewDelegate::eAction whatToDo = IUIWebViewDelegate::PROCESS_IN_WEBVIEW;
    RunOnMainThread([this, &whatToDo, &url, redirectedByMouse]() {
        if (uiWebView != nullptr && webViewDelegate != nullptr)
        {
            whatToDo = webViewDelegate->URLChanged(uiWebView, url, redirectedByMouse);
        }
    });

    nativeWebView->Visibility = Visibility::Visible;
    return whatToDo;
}

void WebViewControl::OnWindowSizeChanged(Window* w, Size2f windowSize, Size2f surfaceSize)
{
    properties.rectInWindowSpace = VirtualToWindow(properties.rect);
    properties.rectChanged = true;
    properties.anyPropertyChanged = true;
}

void WebViewControl::OnWindowDestroyed(Window* w)
{
    OwnerIsDying();
    window = nullptr;
}

void WebViewControl::ProcessProperties(const WebViewProperties& props)
{
    rectInWindowSpace = props.rectInWindowSpace;
    if (props.createNew)
    {
        CreateNativeControl();
    }
    if (props.anyPropertyChanged)
    {
        ApplyChangedProperties(props);
        if (renderToTexture && (props.rectChanged || props.backgroundTransparencyChanged || props.renderToTextureChanged))
        {
            RenderToTexture();
        }
    }
}

void WebViewControl::ApplyChangedProperties(const WebViewProperties& props)
{
    if (props.renderToTextureChanged)
        renderToTexture = props.renderToTexture;
    if (props.visibleChanged)
        visible = props.visible;

    if (props.rectChanged || props.visibleChanged || props.renderToTextureChanged)
        SetNativePositionAndSize(props.rectInWindowSpace, renderToTexture || !visible);
    if (props.backgroundTransparencyChanged)
        SetNativeBackgroundTransparency(props.backgroundTransparency);
    if (props.navigateTo != WebViewProperties::NAVIGATE_NONE)
        NativeNavigateTo(props);
    if (props.execJavaScript)
        NativeExecuteJavaScript(props.jsScript);
}

void WebViewControl::SetNativePositionAndSize(const Rect& rect, bool offScreen)
{
    float32 xOffset = 0.0f;
    float32 yOffset = 0.0f;
    if (offScreen)
    {
        // Move control very far offscreen as on phone control with disabled input remains visible
        xOffset = rect.x + rect.dx + 1000.0f;
        yOffset = rect.y + rect.dy + 1000.0f;
    }
    nativeWebView->Width = std::max(0.0f, rect.dx);
    nativeWebView->Height = std::max(0.0f, rect.dy);
    PlatformApi::Win10::PositionXamlControl(window, nativeWebView, rect.x - xOffset, rect.y - yOffset);
}

void WebViewControl::SetNativeBackgroundTransparency(bool enabled)
{
    using ::Windows::UI::Colors;
    nativeWebView->DefaultBackgroundColor = enabled ? Colors::Transparent : defaultBkgndColor;
}

void WebViewControl::NativeNavigateTo(const WebViewProperties& props)
{
    using ::Windows::Foundation::Uri;

    // WebView does not provide methods to determine whether navigation has occured by user click or
    // programmatically. So try to guess it myself using programmaticUrlNavigation flag.
    programmaticUrlNavigation = true;

    // clang-format off
    if (WebViewProperties::NAVIGATE_OPEN_URL == props.navigateTo)
    {
        Uri^ uri = ref new Uri(ref new Platform::String(UTF8Utils::EncodeToWideString(props.urlOrHtml).c_str()));
        nativeWebView->Navigate(uri);
    }
    else if (WebViewProperties::NAVIGATE_LOAD_HTML == props.navigateTo)
    {
        Platform::String^ html = ref new Platform::String(props.htmlString.c_str());
        nativeWebView->NavigateToString(html);
    }
    else if (WebViewProperties::NAVIGATE_OPEN_BUFFER == props.navigateTo)
    {
        // Generate some unique content identifier for each request
        // as WebViews' backend can remember content id and reuse UriResolver instance
        // for another WebView control
        uint32 generatedContentId = Random::Instance()->Rand();
        Platform::String^ contentId = ref new Platform::String(UTF8Utils::EncodeToWideString(Format("%u", generatedContentId)).c_str());

        UriResolver^ resolver = ref new UriResolver(props.urlOrHtml, props.basePath);
        Uri^ uri = nativeWebView->BuildLocalStreamUri(contentId, "/johny23");
        nativeWebView->NavigateToLocalStreamUri(uri, resolver);
    }
    // clang-format on
}

void WebViewControl::NativeExecuteJavaScript(const String& jsScript)
{
    using ::concurrency::create_task;
    using ::concurrency::task;

    // clang-format off
    Platform::String^ script = ref new Platform::String(UTF8Utils::EncodeToWideString(jsScript).c_str());

    auto args = ref new Platform::Collections::Vector<Platform::String^>();
    args->Append(script);

    auto js = nativeWebView->InvokeScriptAsync(L"eval", args);
    auto self{shared_from_this()};
    create_task(js).then([this, self](Platform::String^ result) {
        RunOnMainThreadAsync([this, self, result]() {
            if (webViewDelegate != nullptr && uiWebView != nullptr)
            {
                String jsResult = UTF8Utils::EncodeToUTF8(result->Data());
                webViewDelegate->OnExecuteJScript(uiWebView, jsResult);
            }
        });
    }).then([self](task<void> t) {
        try {
            t.get();
        } catch (Platform::Exception^ e) {
            // Exception can be thrown if a webpage has not been loaded into the WebView
            HRESULT hr = e->HResult;
            Logger::Error("[WebView] failed to execute JS: hresult=0x%08X, message=%s", hr, UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str());
        }
    });
    // clang-format on
}

Rect WebViewControl::VirtualToWindow(const Rect& srcRect) const
{
    VirtualCoordinatesSystem* vcs = window->GetUIControlSystem()->vcs;
    return vcs->ConvertVirtualToInput(srcRect);
}

void WebViewControl::RenderToTexture()
{
    using ::concurrency::create_task;
    using ::concurrency::task;
    using ::Windows::Storage::Streams::DataReader;
    using ::Windows::Storage::Streams::InMemoryRandomAccessStream;

    // clang-format off
    int32 width = static_cast<int32>(nativeWebView->Width);
    int32 height = static_cast<int32>(nativeWebView->Height);

    auto self{shared_from_this()};
    InMemoryRandomAccessStream^ inMemoryStream = ref new InMemoryRandomAccessStream();
    auto taskCapture = create_task(nativeWebView->CapturePreviewToStreamAsync(inMemoryStream));
    taskCapture.then([this, self, inMemoryStream, width, height]() {
        unsigned int streamSize = static_cast<unsigned int>(inMemoryStream->Size);
        DataReader^ reader = ref new DataReader(inMemoryStream->GetInputStreamAt(0));
        auto taskLoad = create_task(reader->LoadAsync(streamSize));
        taskLoad.then([this, self, reader, width, height, streamSize](task<unsigned int>) {
            size_t index = 0;
            Vector<uint8> buf(streamSize, 0);
            while (reader->UnconsumedBufferLength > 0)
            {
                buf[index] = reader->ReadByte();
                index += 1;
            }

            RefPtr<Sprite> sprite(CreateSpriteFromPreviewData(&buf[0], width, height));
            if (sprite.Valid())
            {
                RunOnMainThreadAsync([this, self, sprite]()
                {
                    if (uiWebView != nullptr)
                    {
                        UIControlBackground* bg = uiWebView->GetOrCreateComponent<UIControlBackground>();
                        bg->SetSprite(sprite.Get(), 0);
                    }
                });
            }
        });
    }).then([this, self](task<void> t) {
        try {
            t.get();
        } catch (Platform::COMException^ e) {
            HRESULT hr = e->HResult;
            Logger::Error("[WebView] RenderToTexture failed: 0x%08X", hr);
        }
    });
    // clang-format on
}

Sprite* WebViewControl::CreateSpriteFromPreviewData(uint8* imageData, int32 width, int32 height) const
{
    /*
        imageData is in-memory BMP file and starts with BITMAPFILEHEADER struct
        In WinRT application this struct is invisible for compiler

        struct BITMAPFILEHEADER {
            WORD    bfType;
            DWORD   bfSize;
            WORD    bfReserved1;
            WORD    bfReserved2;
            DWORD   bfOffBits;      // offset +10
        };
    */
    DWORD bitsOffset = *OffsetPointer<DWORD>(imageData, 10);
    uint8* dataPtr = imageData + bitsOffset;

    ScopedPtr<Image> imgSrc(Image::CreateFromData(width, height, FORMAT_RGBA8888, dataPtr));
    ImageConvert::SwapRedBlueChannels(imgSrc);
    return Sprite::CreateFromImage(imgSrc, true, false);
}

void WebViewControl::DeleteCookies(const String& url)
{
    using ::Windows::Foundation::Uri;
    using ::Windows::Foundation::Collections::IIterator;
    using namespace ::Windows::Web::Http;
    using namespace ::Windows::Web::Http::Filters;

    Uri ^ uri = ref new Uri(ref new Platform::String(UTF8Utils::EncodeToWideString(url).c_str()));
    HttpBaseProtocolFilter httpObj;
    HttpCookieManager ^ cookieManager = httpObj.CookieManager;
    HttpCookieCollection ^ cookies = cookieManager->GetCookies(uri);

    IIterator<HttpCookie ^> ^ it = cookies->First();
    while (it->HasCurrent)
    {
        HttpCookie ^ cookie = it->Current;
        cookieManager->DeleteCookie(cookie);
        it->MoveNext();
    }
}

String WebViewControl::GetCookie(const String& url, const String& name) const
{
    using ::Windows::Foundation::Uri;
    using ::Windows::Foundation::Collections::IIterator;
    using namespace ::Windows::Web::Http;
    using namespace ::Windows::Web::Http::Filters;

    String result;

    Uri ^ uri = ref new Uri(ref new Platform::String(UTF8Utils::EncodeToWideString(url).c_str()));
    HttpBaseProtocolFilter httpObj;
    HttpCookieCollection ^ cookies = httpObj.CookieManager->GetCookies(uri);

    Platform::String ^ cookieName = ref new Platform::String(UTF8Utils::EncodeToWideString(name).c_str());
    IIterator<HttpCookie ^> ^ it = cookies->First();
    while (it->HasCurrent)
    {
        HttpCookie ^ cookie = it->Current;
        if (cookie->Name == cookieName)
        {
            result = UTF8Utils::EncodeToUTF8(cookie->Value->Data());
            break;
        }
        it->MoveNext();
    }
    return result;
}

Map<String, String> WebViewControl::GetCookies(const String& url) const
{
    using ::Windows::Foundation::Uri;
    using ::Windows::Foundation::Collections::IIterator;
    using namespace ::Windows::Web::Http;
    using namespace ::Windows::Web::Http::Filters;

    Map<String, String> result;

    Uri ^ uri = ref new Uri(ref new Platform::String(UTF8Utils::EncodeToWideString(url).c_str()));
    HttpBaseProtocolFilter httpObj;
    HttpCookieCollection ^ cookies = httpObj.CookieManager->GetCookies(uri);

    IIterator<HttpCookie ^> ^ it = cookies->First();
    while (it->HasCurrent)
    {
        HttpCookie ^ cookie = it->Current;
        result.emplace(UTF8Utils::EncodeToUTF8(cookie->Name->Data()), UTF8Utils::EncodeToUTF8(cookie->Value->Data()));
        it->MoveNext();
    }
    return result;
}

} // namespace DAVA

#endif // (__DAVAENGINE_WIN_UAP__) && !(DISABLE_NATIVE_WEBVIEW)
