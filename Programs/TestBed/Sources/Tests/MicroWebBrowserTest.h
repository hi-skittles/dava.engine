#pragma once
#include "DAVAEngine.h"
#include <UI/UIWebView.h>
#include <Utils/FpsMeter.h>

#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class MicroWebBrowserTest : public BaseScreen, public UITextFieldDelegate
{
public:
    MicroWebBrowserTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

private:
    ~MicroWebBrowserTest() = default;

    void OnLoadPage(BaseObject* obj, void* data, void* callerData);

    // UITextFieldDelegate implementation
    void TextFieldShouldReturn(UITextField* /*textField*/) override;

    void Update(float elapsedTime) override;

    RefPtr<UIWebView> webView;
    RefPtr<UITextField> textField;

    UIStaticText* fpsText;
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    UIStaticText* memoryText;
#endif
    FpsMeter fpsMeter;
};
