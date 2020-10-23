#include "UI/Private/Ios/WebViewControl.Ios.h"

#if defined(__DAVAENGINE_IPHONE__) && !defined(DISABLE_NATIVE_WEBVIEW)

#import <UIKit/UIKit.h>

#include "Engine/Engine.h"
#include "Engine/PlatformApiIos.h"
#include "Render/2D/Sprite.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Image/Image.h"
#include "Render/Texture.h"
#include "UI/UIControlBackground.h"
#include "UI/UIControlSystem.h"
#include "UI/UIWebView.h"
#include "UI/IUIWebViewDelegate.h"

@interface WebViewURLDelegate : NSObject<UIWebViewDelegate>
{
    DAVA::IUIWebViewDelegate* delegate;
    DAVA::UIWebView* webView;
    DAVA::WebViewControl* webViewControl;
}

- (id)init;

- (void)setDelegate:(DAVA::IUIWebViewDelegate*)d andWebView:(DAVA::UIWebView*)w;

- (BOOL)webView:(UIWebView*)webView shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)navigationType;

- (void)webViewDidFinishLoad:(UIWebView*)webView;
- (void)webView:(UIWebView*)webView didFailLoadWithError:(NSError*)error;
- (void)leftGesture;
- (void)rightGesture;
- (void)setDAVAUIWebView:(DAVA::UIWebView*)uiWebControl;
- (void)onExecuteJScript:(NSArray*)result;
- (void)setDAVAWebViewControl:(DAVA::WebViewControl*)webViewControl;
@end

@implementation WebViewURLDelegate

- (id)init
{
    self = [super init];
    if (self)
    {
        delegate = nullptr;
        webView = nullptr;
        webViewControl = nullptr;
    }
    return self;
}

- (void)leftGesture
{
    if (delegate)
    {
        delegate->SwipeGesture(true);
    }
}

- (void)rightGesture
{
    if (delegate)
    {
        delegate->SwipeGesture(false);
    }
}

- (void)setDelegate:(DAVA::IUIWebViewDelegate*)d andWebView:(DAVA::UIWebView*)w
{
    delegate = d;
    webView = w;
}

- (BOOL)webView:(UIWebView*)webView shouldStartLoadWithRequest:(NSURLRequest*)request navigationType:(UIWebViewNavigationType)navigationType
{
    BOOL process = YES;

    if (delegate && self->webView)
    {
        NSString* url = [[request URL] absoluteString];

        if (url)
        {
            bool isRedirectedByMouseClick = navigationType == UIWebViewNavigationTypeLinkClicked;
            DAVA::IUIWebViewDelegate::eAction action = delegate->URLChanged(self->webView, [url UTF8String], isRedirectedByMouseClick);

            switch (action)
            {
            case DAVA::IUIWebViewDelegate::PROCESS_IN_WEBVIEW:
                DAVA::Logger::FrameworkDebug("PROCESS_IN_WEBVIEW");
                process = YES;
                break;

            case DAVA::IUIWebViewDelegate::PROCESS_IN_SYSTEM_BROWSER:
                DAVA::Logger::FrameworkDebug("PROCESS_IN_SYSTEM_BROWSER");
                [[UIApplication sharedApplication] openURL:[request URL]];
                process = NO;
                break;

            case DAVA::IUIWebViewDelegate::NO_PROCESS:
                DAVA::Logger::FrameworkDebug("NO_PROCESS");

            default:
                process = NO;
                break;
            }
        }
    }

    return process;
}

- (void)webViewDidFinishLoad:(UIWebView*)webViewParam
{
    DVASSERT(webViewControl);
    DVASSERT(webView);

    if (webViewControl && webView && webViewControl->IsRenderToTexture())
    {
        webViewControl->RenderToTextureAndSetAsBackgroundSpriteToControl(
        *webView);
    }

    if (delegate && self->webView)
    {
        delegate->PageLoaded(self->webView);
    }
}

- (void)webView:(UIWebView*)webView didFailLoadWithError:(NSError*)error
{
    DAVA::Logger::Error("WebView error: %s", [[error description] UTF8String]);
    if (delegate && self->webView)
    {
        delegate->PageLoaded(self->webView);
    }
}

- (void)onExecuteJScript:(NSString*)result
{
    if (delegate)
    {
        delegate->OnExecuteJScript(webView, DAVA::String([result UTF8String]));
    }
}

- (void)setDAVAUIWebView:(DAVA::UIWebView*)uiWebControl
{
    DVASSERT(uiWebControl);
    webView = uiWebControl;
}

- (void)setDAVAWebViewControl:(DAVA::WebViewControl*)control
{
    DVASSERT(control);
    webViewControl = control;
}

