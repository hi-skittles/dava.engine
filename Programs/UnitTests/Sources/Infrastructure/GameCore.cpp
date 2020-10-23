#include "Infrastructure/GameCore.h"

#include <DocDirSetup/DocDirSetup.h>

#include "CommandLine/CommandLineParser.h"
#include "Debug/DVAssert.h"
#include "Debug/DVAssertDefaultHandlers.h"
#include "Engine/Engine.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Logger/TeamCityTestsOutput.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/RHI/rhi_Public.h"
#include "UI/UIControlSystem.h"
#include "UnitTests/UnitTests.h"
#include "Utils/StringFormat.h"

#if defined(__DAVAENGINE_STEAM__)
#include "Platform/Steam.h"
#endif

#if defined(__DAVAENGINE_WIN_UAP__)
#include "Network/PeerDesription.h"
#include "Network/NetConfig.h"
#include <LoggerService/NetLogger.h>
#include <LoggerService/ServiceInfo.h>
#include "Platform/TemplateWin32/UAPNetworkHelper.h"
#endif

#include <Version/Version.h>

using namespace DAVA;

namespace
{
// List of names specifying which test classes should run.
// Names should be separated with ' ' or ',' or ';'
String runOnlyTheseTestClasses = ""; //""

// List of names specifying which test classes shouldn't run. This list takes precedence over runOnlyTheseTests.
// Names should be separated with ' ' or ',' or ';'
#if !defined(__DAVAENGINE_LINUX__)
String disableTheseTestClasses = "ScriptTest;ReflectionTest";
#else
// TODO: linux
// Linux unittests are running in console mode so disable tests dependent on GUI facilities (windows, input, etc)
String disableTheseTestClasses =
"ActionSystemTest;ReflectionTest"
;
#endif

bool teamcityOutputEnabled = false; // Flag whether to enable TeamCity output
bool teamcityCaptureStdout = false; // Flag whether to set TeamCity option 'captureStandardOutput=true'

const String testCoverageFileName = "Tests.cover";

} // unnamed namespace

int DAVAMain(Vector<String> cmdline)
{
    Assert::AddHandler(Assert::DefaultLoggerHandler);

    KeyedArchive* appOptions = new KeyedArchive();
    appOptions->SetInt32("rhi_threaded_frame_count", 2);
#if defined(__DAVAENGINE_QT__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_MACOS__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_IPHONE__)
    appOptions->SetInt32("renderer", rhi::ApiIsSupported(rhi::RHI_METAL) ? rhi::RHI_METAL : rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_WIN32__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("renderer", rhi::RHI_DX11);
#elif defined(__DAVAENGINE_ANDROID__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

#if defined(__DAVAENGINE_STEAM__)
    appOptions->SetUInt32(Steam::appIdPropertyKey, 0);
#endif

    Vector<String> modules = {
        "JobManager",
        "NetCore",
        "LocalizationSystem",
        "SoundSystem",
        "DownloadManager",
        "PackManager"
    };

    Engine e;
#if defined(__DAVAENGINE_LINUX__)
    appOptions->SetInt32("renderer", rhi::RHI_NULL_RENDERER);
    e.Init(eEngineRunMode::CONSOLE_MODE, modules, appOptions);
#else
    e.Init(eEngineRunMode::GUI_STANDALONE, modules, appOptions);
#endif

    FileSystem* fileSystem = e.GetContext()->fileSystem;

    DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "UnitTests");

    CommandLineParser::Instance()->SetFlags(cmdline);

    GameCore g(e);
    e.Run();
    return 0;
}

GameCore::GameCore(DAVA::Engine& e)
    : engine(e)
#if defined(__DAVAENGINE_WIN_UAP__)
    , netLogger(std::make_unique<Net::NetLogger>(true, 1000))
#endif
{
    engine.gameLoopStarted.Connect(this, &GameCore::OnAppStarted);
    engine.gameLoopStopped.Connect(this, &GameCore::OnAppFinished);
    engine.update.Connect(this, &GameCore::Update);
    if (engine.GetRunMode() != eEngineRunMode::CONSOLE_MODE)
    {
        engine.windowCreated.Connect(this, &GameCore::OnWindowCreated);
    }
}

void GameCore::Update(float32 timeElapsed)
{
    if (!isFinishing)
    {
        ProcessTests(timeElapsed);
    }
}

