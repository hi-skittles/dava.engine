#include "Infrastructure/TestBed.h"

#include <DocDirSetup/DocDirSetup.h>

#include <Engine/AppInstanceMonitor.h>
#include <Engine/Engine.h>
#include <Engine/EngineSettings.h>

#include <Entity/ComponentManager.h>

#include <Reflection/ReflectionRegistrator.h>

#include <Render/RHI/rhi_Public.h>
#include <Render/RHI/dbg_Draw.h>

#include <CommandLine/CommandLineParser.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <Time/DateTime.h>
#include <Utils/Utils.h>
#include <DeviceManager/DeviceManager.h>
#include <UI/Render/UIRenderSystem.h>

#include "Infrastructure/TestListScreen.h"
#include "Tests/TextSystemTest.h"
#include "Tests/NotificationTest.h"
#include "Tests/UIScrollViewTest.h"
#include "Tests/SpeedLoadImagesTest.h"
#include "Tests/MultilineTest.h"
#include "Tests/StaticTextTest.h"
#include "Tests/StaticWebViewTest.h"
#include "Tests/MicroWebBrowserTest.h"
#include "Tests/UIMovieTest.h"
#include "Tests/FontTest.h"
#include "Tests/WebViewTest.h"
#include "Tests/FunctionSignalTest.h"
#include "Tests/GamepadTest.h"
#include "Tests/FullscreenTest.h"
#include "Tests/UIBackgroundTest.h"
#include "Tests/ClipTest.h"
#include "Tests/FloatingPointExceptionTest.h"
#include "Tests/DlcTest.h"
#include "Tests/FormatsTest.h"
#include "Tests/GPUTest.h"
#include "Tests/DLCManagerTest.h"
#include "Tests/ScriptingTest.h"
#include "Tests/SamplePluginTest.h"
#include "Tests/AssertTest.h"
#include "Tests/CoreV2Test.h"
#include "Tests/DeviceInfoTest.h"
#include "Tests/UILoggingTest.h"
#include "Tests/ProfilerTest.h"
#include "Tests/ImGuiTest.h"
#include "Tests/DeviceManagerTest.h"
#include "Tests/SoundTest.h"
#include "Tests/AnyPerformanceTest.h"
#include "Tests/OverdrawTest/OverdrawTesterComponent.h"
#include "Tests/OverdrawTest.h"
#include "Tests/WindowTest.h"
#include "Tests/UIStylesTest.h"
#include "Tests/InputSystemTest.h"
#include "Tests/RichTextTest.h"
#include "Tests/SkillSystemTest.h"
#include "Tests/DebugOverlayTest.h"
#include "Tests/UIJoypadSystemTest.h"
#include "Tests/TexturesLoadingTest.h"
#include "Tests/OnLowMemorySignalTest.h"
#include "Tests/FileActivationTest.h"
#include "Tests/FileWatcherTest.h"
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
#include "Tests/PhysicsTest.h"
#endif
#include "Tests/SpineTest.h"
#include "Tests/SkeletonAnimationTest.h"
#include "Tests/UIEntityMarkerTest.h"
//$UNITTEST_INCLUDE

#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include <MemoryProfilerService/ServiceInfo.h>
#include <MemoryProfilerService/MMNetServer.h>
#include <MemoryManager/MemoryProfiler.h>
#endif
#include <LoggerService/ServiceInfo.h>
#include <LoggerService/NetLogger.h>

#include <Version/Version.h>

#include "Infrastructure/NativeDelegateMac.h"
#include "Infrastructure/NativeDelegateIos.h"
#include "Infrastructure/NativeDelegateWin10.h"
#ifdef __DAVAENGINE_WIN_UAP__
#include <Platform/TemplateWin32/UAPNetworkHelper.h>
#endif

static void CheckDeviceInfoValid();
static const DAVA::DLCManager::IRequest* RequestAllFilesPack(DAVA::Engine& e);

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    using namespace DAVA;
    using namespace Net;

    // Instantiate Engine as CommandLineParser depends on it
    Engine e;

    AppInstanceMonitor* appInstanceMonitor = GetAppInstanceMonitor("{DD67F14C-3926-4BEE-BA40-463EE105C7C8}");
    appInstanceMonitor->PassActivationFilename(CommandLineParser::GetCommandParam("--file"));
    if (appInstanceMonitor->IsAnotherInstanceRunning())
    {
        return 0;
    }

    Assert::SetupDefaultHandlers();

    KeyedArchive* appOptions = new KeyedArchive();
    appOptions->SetInt32("bpp", 32);
    appOptions->SetInt32("rhi_threaded_frame_count", 2);
