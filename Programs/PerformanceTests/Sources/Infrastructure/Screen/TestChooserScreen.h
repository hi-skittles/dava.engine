#ifndef __TEST_CHOOSER_SCREEN_H__
#define __TEST_CHOOSER_SCREEN_H__

#include "BaseScreen.h"
#include "Tests/BaseTest.h"

class TestChooserScreen : public BaseScreen
{
public:
    TestChooserScreen();

    void SetTestChain(const Vector<BaseTest*>& testsChain);
    bool IsFinished() const override;
    void OnFinish() override;

    BaseTest* GetTestForRun() const;

protected:
    void OnButtonPressed(BaseObject* obj, void* data, void* callerData);

    void LoadResources() override;

private:
    void CreateChooserUI();

    Vector<BaseTest*> testChain;
    BaseTest* testForRun;
};

inline void TestChooserScreen::SetTestChain(const Vector<BaseTest*>& _testsChain)
{
    testChain = _testsChain;
}

inline BaseTest* TestChooserScreen::GetTestForRun() const
{
    return testForRun;
}

#endif