@end

namespace DAVA
{
struct WebViewControl::WebViewObjcBridge final
{
    ::UIWebView* nativeWebView = nullptr;
    WebViewURLDelegate* webViewDelegate = nullptr;
    UISwipeGestureRecognizer* rightSwipeGesture = nullptr;
    UISwipeGestureRecognizer* leftSwipeGesture = nullptr;
};

WebViewControl::WebViewControl(Window* w, UIWebView* uiWebView)
    : bridge(new WebViewObjcBridge)
    , window(w)
    , uiWebView(*uiWebView)
{
    bridge->nativeWebView = static_cast<::UIWebView*>(PlatformApi::Ios::GetUIViewFromPool(window, "UIWebView"));

    CGRect emptyRect = CGRectMake(0.0f, 0.0f, 0.0f, 0.0f);
    [bridge->nativeWebView setFrame:emptyRect];

    SetBounces(false);

    bridge->webViewDelegate = [[WebViewURLDelegate alloc] init];

    [bridge->nativeWebView setDelegate:bridge->webViewDelegate];
    [bridge->webViewDelegate setDAVAUIWebView:uiWebView];
    [bridge->webViewDelegate setDAVAWebViewControl:this];

    [bridge->nativeWebView becomeFirstResponder];
}

void WebViewControl::RenderToTextureAndSetAsBackgroundSpriteToControl(UIWebView& control)
{
    UIImage* nativeImage = PlatformApi::Ios::RenderUIViewToUIImage(bridge->nativeWebView);
    if (nativeImage != nullptr)
    {
        RefPtr<Image> image(PlatformApi::Ios::ConvertUIImageToImage(nativeImage));
        if (image != nullptr)
        {
            RefPtr<Texture> texture(Texture::CreateFromData(image.Get(), false));
            if (texture != nullptr)
            {
                uint32 width = image->GetWidth();
                uint32 height = image->GetHeight();
                Rect rect = control.GetRect();
                RefPtr<Sprite> sprite(Sprite::CreateFromTexture(texture.Get(), 0, 0, width, height, rect.dx, rect.dy));
                if (sprite != nullptr)
                {
                    UIControlBackground* bg = control.GetOrCreateComponent<UIControlBackground>();
                    bg->SetSprite(sprite.Get(), 0);
                }
            }
        }
    }
}

static const struct
{
    DAVA::UIWebView::eDataDetectorType davaDetectorType;
    NSUInteger systemDetectorType;
}
detectorsMap[] =
{
  { DAVA::UIWebView::DATA_DETECTOR_ALL, UIDataDetectorTypeAll },
  { DAVA::UIWebView::DATA_DETECTOR_NONE, UIDataDetectorTypeNone },
  { DAVA::UIWebView::DATA_DETECTOR_PHONE_NUMBERS, UIDataDetectorTypePhoneNumber },
  { DAVA::UIWebView::DATA_DETECTOR_LINKS, UIDataDetectorTypeLink },
  { DAVA::UIWebView::DATA_DETECTOR_ADDRESSES, UIDataDetectorTypeAddress },
  { DAVA::UIWebView::DATA_DETECTOR_CALENDAR_EVENTS, UIDataDetectorTypeCalendarEvent }
};

WebViewControl::~WebViewControl()
{
    SetGestures(false);
    ::UIWebView* innerWebView = bridge->nativeWebView;

    [innerWebView setDelegate:nil];
    [innerWebView stopLoading];
    [innerWebView loadHTMLString:@"" baseURL:nil];

    [innerWebView resignFirstResponder];

    PlatformApi::Ios::ReturnUIViewToPool(window, innerWebView);

    [bridge->webViewDelegate release];

    RestoreSubviewImages();
}

void WebViewControl::SetDelegate(IUIWebViewDelegate* delegate, DAVA::UIWebView* webView)
{
    [bridge->webViewDelegate setDelegate:delegate andWebView:webView];
}

void WebViewControl::Initialize(const Rect& rect)
{
    SetRect(rect);
}

// Open the URL requested.
void WebViewControl::OpenURL(const String& urlToOpen)
{
    NSString* nsURLPathToOpen = [NSString stringWithUTF8String:urlToOpen.c_str()];
    NSURL* url = [NSURL URLWithString:[nsURLPathToOpen stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

    NSURLRequest* requestObj = [NSURLRequest requestWithURL:url];
    ::UIWebView* innerWebView = bridge->nativeWebView;
    [innerWebView stopLoading];
    [innerWebView loadRequest:requestObj];
}

void WebViewControl::OpenFromBuffer(const String& string, const FilePath& basePath)
{
    NSString* dataToOpen = [NSString stringWithUTF8String:string.c_str()];
    NSString* baseUrl = [NSString stringWithUTF8String:basePath.AsURL().c_str()];

    ::UIWebView* innerWebView = bridge->nativeWebView;
    [innerWebView stopLoading];

    [innerWebView loadHTMLString:dataToOpen baseURL:[NSURL URLWithString:baseUrl]];
}

void WebViewControl::LoadHtmlString(const WideString& htlmString)
{
    NSString* htmlPageToLoad = [[[NSString alloc] initWithBytes:htlmString.data()
                                                         length:htlmString.size() * sizeof(wchar_t)
                                                       encoding:NSUTF32LittleEndianStringEncoding] autorelease];

    [bridge->nativeWebView loadHTMLString:htmlPageToLoad baseURL:nil];
}

void WebViewControl::DeleteCookies(const String& targetUrl)
{
    NSString* targetUrlString = [NSString stringWithUTF8String:targetUrl.c_str()];
    NSHTTPCookieStorage* cookies = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    // Delete all cookies for specified URL
    for (NSHTTPCookie* cookie in [cookies cookies])
    {
        if ([[cookie domain] rangeOfString:targetUrlString].location != NSNotFound)
        {
            [cookies deleteCookie:cookie];
        }
    }
    // Syncronized all changes with file system
    [[NSUserDefaults standardUserDefaults] synchronize];
}

String WebViewControl::GetCookie(const String& targetUrl, const String& name) const
{
    Map<String, String> cookiesMap = GetCookies(targetUrl);
    Map<String, String>::iterator cIter = cookiesMap.find(name);

    if (cIter != cookiesMap.end())
    {
        return cIter->second;
    }

    return String();
}

Map<String, String> WebViewControl::GetCookies(const String& targetUrl) const
{
    Map<String, String> resultMap;

    NSString* targetUrlString = [NSString stringWithUTF8String:targetUrl.c_str()];
    NSArray* cookiesArray = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:[NSURL URLWithString:targetUrlString]];

    for (NSHTTPCookie* cookie in cookiesArray)
    {
        String cookieName = [[cookie name] UTF8String];
        resultMap[cookieName] = [[cookie value] UTF8String];
    }

    return resultMap;
}

void WebViewControl::ExecuteJScript(const String& scriptString)
{
    NSString* jScriptString = [NSString stringWithUTF8String:
                                        scriptString.c_str()];

    NSString* resultString = [bridge->nativeWebView stringByEvaluatingJavaScriptFromString:jScriptString];

    [bridge->webViewDelegate performSelector:@selector(onExecuteJScript:)
                                  withObject:resultString
                                  afterDelay:0.0];
}

void WebViewControl::SetRect(const Rect& rect)
{
    Rect r = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(rect);
    [bridge->nativeWebView setFrame:CGRectMake(r.x, r.y, r.dx, r.dy)];
}

void WebViewControl::SetVisible(bool isVisible, bool hierarchic)
{
    pendingVisible = isVisible;

    // Workaround: call WillDraw instantly because it will not be called on SystemDraw
    if (!isVisible)
    {
        WillDraw();
    }
}

void WebViewControl::SetScalesPageToFit(bool isScalesToFit)
{
    [bridge->nativeWebView setScalesPageToFit:isScalesToFit];
}

void WebViewControl::SetBackgroundTransparency(bool enabled)
{
    ::UIWebView* webView = bridge->nativeWebView;
    [webView setOpaque:(enabled ? NO : YES)];

    UIColor* color = [webView backgroundColor];
    CGFloat r, g, b, a;
    [color getRed:&r green:&g blue:&b alpha:&a];

    if (enabled)
    {
        [webView setBackgroundColor:[UIColor colorWithRed:r
                                                    green:g
                                                     blue:b
                                                    alpha:0.f]];
        HideSubviewImages(webView);
    }
    else
    {
        [webView setBackgroundColor:[UIColor colorWithRed:r
                                                    green:g
                                                     blue:b
                                                    alpha:1.0f]];
        RestoreSubviewImages();
    }
}

void WebViewControl::HideSubviewImages(void* view)
{
    ::UIWebView* webView = bridge->nativeWebView;
    ::UIScrollView* scrollView = webView.scrollView;

    UIView* uiview = (UIView*)view;
    for (UIView* subview in [uiview subviews])
    {
        if (uiview == scrollView)
            continue;

        if ([subview isKindOfClass:[UIImageView class]])
        {
            subviewVisibilityMap[subview] = [subview isHidden];
            [subview setHidden:YES];
            [subview retain];
        }
        HideSubviewImages(subview);
    }
}

void WebViewControl::RestoreSubviewImages()
{
    Map<void*, bool>::iterator it;
    for (it = subviewVisibilityMap.begin(); it != subviewVisibilityMap.end(); ++it)
    {
        UIView* view = (UIView*)it->first;
        [view setHidden:it->second];
        [view release];
    }
    subviewVisibilityMap.clear();
}

bool WebViewControl::GetBounces() const
{
    return [[bridge->nativeWebView scrollView] bounces] == YES;
}

void WebViewControl::SetBounces(bool value)
{
    [[bridge->nativeWebView scrollView] setBounces:value ? YES : NO];
}

//for iOS we need use techique like http://stackoverflow.com/questions/12578895/how-to-detect-a-swipe-gesture-on-webview
void WebViewControl::SetGestures(bool value)
{
    UIView* backView = [bridge->nativeWebView superview];
    if (value && !gesturesEnabled)
    {
        WebViewURLDelegate* urlDelegate = bridge->webViewDelegate;

        bridge->rightSwipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:urlDelegate action:@selector(rightGesture)];
        bridge->leftSwipeGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:urlDelegate action:@selector(leftGesture)];
        bridge->rightSwipeGesture.direction = UISwipeGestureRecognizerDirectionRight;
        bridge->leftSwipeGesture.direction = UISwipeGestureRecognizerDirectionLeft;

        [backView addGestureRecognizer:bridge->rightSwipeGesture];
        [backView addGestureRecognizer:bridge->leftSwipeGesture];

        ::UIWebView* localWebView = bridge->nativeWebView;
        [localWebView.scrollView.panGestureRecognizer requireGestureRecognizerToFail:bridge->rightSwipeGesture];
        [localWebView.scrollView.panGestureRecognizer requireGestureRecognizerToFail:bridge->leftSwipeGesture];
    }
    else if (!value && gesturesEnabled)
    {
        [backView removeGestureRecognizer:bridge->rightSwipeGesture];
        [backView removeGestureRecognizer:bridge->leftSwipeGesture];
        [bridge->rightSwipeGesture release];
        [bridge->leftSwipeGesture release];
        bridge->rightSwipeGesture = nullptr;
        bridge->leftSwipeGesture = nullptr;
    }
    gesturesEnabled = value;
}