#if defined(__DAVAENGINE_QT__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_MACOS__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_IPHONE__)
    if (rhi::ApiIsSupported(rhi::Api::RHI_METAL))
    {
        appOptions->SetInt32("renderer", rhi::RHI_METAL);
    }
    else
    {
        appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    }
#elif defined(__DAVAENGINE_WIN32__)
    appOptions->SetInt32("renderer", rhi::RHI_DX9);
#elif defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("renderer", rhi::RHI_DX11);
#elif defined(__DAVAENGINE_ANDROID__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

    appOptions->SetBool("init_imgui", true);

    eEngineRunMode runmode = eEngineRunMode::GUI_STANDALONE;
    if (CommandLineParser::CommandIsFound("--console"))
    {
        runmode = eEngineRunMode::CONSOLE_MODE;
    }

    Vector<String> modules = {
        "JobManager",
        "NetCore",
        "LocalizationSystem",
        "SoundSystem",
        "DownloadManager",
        "PackManager"
    };

    GetEngineContext()->logger->EnableConsoleMode();
    e.Init(runmode, modules, appOptions);

    CheckDeviceInfoValid();

    TestBed game(e);
    return e.Run();
}

TestBed::TestBed(Engine& engine)
    : engine(engine)
    , currentScreen(nullptr)
    , testListScreen(nullptr)
{
    using namespace DAVA;

#if defined(__DAVAENGINE_MACOS__)
    RegisterMacApplicationListener();
#elif defined(__DAVAENGINE_IPHONE__)
    RegisterIosApplicationListener();
#elif defined(__DAVAENGINE_WIN_UAP__)
    nativeDelegate.reset(new NativeDelegateWin10());
    PlatformApi::Win10::RegisterXamlApplicationListener(nativeDelegate.get());
#endif

    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(OverdrawPerformanceTester::OverdrawTesterComponent);
    GetEngineContext()->componentManager->RegisterComponent<OverdrawPerformanceTester::OverdrawTesterComponent>();

    engine.gameLoopStarted.Connect(this, &TestBed::OnGameLoopStarted);
    engine.gameLoopStopped.Connect(this, &TestBed::OnGameLoopStopped);
    engine.cleanup.Connect(this, &TestBed::OnEngineCleanup);

    engine.suspended.Connect(this, &TestBed::OnSuspended);
    engine.resumed.Connect(this, &TestBed::OnResumed);

    if (engine.IsConsoleMode())
    {
        engine.update.Connect(this, &TestBed::OnUpdateConsole);
    }
    else
    {
        engine.windowCreated.Connect(this, &TestBed::OnWindowCreated);
        engine.windowDestroyed.Connect(this, &TestBed::OnWindowDestroyed);
        engine.backgroundUpdate.Connect(this, &TestBed::OnBackgroundUpdate);
        engine.update.Connect(this, &TestBed::OnUpdate);

        Window* w = engine.PrimaryWindow();
        w->sizeChanged.Connect(this, &TestBed::OnWindowSizeChanged);
        String title = Version::CreateAppVersion("TestBed");
        w->SetTitleAsync(title);
        w->SetSizeAsync({ 1024.f, 768.f });
    }

    const EngineContext* context = engine.GetContext();
    FileSystem* fileSystem = context->fileSystem;

    DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "TestBed");

    const DLCManager::IRequest* const pack = RequestAllFilesPack(engine);
    if (!pack->IsDownloaded())
    {
        Logger::Error("can't load pack with all files");
        DAVA_THROW(Exception, "can't load pack with all files");
    }

    context->settings->Load("~res:/TestBed/EngineSettings.yaml");

    servicesProvider.reset(new DAVA::Net::ServicesProvider(engine, "TestBed"));
    netLogger.reset(new DAVA::Net::NetLogger);
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memprofServer.reset(new DAVA::Net::MMNetServer);
#endif
}

void TestBed::OnGameLoopStarted()
{
    Logger::Debug("****** TestBed::OnGameLoopStarted");

    UIYamlLoader::LoadFonts("~res:/TestBed/UI/Fonts/fonts.yaml");

    startupActivationFilenames = engine.GetStartupActivationFilenames();
    if (!startupActivationFilenames.empty())
    {
        Logger::Debug("Testbed was started with files:");
        for (const String& filename : startupActivationFilenames)
        {
            Logger::Debug("      %s", filename.c_str());
        }
    }

    InitNetwork();
    RunOnlyThisTest();

    if (engine.IsConsoleMode())
    {
        RunOnMainThreadAsync([]() {
            Logger::Error("******** KABOOM on main thread********");
        });
    }
}