void GameCore::ProcessCommandLine()
{
    CommandLineParser* cmdline = CommandLineParser::Instance();
    if (cmdline->CommandIsFound("-only_test"))
    {
        runOnlyTheseTestClasses = cmdline->GetCommandParam("-only_test");
    }
    if (cmdline->CommandIsFound("-disable_test"))
    {
        disableTheseTestClasses = cmdline->GetCommandParam("-disable_test");
    }

    teamcityOutputEnabled = cmdline->CommandIsFound("-teamcity");
    teamcityCaptureStdout = cmdline->CommandIsFound("-teamcity_capture_stdout");
}

void GameCore::OnAppStarted()
{
    ProcessCommandLine();
#if defined(__DAVAENGINE_WIN_UAP__)
    InitNetwork();
#endif

    if (teamcityOutputEnabled)
    {
        teamCityOutput.reset(new TeamcityTestsOutput());
        teamCityOutput->SetCaptureStdoutFlag(teamcityCaptureStdout);
        engine.GetContext()->logger->AddCustomOutput(teamCityOutput.get());
    }

    UnitTests::TestCore::Instance()->Init(MakeFunction(this, &GameCore::OnTestClassStarted),
                                          MakeFunction(this, &GameCore::OnTestClassFinished),
                                          MakeFunction(this, &GameCore::OnTestStarted),
                                          MakeFunction(this, &GameCore::OnTestFinished),
                                          MakeFunction(this, &GameCore::OnTestFailed),
                                          MakeFunction(this, &GameCore::OnTestClassDisabled));
    if (!runOnlyTheseTestClasses.empty())
    {
        UnitTests::TestCore::Instance()->RunOnlyTheseTestClasses(runOnlyTheseTestClasses);
    }
    if (!disableTheseTestClasses.empty())
    {
        UnitTests::TestCore::Instance()->DisableTheseTestClasses(disableTheseTestClasses);
    }

    if (!UnitTests::TestCore::Instance()->HasTestClasses())
    {
        Logger::Error("%s", "There are no test classes");
        Quit(0);
    }
    else
    {
#if defined(TEST_COVERAGE)
        RefPtr<File> covergeFile(File::Create(testCoverageFileName, File::CREATE | File::WRITE));
        TEST_VERIFY(covergeFile);
        covergeFile->Flush();
#endif // __DAVAENGINE_MACOS__
    }

    if (engine.GetRunMode() == eEngineRunMode::CONSOLE_MODE)
    {
        // Register at least one resource size as some tests directly on indirectly access virtual coordinate system
        GetEngineContext()->uiControlSystem->vcs->RegisterAvailableResourceSize(1024, 768, "Gfx");
    }
}

void GameCore::OnWindowCreated(Window* w)
{
    String title = Version::CreateAppVersion("UnitTests");
    w->SetTitleAsync(title);
    w->SetVirtualSize(1024.f, 768.f);

    VirtualCoordinatesSystem* vcs = w->GetUIControlSystem()->vcs;
    vcs->RegisterAvailableResourceSize(1024, 768, "Gfx");
}

void GameCore::OnAppFinished()
{
    if (teamcityOutputEnabled)
    {
        engine.GetContext()->logger->RemoveCustomOutput(teamCityOutput.get());
        teamCityOutput.reset();
    }

#if defined(__DAVAENGINE_WIN_UAP__)
    UnInitNetwork();
#endif
}

void GameCore::OnError()
{
    DVASSERT_HALT();
}

void GameCore::OnTestClassStarted(const DAVA::String& testClassName)
{
    Logger::Info("%s", TeamcityTestsOutput::FormatTestClassStarted(testClassName).c_str());
}

void GameCore::OnTestClassFinished(const DAVA::String& testClassName)
{
    Logger::Info("%s", TeamcityTestsOutput::FormatTestClassFinished(testClassName).c_str());
}

void GameCore::OnTestClassDisabled(const DAVA::String& testClassName)
{
    Logger::Info("%s", TeamcityTestsOutput::FormatTestClassDisabled(testClassName).c_str());
}

void GameCore::OnTestStarted(const DAVA::String& testClassName, const DAVA::String& testName)
{
    Logger::Info("%s", TeamcityTestsOutput::FormatTestStarted(testClassName, testName).c_str());
}

void GameCore::OnTestFinished(const DAVA::String& testClassName, const DAVA::String& testName)
{
    Logger::Info("%s", TeamcityTestsOutput::FormatTestFinished(testClassName, testName).c_str());
}

void GameCore::OnTestFailed(const String& testClassName, const String& testName, const String& condition, const char* filename, int lineno, const String& userMessage)
{
    String errorString;
    if (userMessage.empty())
    {
        errorString = Format("%s:%d: %s", filename, lineno, testName.c_str());
    }
    else
    {
        errorString = Format("%s:%d: %s (%s)", filename, lineno, testName.c_str(), userMessage.c_str());
    }
    Logger::Error("%s", TeamcityTestsOutput::FormatTestFailed(testClassName, testName, condition, errorString).c_str());

    if (teamcityOutputEnabled == false)
    {
        // Call OnError here to log failed test condition
        OnError();
    }
}

