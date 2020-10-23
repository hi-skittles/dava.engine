#include <DocDirSetup/DocDirSetup.h>

#include <Base/BaseTypes.h>
#include <Engine/Engine.h>
#include <Logger/Logger.h>
#include <Debug/DVAssertDefaultHandlers.h>

#include "CommandLineApplication.h"
#include "ArchivePackTool.h"
#include "ArchiveUnpackTool.h"
#include "ArchiveListTool.h"

int DAVAMain(DAVA::Vector<DAVA::String>)
{
    using namespace DAVA;

    AddHandler(Assert::DefaultLoggerHandler);
    AddHandler(Assert::DefaultDebuggerBreakHandler);

    const Vector<String> modules = {
        "JobManager",
        "NetCore",
        "LocalizationSystem"
    };

    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, modules, nullptr);

    const EngineContext* context = e.GetContext();

    DocumentsDirectorySetup::SetApplicationDocDirectory(context->fileSystem, "ResourceArchiver");

    context->logger->SetLogLevel(Logger::LEVEL_INFO);
    context->logger->EnableConsoleMode();

    e.update.Connect([&e](float32)
                     {
                         CommandLineApplication app("ResourceArchiver");
                         app.AddTool(std::make_unique<ArchivePackTool>());
                         app.AddTool(std::make_unique<ArchiveUnpackTool>());
                         app.AddTool(std::make_unique<ArchiveListTool>());
                         int retCode = app.Process(e.GetCommandLine());
                         e.QuitAsync(retCode);
                     });

    return e.Run();
}
