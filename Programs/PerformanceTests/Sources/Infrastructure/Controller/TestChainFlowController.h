#ifndef __TEST_CHAIN_FLOW_CONTROLLER_H__
#define __TEST_CHAIN_FLOW_CONTROLLER_H__

#include "TestFlowController.h"
#include "Infrastructure/Screen/ReportScreen.h"

class TestChainFlowController : public TestFlowController
{
public:
    TestChainFlowController(bool showUIReport);

    void Init(const Vector<BaseTest*>& testChain) override;

    void BeginFrame() override;
    void EndFrame() override;

private:
    ScopedPtr<ReportScreen> reportScreen;
    BaseScreen* currentScreen;
    BaseTest* currentTest;

    uint32 currentTestIndex;

    bool showUI;
    bool testsFinished;
};

#endif