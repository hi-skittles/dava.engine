#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN_UAP__)
#include "Network/NetCore.h"
#endif

#include <memory>

namespace DAVA
{
class Engine;
class TeamcityTestsOutput;
class Window;
namespace Net
{
class NetLogger;
}
}

class GameCore final
{
public:
    GameCore(DAVA::Engine& e);

    void OnAppStarted();
    void OnWindowCreated(DAVA::Window* w);
    void OnAppFinished();

    void Update(DAVA::float32 update);

private:
    void ProcessCommandLine();
    void ProcessTests(DAVA::float32 timeElapsed);
    void ProcessTestCoverage();
    void FinishTests();
    void Quit(int exitCode);

    void OnError();

    void OnTestClassStarted(const DAVA::String& testClassName);
    void OnTestClassFinished(const DAVA::String& testClassName);
    void OnTestClassDisabled(const DAVA::String& testClassName);
    void OnTestStarted(const DAVA::String& testClassName, const DAVA::String& testName);
    void OnTestFinished(const DAVA::String& testClassName, const DAVA::String& testName);
    void OnTestFailed(const DAVA::String& testClassName, const DAVA::String& testName, const DAVA::String& condition, const char* filename, int lineno, const DAVA::String& userMessage);

    DAVA::Engine& engine;
    std::unique_ptr<DAVA::TeamcityTestsOutput> teamCityOutput;
    bool isFinishing = false;

#if defined(__DAVAENGINE_WIN_UAP__)
    void InitNetwork();
    void UnInitNetwork();

    std::unique_ptr<DAVA::Net::NetLogger> netLogger;
    DAVA::Net::NetCore::TrackId netController;
    bool loggerInUse = false;
#endif
};
