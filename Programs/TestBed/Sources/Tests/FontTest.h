#ifndef __FONTTEST_TEST_H__
#define __FONTTEST_TEST_H__

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class FontTest : public BaseScreen
{
public:
    FontTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

private:
    FTFont* ftFont;
    GraphicFont* dfFont;
    GraphicFont* graphicFont;

    void OnFontSelectClick(BaseObject* sender, void* data, void* callerData);
    void OnFontSizeClick(BaseObject* sender, void* data, void* callerData);
    void UpdateFontSizeText();

    UIStaticText* previewText;
    UIStaticText* sizeText;
    UITextField* inputText;
    UITextFieldDelegate* inputDelegate;
};

#endif //__FONTTEST_TEST_H__
