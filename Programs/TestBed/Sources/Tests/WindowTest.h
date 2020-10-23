#pragma once

#include "Infrastructure/BaseScreen.h"
#include <Math/Rect.h>
#include <UI/UIControl.h>
#include <UI/UIStaticText.h>
#include <UI/UITextField.h>

class TestBed;
class TextFieldDelegate;
class WindowTest : public BaseScreen
{
public:
    WindowTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

    void UpdateKeyboardFrameSize(const DAVA::Rect& r);
    void UpdateVisibleFrameSize(const DAVA::Rect& r);

private:
    DAVA::RefPtr<DAVA::UIStaticText> visibleFrameRectText;
    DAVA::RefPtr<DAVA::UIStaticText> keyboardFrameRectText;
    DAVA::RefPtr<DAVA::UITextField> textField1;
    DAVA::RefPtr<DAVA::UITextField> textField2;
    TextFieldDelegate* tfDelegate = nullptr;
};