void TestBed::OnGameLoopStopped()
{
    using namespace DAVA;

    Logger::Debug("****** TestBed::OnGameLoopStopped");

    for (auto testScreen : screens)
    {
        SafeRelease(testScreen);
    }
    screens.clear();
    SafeRelease(testListScreen);

#if defined(__DAVAENGINE_WIN_UAP__)
    PlatformApi::Win10::UnregisterXamlApplicationListener(nativeDelegate.get());
#endif
}

void TestBed::OnEngineCleanup()
{
    Logger::Debug("****** TestBed::OnEngineCleanup");

    servicesProvider.reset();
    netLogger.reset();
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memprofServer.reset();
#endif

#if !defined(__DAVAENGINE_MACOS__)
    nativeDelegate.reset();
#endif
}

void TestBed::OnWindowCreated(DAVA::Window* w)
{
    Logger::Error("****** TestBed::OnWindowCreated");

    DAVA::int32 resW = 1024;
    DAVA::int32 resH = 768;

    float resDPI = 240.0;
    float virtualSizeScale = 1.0f;

    // For devices with very high dpi ( > resDPI) we will scale virtual size
    // to make it slightly smaller that for the regular devices.
    // In this way on very high dpi devices (usualy phones) all UI-contols
    // will be slightly larger.
    {
        float winDpi = w->GetDPI();
        if (winDpi > resDPI)
        {
            virtualSizeScale = std::max(0.75f, (resDPI / winDpi));
        }
    }

    float vw = static_cast<float>(resW * virtualSizeScale);
    float vh = static_cast<float>(resH * virtualSizeScale);

    w->SetVirtualSize(vw, vh);
    w->GetUIControlSystem()->vcs->RegisterAvailableResourceSize(resW, resH, "Gfx");
    w->GetUIControlSystem()->GetRenderSystem()->SetClearColor(Color::Black);

    LocalizationSystem* ls = LocalizationSystem::Instance();
    ls->SetDirectory("~res:/TestBed/Strings/");
    ls->SetCurrentLocale("en");
    ls->Init();

    testListScreen = new TestListScreen();
    GetEngineContext()->uiScreenManager->RegisterScreen(0, testListScreen);
    RegisterTests();
    RunTests();
}

void TestBed::OnWindowDestroyed(DAVA::Window* w)
{
    GetEngineContext()->uiScreenManager->ResetScreen();
    Logger::Error("****** TestBed::OnWindowDestroyed");
}

void TestBed::OnWindowSizeChanged(DAVA::Window* w, DAVA::Size2f size, DAVA::Size2f surfaceSize)
{
    Logger::Debug("********** TestBed::OnWindowSizeChanged: w=%.1f, h=%.1f, surfaceW=%.1f, surfaceH=%.1f", size.dx, size.dy, surfaceSize.dx, surfaceSize.dy);
}

void TestBed::OnSuspended()
{
    Logger::Error("****** TestBed::OnSuspended");
}

void TestBed::OnResumed()
{
    Logger::Error("****** TestBed::OnResumed");
}

void TestBed::OnUpdateConsole(DAVA::float32 frameDelta)
{
    static int frameCount = 0;
    frameCount += 1;
    Logger::Debug("****** update: count=%d, delta=%f", frameCount, frameDelta);
    if (frameCount >= 100)
    {
        Logger::Debug("****** quit");
        engine.QuitAsync(0);
    }
}

void TestBed::OnBackgroundUpdate(DAVA::float32 frameDelta)
{
    static float32 t = 0.f;
    t += frameDelta;
    if (t >= 2.f)
    {
        Logger::Debug("****** TestBed::OnBackgroundUpdate");
        t = 0.f;
    }
}