void GameCore::ProcessTestCoverage()
{
#if defined(TEST_COVERAGE)
    // Output test coverage for sample
    Map<String, DAVA::UnitTests::TestCoverageInfo> map = UnitTests::TestCore::Instance()->GetTestCoverage();
    Logger::Info("Test coverage");

    for (const auto& x : map)
    {
        Logger::Info("  %s:", x.first.c_str());
        const Vector<String>& v = x.second.testFiles;
        for (const String& s : v)
        {
            Logger::Info("        %s", s.c_str());
        }
    }

    RefPtr<File> coverageFile(File::Create(testCoverageFileName, File::APPEND | File::WRITE));
    DVASSERT(coverageFile);

    auto toJson = [&coverageFile](const DAVA::String& item) { coverageFile->Write(item.c_str(), item.size()); };

    toJson("{ \n");

#if defined(DAVA_UNITY_FOLDER)
    toJson("    \"UnityFolder\": \"" + DAVA::String(DAVA_UNITY_FOLDER) + "\",\n");
#endif

    toJson("    \"Coverage\":  {\n");

    for (const auto& x : map)
    {
        toJson("         \"" + x.first + "\": \"");

        const Vector<String>& v = x.second.testFiles;
        for (const String& s : v)
        {
            toJson(s + (&s != &*v.rbegin() ? " " : ""));
        }

        toJson(x.first != map.rbegin()->first ? "\",\n" : "\"\n");
    }

    toJson("     },\n");

    toJson("    \"CoverageFolders\":  {\n");

    for (const auto& x : map)
    {
        const Vector<String>& v = x.second.testFiles;
        for (const String& s : v)
        {
            toJson("         \"" + s + "\": \"");

            auto mapTargetFolders = x.second.targetFolders;
            auto it = mapTargetFolders.find(s);
            String strPast;
            if (it != mapTargetFolders.end())
            {
                strPast = it->second;
            }
            else
            {
                strPast = mapTargetFolders.find("all")->second;
            }
            toJson(strPast);
            toJson(x.first != map.rbegin()->first || s != *v.rbegin() ? "\",\n" : "\"\n");
        }
    }

    toJson("     }\n");

    toJson("}\n");

#endif // TEST_COVERAGE
}

void GameCore::ProcessTests(float32 timeElapsed)
{
    if (!UnitTests::TestCore::Instance()->ProcessTests(timeElapsed))
    {
        ProcessTestCoverage();
        FinishTests();
    }
}

void GameCore::FinishTests()
{
    // Inform teamcity script we just finished all tests
    Logger::Debug("Finish all tests.");
    Quit(0);
}

void GameCore::Quit(int exitCode)
{
    if (!isFinishing)
    {
        engine.QuitAsync(exitCode);
        isFinishing = true;
    }
}

#if defined(__DAVAENGINE_WIN_UAP__)
void GameCore::InitNetwork()
{
    using namespace Net;

    // clang-format off
    auto loggerCreate = [this](uint32 serviceId, void*) -> IChannelListener* {
        if (!loggerInUse)
        {
            loggerInUse = true;
            return netLogger.get();
        }
        return nullptr;
    };
    auto loggerDestroy = [this](IChannelListener*, void*) -> void {
        loggerInUse = false;
    };
    // clang-format on

    NetCore::Instance()->RegisterService(DAVA::Net::LOG_SERVICE_ID, loggerCreate, loggerDestroy);

    eNetworkRole role = UAPNetworkHelper::GetCurrentNetworkRole();
    Net::Endpoint endpoint = UAPNetworkHelper::GetCurrentEndPoint();

    NetConfig config(role);
    config.AddTransport(TRANSPORT_TCP, endpoint);
    config.AddService(DAVA::Net::LOG_SERVICE_ID);

    netController = NetCore::Instance()->CreateController(config, nullptr);
}

void GameCore::UnInitNetwork()
{
    netLogger->Uninstall();
    // Run network loop untill all messages are sent to receiver
    while (netLogger->IsChannelOpen() && netLogger->GetMessageQueueSize() != 0)
    {
        Net::NetCore::Instance()->Poll();
    }

    Net::NetCore::Instance()->DestroyControllerBlocked(netController);
    netLogger.reset();
}

#endif // __DAVAENGINE_WIN_UAP__
