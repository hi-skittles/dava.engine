#ifndef __MULTILINETEST_TEST_H__
#define __MULTILINETEST_TEST_H__

#include "Infrastructure/BaseScreen.h"

namespace DAVA
{
class UITextField;
}

class TestBed;

class MultilineTest : public BaseScreen, public DAVA::UITextFieldDelegate
{
public:
    MultilineTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

    // UITextFieldDelegate interface
    void TextFieldOnTextChanged(DAVA::UITextField* textField, const DAVA::WideString& newText, const DAVA::WideString& oldText) override
    {
    }

private:
    void OnShowHideClick(BaseObject* sender, void* data, void* callerData);
    UIControl* topLayerControl = nullptr;

    DAVA::UIButton* CreateUIButton(DAVA::Font* font, const DAVA::Rect& rect, const DAVA::String& text,
                                   void (MultilineTest::*onClick)(DAVA::BaseObject*, void*, void*));

    DAVA::UITextField* textField1 = nullptr;
    DAVA::UITextField* textField2 = nullptr;
    DAVA::UITextField* textFieldMulti = nullptr;

    DAVA::UITextFieldDelegate* textDelegate1 = nullptr;
    DAVA::UITextFieldDelegate* textDelegate2 = nullptr;
};

#endif //__MULTILINETEST_TEST_H__
