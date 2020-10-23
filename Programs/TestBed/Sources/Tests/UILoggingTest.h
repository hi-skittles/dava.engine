#pragma once

#include "Infrastructure/BaseScreen.h"

class TestBed;
class UILoggingTest : public BaseScreen
{
public:
    UILoggingTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    DAVA::UIButton* CreateUIButton(DAVA::Font* font, const DAVA::Rect& rect,
                                   void (UILoggingTest::*onClick)(DAVA::BaseObject*, void*, void*));
    void OnSwitch(DAVA::BaseObject* obj, void* data, void* callerData);
    void UpdateSwithButton();

    DAVA::RefPtr<DAVA::UIButton> switchButton;
};