void TestBed::OnUpdate(DAVA::float32 frameDelta)
{
    // Output cpu temperature

    const int32 screenWidth = Renderer::GetFramebufferWidth();
    const int32 screenHeight = Renderer::GetFramebufferHeight();

    DbgDraw::EnsureInited();
    DbgDraw::SetScreenSize(screenWidth, screenHeight);
    DbgDraw::SetNormalTextSize();

    const float32 cpuTemperature = GetEngineContext()->deviceManager->GetCpuTemperature();
    DbgDraw::Text2D(screenWidth - 100, 0, rhi::NativeColorRGBA(1.0f, 1.0f, 1.0f, 0.5f), "CPU T: %.2f", cpuTemperature);

    rhi::RenderPassConfig passConfig;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = PRIORITY_MAIN_2D - 10;
    passConfig.viewport.x = 0;
    passConfig.viewport.y = 0;
    passConfig.viewport.width = screenWidth;
    passConfig.viewport.height = screenHeight;

    rhi::HPacketList packetList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(packetList);

    DbgDraw::FlushBatched(packetList);

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(pass);
}

void TestBed::RunOnlyThisTest()
{
    //runOnlyThisTest = "NotificationScreen";
}

void TestBed::OnError()
{
    DVASSERT_HALT();
}

void TestBed::RegisterTests()
{
    new FileWatcherTest(*this);
    new OnLowMemorySignalTest(*this);
    new CoreV2Test(*this);
    new FileActivationTest(*this);
    new DeviceManagerTest(*this);
    new DeviceInfoTest(*this);
    new DlcTest(*this);
    new OverdrawPerformanceTester::OverdrawTest(*this);
    new UIScrollViewTest(*this);
    new NotificationScreen(*this);
    new SpeedLoadImagesTest(*this);
    new MultilineTest(*this);
    new StaticTextTest(*this);
    new StaticWebViewTest(*this);
    new MicroWebBrowserTest(*this);
    new UIMovieTest(*this);
    new FontTest(*this);
    new WebViewTest(*this);
    new FunctionSignalTest(*this);
    new InputSystemTest(*this);
    new GamepadTest(*this);
    new FullscreenTest(*this);
    new UIBackgroundTest(*this);
    new ClipTest(*this);
    new GPUTest(*this);
    new FormatsTest(*this);
    new AssertTest(*this);
    new FloatingPointExceptionTest(*this);
    new DLCManagerTest(*this);
    new UILoggingTest(*this);
    new ProfilerTest(*this);
    new ScriptingTest(*this);
    new ImGuiTest(*this);
    new SoundTest(*this);
    new AnyPerformanceTest(*this);
    new SkillSystemTest(*this);
    new UIJoypadSystemTest(*this);
    new DebugOverlayTest(*this);

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)

    new SamplePluginTest(*this);

#endif

    new WindowTest(*this);
    new UIStylesTest(*this);
    new RichTextTest(*this);
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
    new PhysicsTest(*this);
#endif
    new TextSystemTest(*this);
    new SpineTest(*this);
    new SkeletonAnimationTest(*this);
    new TexturesLoadingTest(*this);
    new UIEntityMarkerTest(*this);
    //$UNITTEST_CTOR
}

void TestBed::RegisterScreen(BaseScreen* screen)
{
    GetEngineContext()->uiScreenManager->RegisterScreen(screen->GetScreenId(), screen);

    screens.push_back(screen);
    testListScreen->AddTestScreen(screen);
}

void TestBed::ShowStartScreen()
{
    currentScreen = nullptr;
    GetEngineContext()->uiScreenManager->SetScreen(0);
}

void TestBed::CreateDocumentsFolder()
{
    FilePath documentsPath = FileSystem::Instance()->GetUserDocumentsPath() + "TestBed/";

    FileSystem::Instance()->CreateDirectory(documentsPath, true);
    FileSystem::Instance()->SetCurrentDocumentsDirectory(documentsPath);
}

File* TestBed::CreateDocumentsFile(const String& filePathname)
{
    FilePath workingFilepathname = FilePath::FilepathInDocuments(filePathname);

    FileSystem::Instance()->CreateDirectory(workingFilepathname.GetDirectory(), true);

    File* retFile = File::Create(workingFilepathname, File::CREATE | File::WRITE);
    return retFile;
}

void TestBed::RunTests()
{
    if ("" != runOnlyThisTest)
    {
        for (auto screen : screens)
        {
            if (!IsNeedSkipTest(*screen))
            {
                currentScreen = screen;
            }
        }
    }
    else
    {
        currentScreen = nullptr;
    }

    if (nullptr != currentScreen)
    {
        GetEngineContext()->uiScreenManager->SetScreen(currentScreen->GetScreenId());
    }
    else
    {
        GetEngineContext()->uiScreenManager->SetScreen(0);
    }
}

