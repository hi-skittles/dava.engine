#ifndef __DAVAENGINE_IWEBVIEWCONTROL_H__
#define __DAVAENGINE_IWEBVIEWCONTROL_H__

#include "Math/MathConstants.h"
#include "Math/Math2D.h"
#include "Math/Vector.h"
#include "Math/Rect.h"

#include "Logger/Logger.h"

namespace DAVA
{
class UIWebView;
class IUIWebViewDelegate;
class FilePath;
class UIGeometricData;

// Common interface for Web View Controls for different platforms.
class IWebViewControl
{
public:
    virtual ~IWebViewControl() = default;

    // Initialize the control.
    virtual void Initialize(const Rect& rect) = 0;
    virtual void OwnerIsDying()
    {
    }

    // Open the URL requested.
    virtual void OpenURL(const String& urlToOpen) = 0;
    // Load html page from string
    virtual void LoadHtmlString(const WideString& htmlString) = 0;
    // Delete all cookies associated with target URL
    virtual void DeleteCookies(const String& targetUrl)
    {
    }
    // Get cookie for specific domain and name
    virtual String GetCookie(const String& url, const String& name) const
    {
        return String();
    }
    // Get the list of cookies for specific domain
    virtual Map<String, String> GetCookies(const String& url) const
    {
        return Map<String, String>();
    }
    // Execute javascript command
    // if you need return data from javascript just
    // return JSON string you can parse it in c++
    // with yaml parser
    virtual void ExecuteJScript(const String& scriptString)
    {
    }

    virtual void OpenFromBuffer(const String& string, const FilePath& basePath) = 0;

    // Size/pos/visibility changes.
    virtual void SetRect(const Rect& rect) = 0;
    virtual void SetVisible(bool isVisible, bool hierarchic) = 0;
    // Page scale property change
    virtual void SetScalesPageToFit(bool isScalesToFit);

    virtual void SetDelegate(IUIWebViewDelegate* delegate, UIWebView* webView) = 0;
    virtual void SetBackgroundTransparency(bool enabled);

    // Bounces settings.
    virtual bool GetBounces() const
    {
        return false;
    }
    virtual void SetBounces(bool value)
    {
    }
    virtual void SetGestures(bool value)
    {
    }

    // Data detector types.
    virtual void SetDataDetectorTypes(int32 /*value*/)
    {
    }
    virtual int32 GetDataDetectorTypes() const
    {
        return 0;
    }

    virtual void SetRenderToTexture(bool value) = 0;
    virtual bool IsRenderToTexture() const = 0;

    // Draw events
    virtual void WillDraw()
    {
    }
    virtual void Draw(const UIGeometricData& geometricData)
    {
    }
    virtual void DidDraw()
    {
    }
    virtual void Input(class UIEvent* currentInput)
    {
    }

    virtual void Update()
    {
    }
};

inline void IWebViewControl::SetScalesPageToFit(bool isScalesToFit)
{
    Logger::FrameworkDebug("unsupported SetScalesPageToFit");
};

inline void IWebViewControl::SetBackgroundTransparency(bool enabled)
{
    Logger::FrameworkDebug("unsupported SetBackgroundTransparency");
};
};

#endif // __DAVAENGINE_IWEBVIEWCONTROL_H__
