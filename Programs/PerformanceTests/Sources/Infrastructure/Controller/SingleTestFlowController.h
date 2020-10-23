#ifndef __SINGLE_TEST_CONTROLLER_H__
#define __SINGLE_TEST_CONTROLLER_H__

#include "TestFlowController.h"
#include "Infrastructure/Screen/TestChooserScreen.h"

class SingleTestFlowController : public TestFlowController
{
public:
    SingleTestFlowController(const String& testName, const BaseTest::TestParams& testParams, bool showUI);

    void Init(const Vector<BaseTest*>& testChain) override;

    void BeginFrame() override;
    void EndFrame() override;

private:
    String testForRunName;
    bool showUI;

    BaseTest::TestParams testParams;
    BaseTest* testForRun;
    ScopedPtr<TestChooserScreen> testChooserScreen;

    BaseScreen* currentScreen;
};

#endif