void WebViewControl::SetDataDetectorTypes(int32 value)
{
    NSUInteger systemDetectorTypes = 0;

    int detectorsCount = COUNT_OF(detectorsMap);
    for (int i = 0; i < detectorsCount; i++)
    {
        if ((value & detectorsMap[i].davaDetectorType) == detectorsMap[i].davaDetectorType)
        {
            systemDetectorTypes |= detectorsMap[i].systemDetectorType;
        }
    }

    [bridge->nativeWebView setDataDetectorTypes:systemDetectorTypes];
}

int32 WebViewControl::GetDataDetectorTypes() const
{
    NSUInteger systemDetectorTypes = [bridge->nativeWebView dataDetectorTypes];

    int32 davaDetectorTypes = 0;

    int detectorsCount = COUNT_OF(detectorsMap);
    for (int i = 0; i < detectorsCount; i++)
    {
        if ((systemDetectorTypes & detectorsMap[i].systemDetectorType) == detectorsMap[i].systemDetectorType)
        {
            davaDetectorTypes |= detectorsMap[i].davaDetectorType;
        }
    }

    return davaDetectorTypes;
}

void WebViewControl::SetRenderToTexture(bool value)
{
    pendingRenderToTexture = value;
}

void WebViewControl::WillDraw()
{
    if (isVisible != pendingVisible)
    {
        isVisible = pendingVisible;
        [bridge->nativeWebView setHidden:(isVisible ? NO : YES)];
    }

    if (isRenderToTexture != pendingRenderToTexture)
    {
        isRenderToTexture = pendingVisible;

        // hide windows - move to offScreenPos position
        // so it still can render WebView into
        DAVA::Rect r = uiWebView.GetRect();
        SetRect(r);

        if (isRenderToTexture)
        {
            // we have to show window or we can't render web view into texture
            if (!isVisible)
            {
                [bridge->nativeWebView setHidden:NO];
            }
            RenderToTextureAndSetAsBackgroundSpriteToControl(uiWebView);
            if (!isVisible)
            {
                [bridge->nativeWebView setHidden:YES];
            }
        }
    }
}

} // end namespace DAVA

#endif //defined(__DAVAENGINE_IPHONE__) && !defined(DISABLE_NATIVE_WEBVIEW)