bool TestBed::IsNeedSkipTest(const BaseScreen& screen) const
{
    if (runOnlyThisTest.empty())
    {
        return false;
    }

    const FastName& name = screen.GetName();

    return 0 != CompareCaseInsensitive(runOnlyThisTest, name.c_str());
}

void TestBed::InitNetwork()
{
    servicesProvider->AddService(DAVA::Net::LOG_SERVICE_ID, netLogger);
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    servicesProvider->AddService(DAVA::Net::MEMORY_PROFILER_SERVICE_ID, memprofServer);
#endif

    servicesProvider->Start();
}

void CheckDeviceInfoValid()
{
    using namespace DAVA;
    Logger::Info("device info begin==========================================");

    auto platform = DeviceInfo::GetPlatform();
    Logger::Info("platform enum index: %d", platform);
    DVASSERT(DeviceInfo::PLATFORM_UNKNOWN_VALUE != platform);

    auto platformString = DeviceInfo::GetPlatformString();
    Logger::Info("platform name: %s", platformString.c_str());
    DVASSERT(platformString != "Unknown");

    auto version = DeviceInfo::GetVersion();
    Logger::Info("version: %s", version.c_str());
    DVASSERT(version != "");

    auto manufacturer = DeviceInfo::GetManufacturer();
    Logger::Info("manufacturer: %s", manufacturer.c_str());

    auto model = DeviceInfo::GetModel();
    Logger::Info("model: %s", model.c_str());

    auto locale = DeviceInfo::GetLocale();
    Logger::Info("locale: %s", locale.c_str());
    DVASSERT(locale != "");

    auto region = DeviceInfo::GetRegion();
    Logger::Info("region: %s", region.c_str());
    DVASSERT(region != "");

    auto timezone = DeviceInfo::GetTimeZone();
    Logger::Info("timezone: %s", timezone.c_str());
    DVASSERT(timezone != "");

    auto udid = DeviceInfo::GetUDID();
    Logger::Info("udid: %s", udid.c_str());
    DVASSERT(udid != "");

    auto name = DeviceInfo::GetName();
    Logger::Info("name: %s", UTF8Utils::EncodeToUTF8(name).c_str());
    DVASSERT(name != L"");

    auto httpProxyHost = DeviceInfo::GetHTTPProxyHost();
    Logger::Info("http_proxy_host: %s", httpProxyHost.c_str());
    DVASSERT(httpProxyHost == "");

    auto httpNonProxyHosts = DeviceInfo::GetHTTPNonProxyHosts();
    Logger::Info("http_non_proxy_host: %s", httpNonProxyHosts.c_str());

    auto httpProxyPort = DeviceInfo::GetHTTPProxyPort();
    Logger::Info("http_proxy_port: %d", httpProxyPort);
    DVASSERT(httpProxyPort == 0);

    auto zbufferSize = DeviceInfo::GetZBufferSize();
    Logger::Info("zbuffer_size: %d", zbufferSize);
    DVASSERT(zbufferSize == 16 || zbufferSize == 24);

    auto gpuFamily = DeviceInfo::GetGPUFamily();
    Logger::Info("gpu_family enum index: %d", gpuFamily);
    DVASSERT(gpuFamily != GPU_INVALID);

    auto networkInfo = DeviceInfo::GetNetworkInfo();
    Logger::Info("network: type=%d signal_strength=%d", networkInfo.networkType, networkInfo.signalStrength);

    List<DeviceInfo::StorageInfo> storageInfo = DeviceInfo::GetStoragesList();
    for (const auto& info : storageInfo)
    {
        Logger::Info("storage info: type=%d total_space=%lld free_space=%lld, read_only=%d, removable=%d, emulated=%d",
                     info.type, info.totalSpace, info.freeSpace, info.readOnly, info.removable, info.emulated);
    }
    DVASSERT(storageInfo.size() > 0);

    uint32 cpuCount = DeviceInfo::GetCpuCount();
    Logger::Info("cpu_count: %d", cpuCount);
    DVASSERT(cpuCount > 0);
    Logger::Info("device info end============================================");
}

static const DAVA::DLCManager::IRequest* RequestAllFilesPack(DAVA::Engine& e)
{
    DLCManager* dlcManager = e.GetContext()->dlcManager;
    DLCManager::Hints hints;
    hints.localPacksDB = "~res:/TestBed/local_meta.db";
    dlcManager->Initialize("", "", hints);
    return dlcManager->RequestPack("all");
}
