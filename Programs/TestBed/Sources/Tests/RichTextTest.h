#pragma once

#include "Infrastructure/BaseScreen.h"
#include <Base/RefPtr.h>
#include <UI/UIControl.h>
#include <UI/UITextField.h>

class TestBed;
class RichInputDelegate;

class RichTextTest : public BaseScreen
{
public:
    RichTextTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    RichInputDelegate* inputDelegate = nullptr;
    DAVA::RefPtr<DAVA::UITextField> inputField;
    DAVA::RefPtr<DAVA::UIControl> richText;
};
