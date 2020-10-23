#include "SingleTestFlowController.h"
#include "Infrastructure/GameCore.h"

SingleTestFlowController::SingleTestFlowController(const String& _testName, const BaseTest::TestParams& _testParams, bool _showUI)
    : showUI(_showUI)
    , testForRunName(_testName)
    , testParams(_testParams)
    , testForRun(nullptr)
    , testChooserScreen(new TestChooserScreen())
    , currentScreen(nullptr)
{
}

void SingleTestFlowController::Init(const Vector<BaseTest*>& _testChain)
{
    TestFlowController::Init(_testChain);

    if (testForRunName.empty())
    {
        testChooserScreen->SetTestChain(testChain);
        currentScreen = testChooserScreen;
    }
    else
    {
        for (auto* test : _testChain)
        {
            if (test->GetParams().sceneName == testForRunName)
            {
                test->MergeParams(testParams);
                test->ShowUI(showUI);

                testForRun = test;
            }
        }

        currentScreen = testForRun;

        if (nullptr == currentScreen)
        {
            Logger::Error(DAVA::Format("Test with name: %s not found", testForRunName.c_str()).c_str());
            GameCore::Instance()->Quit();
        }
    }
}

void SingleTestFlowController::BeginFrame()
{
    if (!currentScreen->IsRegistered())
    {
        currentScreen->RegisterScreen();
        currentScreen->OnStart();
    }

    currentScreen->BeginFrame();
}

void SingleTestFlowController::EndFrame()
{
    currentScreen->EndFrame();

    if (nullptr == testForRun)
    {
        if (testChooserScreen->IsFinished())
        {
            testForRun = testChooserScreen->GetTestForRun();
            testForRun->ShowUI(showUI);
            currentScreen = testForRun;
        }
    }
    else if (testForRun->IsFinished())
    {
        testForRun->OnFinish();

        Logger::Info("Finish all tests.");
        GameCore::Instance()->Quit();
    }
}