#include "ClientApplication.h"

#include <DocDirSetup/DocDirSetup.h>

#include <Concurrency/Thread.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/LocalizationSystem.h>
#include <Job/JobManager.h>
#include <Logger/Logger.h>
#include <Network/NetCore.h>
#include <Engine/Engine.h>
#include <Debug/DVAssertDefaultHandlers.h>

using namespace DAVA;

int Process(Engine& e)
{
    const EngineContext* context = e.GetContext();

    DocumentsDirectorySetup::SetApplicationDocDirectory(context->fileSystem, "AssetCacheClient");

    context->logger->SetLogLevel(DAVA::Logger::LEVEL_INFO);
    context->logger->EnableConsoleMode();

    ClientApplication cacheClient;
    bool parsed = cacheClient.ParseCommandLine(e.GetCommandLine());
    if (parsed)
    {
        cacheClient.Process();
    }

    return static_cast<int>(cacheClient.GetExitCode());
}

int DAVAMain(Vector<String> cmdLine)
{
    Assert::AddHandler(Assert::DefaultLoggerHandler);
    Assert::AddHandler(Assert::DefaultDebuggerBreakHandler);

    Vector<String> modules =
    {
      "JobManager",
      "NetCore",
      "LocalizationSystem"
    };
    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, modules, nullptr);

    e.update.Connect([&e](float32)
                     {
                         int result = Process(e);
                         e.QuitAsync(result);
                     });

    return e.Run();
}
