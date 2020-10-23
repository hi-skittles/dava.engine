#ifndef __UISCROLL_VIEW_TEST__
#define __UISCROLL_VIEW_TEST__

#include "Infrastructure/BaseScreen.h"

class TestBed;
class UIScrollViewTest : public BaseScreen
{
protected:
    ~UIScrollViewTest()
    {
    }

public:
    UIScrollViewTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

private:
    void ButtonPressed(DAVA::BaseObject* obj, void* data, void* callerData);

private:
    DAVA::UIButton* finishTestBtn;
    DAVA::UIStaticText* testMessageText;
    DAVA::UIScrollView* scrollView;
};

#endif /* defined(__UISCROLL_VIEW_TEST__) */
