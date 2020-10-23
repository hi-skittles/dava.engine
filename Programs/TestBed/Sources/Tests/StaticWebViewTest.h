#ifndef __TEMPLATEPROJECTIPHONE__STATICWEBVIEWTEST__
#define __TEMPLATEPROJECTIPHONE__STATICWEBVIEWTEST__

#include "DAVAEngine.h"
#include "UI/UIWebView.h"

#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class MyWebViewDelegate;

class TestBed;
class StaticWebViewTest : public BaseScreen
{
protected:
    ~StaticWebViewTest(){};

public:
    StaticWebViewTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnButtonSetStatic(BaseObject* obj, void* data, void* callerData);
    void OnButtonSetNormal(BaseObject* obj, void* data, void* callerData);
    void OnButtonAdd10ToAlfa(BaseObject* obj, void* data, void* callerData);
    void OnButtonMinus10FromAlfa(BaseObject* obj, void* data, void* callerData);
    void OnButtonCheckTransparancy(BaseObject* obj, void* data, void* callerData);
    void OnButtonUncheckTransparancy(BaseObject* obj, void* data, void* callerData);
    void OnButtonExecJS(BaseObject* obj, void*, void*);
    void OnLoadHTMLString(BaseObject* obj, void*, void*);
    void OnButtonVisible(BaseObject*, void*, void*);
    void OnButtonHide(BaseObject*, void*, void*);

    UIButton* CreateUIButton(Font* font, const Rect& rect, const WideString& str,
                             void (StaticWebViewTest::*targetFunction)(BaseObject*, void*, void*));

    UIButton* setStaticButton = nullptr;
    UIButton* setNormalButton = nullptr;
    UIButton* add10ToAlfaButton = nullptr;
    UIButton* minus10FromAlfaButton = nullptr;
    UIButton* checkTransparancyButton = nullptr;
    UIButton* uncheckTransparancyButton = nullptr;
    UIButton* executeJSButton = nullptr;
    UIButton* loadHTMLString = nullptr;
    UIButton* setVisibleButton = nullptr;
    UIButton* setHideButton = nullptr;

    UIControl* overlapedImage = nullptr;

    UIWebView* webView1 = nullptr;
    UIWebView* webView2 = nullptr;
    UIWebView* webView3 = nullptr;

    MyWebViewDelegate* webviewDelegate = nullptr;
};

#endif /* defined(__TEMPLATEPROJECTIPHONE__STATICWEBVIEWTEST__) */
