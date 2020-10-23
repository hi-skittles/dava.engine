#include "TestServer.h"

#include <Engine/Engine.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <Logger/Logger.h>

#include <NetworkCore.h>

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

    TestServer app(e);
    return e.Run();
}

TestServer::TestServer(Engine& engine)
    : engine(engine)
{
    engine.gameLoopStarted.Connect(this, &TestServer::OnLoopStarted);
    engine.gameLoopStopped.Connect(this, &TestServer::OnLoopStopped);
    engine.cleanup.Connect(this, &TestServer::OnEngineCleanup);

    engine.suspended.Connect(this, &TestServer::OnSuspended);
    engine.resumed.Connect(this, &TestServer::OnResumed);

    engine.update.Connect(this, &TestServer::OnUpdate);
}

void TestServer::OnLoopStarted()
{
    Logger::Debug("****** TestServer::OnGameLoopStarted");
}

void TestServer::OnLoopStopped()
{
    Logger::Debug("****** TestServer::OnGameLoopStopped");
}

void TestServer::OnEngineCleanup()
{
    Logger::Debug("****** TestServer::OnEngineCleanup");
}

void TestServer::OnSuspended()
{
    Logger::Debug("****** TestBed::OnSuspended");
}

void TestServer::OnResumed()
{
    Logger::Debug("****** TestBed::OnResumed");
}

void TestServer::OnUpdate(float32 frameDelta)
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
