#ifndef __WEBVIEWTEST_TEST_H__
#define __WEBVIEWTEST_TEST_H__

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class WebViewTest : public BaseScreen
{
public:
    WebViewTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(float32 delta) override;

private:
    UIWebView* webView;
    UIControl* bgStubPanel;
    bool updateWait;

    void OnVisibleClick(BaseObject* sender, void* data, void* callerData);
};

#endif //__WEBVIEWTEST_TEST_H__
