#include "TestChainFlowController.h"
#include "Infrastructure/GameCore.h"

TestChainFlowController::TestChainFlowController(bool _showUI)
    : reportScreen(new ReportScreen())
    , currentScreen(nullptr)
    , currentTest(nullptr)
    , currentTestIndex(0)
    , showUI(_showUI)
    , testsFinished(false)

{
}

void TestChainFlowController::Init(const Vector<BaseTest*>& _testChain)
{
    TestFlowController::Init(_testChain);

    currentTestIndex = 0;
    currentTest = testChain[currentTestIndex];
    currentTest->ShowUI(showUI);
    currentScreen = currentTest;
}

void TestChainFlowController::BeginFrame()
{
    if (!currentScreen->IsRegistered())
    {
        currentScreen->RegisterScreen();
        currentScreen->OnStart();
    }

    currentScreen->BeginFrame();
}

void TestChainFlowController::EndFrame()
{
    currentScreen->EndFrame();

    if (!testsFinished)
    {
        if (currentTest->IsFinished())
        {
            currentTest->OnFinish();
            currentTestIndex++;

            testsFinished = testChain.size() == currentTestIndex;

            if (!testsFinished)
            {
                currentTest = testChain[currentTestIndex];
                currentTest->ShowUI(showUI);
                currentScreen = currentTest;
            }
        }
    }
    else
    {
        if (showUI)
        {
            reportScreen->SetTestChain(testChain);
            currentScreen = reportScreen;
        }
        else
        {
            Logger::Info("Finish all tests.");
            GameCore::Instance()->Quit();
        }
    }
}