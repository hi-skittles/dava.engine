#include "TestClient.h"

#include <Engine/Engine.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <Logger/Logger.h>

using namespace DAVA;

int DAVAMain(Vector<String> cmdline)
{
    Assert::SetupDefaultHandlers();

    eEngineRunMode runmode = eEngineRunMode::CONSOLE_MODE;

    Vector<String> modules
    {
      "JobManager"
    };

    Engine e;
    e.Init(runmode, modules, nullptr);

    TestClient app(e);
    return e.Run();
}

TestClient::TestClient(Engine& engine)
    : engine(engine)
{
    engine.gameLoopStarted.Connect(this, &TestClient::OnLoopStarted);
    engine.gameLoopStopped.Connect(this, &TestClient::OnLoopStopped);
    engine.cleanup.Connect(this, &TestClient::OnEngineCleanup);

    engine.suspended.Connect(this, &TestClient::OnSuspended);
    engine.resumed.Connect(this, &TestClient::OnResumed);

    engine.update.Connect(this, &TestClient::OnUpdate);
}

void TestClient::OnLoopStarted()
{
    Logger::Debug("****** TestClient::OnGameLoopStarted");
}

void TestClient::OnLoopStopped()
{
    Logger::Debug("****** TestClient::OnGameLoopStopped");
}

void TestClient::OnEngineCleanup()
{
    Logger::Debug("****** TestClient::OnEngineCleanup");
}

void TestClient::OnSuspended()
{
    Logger::Debug("****** TestClient::OnSuspended");
}

void TestClient::OnResumed()
{
    Logger::Debug("****** TestClient::OnResumed");
}

void TestClient::OnUpdate(float32 frameDelta)
{
    static int32 frameCount = 0;
    frameCount += 1;
    Logger::Debug("****** update: count=%d, delta=%f", frameCount, frameDelta);
    if (frameCount >= 100)
    {
        Logger::Debug("****** quit");
        engine.QuitAsync(0);
    }
}
