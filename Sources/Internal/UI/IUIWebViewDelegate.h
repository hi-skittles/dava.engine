#pragma once
#include "Base/BaseTypes.h"

namespace DAVA
{
class UIWebView;
class IUIWebViewDelegate
{
public:
    virtual ~IUIWebViewDelegate()
    {
    }
    enum eAction
    {
        PROCESS_IN_WEBVIEW = 0,
        PROCESS_IN_SYSTEM_BROWSER,
        NO_PROCESS,
        ACTIONS_COUNT
    };

    virtual eAction URLChanged(UIWebView* webview, const String& newURL, bool isRedirectedByMouseClick) = 0;
    virtual void OnExecuteJScript(UIWebView* webview, const String& result)
    {
    }

    virtual void PageLoaded(UIWebView* webview) = 0;
    virtual void SwipeGesture(bool left)
    {
    }
};
};
