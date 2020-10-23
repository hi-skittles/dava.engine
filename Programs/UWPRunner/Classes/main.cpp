#if defined(__DAVAENGINE_WIN32__)

#include <DocDirSetup/DocDirSetup.h>

#include "Debug/DVAssertDefaultHandlers.h"
#include "Logger/Logger.h"
#include "Engine/Engine.h"

#include "UWPRunner.h"

using namespace DAVA;

int Process(Engine& e)
{
    bool succeed = false;
    PackageOptions options = ParseCommandLine();
    if (CheckOptions(options))
    {
        try
        {
            UWPRunner runner(options);
            runner.Run();
            succeed = runner.IsSucceed();
        }
        catch (std::exception& e)
        {
            DAVA::Logger::Error("%s", e.what());
        }
    }

    return succeed ? 0 : 1;
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    Assert::SetupDefaultHandlers();

    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, { "NetCore" }, nullptr);

    e.update.Connect([&e](float32)
                     {
                         int result = Process(e);
                         e.QuitAsync(result);
                     });

    DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(e.GetContext()->fileSystem, "UWPRunner");

    return e.Run();
}

#endif // defined(__DAVAENGINE_WIN32__)