#include "UIViewerApp.h"
#include "UIScreens/UIViewScreen.h"

#include <Debug/DVAssert.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <DeviceManager/DeviceManager.h>
#include <Engine/Engine.h>
#include <Engine/Window.h>
#include <FileSystem/KeyedArchive.h>
#include <Input/ActionSystem.h>
#include <Input/Keyboard.h>
#include <Network/ServicesProvider.h>
#include <Render/2D/Systems/VirtualCoordinatesSystem.h>
#include <Render/RHI/rhi_Public.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <UI/UIControlSystem.h>
#include <UI/UIScreenManager.h>
#include <Utils/StringFormat.h>

#include <DocDirSetup/DocDirSetup.h>
#include <LoggerService/NetLogger.h>
#include <LoggerService/ServiceInfo.h>
#include <Version/Version.h>

namespace UIViewerAppPrivate
{
static const DAVA::FastName SHOW_VIRTUAL_KEYBOARD_ACTION("showVirtualKeyboard");
}

UIViewerApp::UIViewerApp(DAVA::Engine& engine_, const DAVA::Vector<DAVA::String>& cmdLine)
    : engine(engine_)
    , options("options")
    , physicalToVirtualScale(1.f, 1.f)
    , servicesProvider(std::make_unique<DAVA::Net::ServicesProvider>(engine, "UIViewer"))
    , netLogger(std::make_shared<DAVA::Net::NetLogger>())
{
    using namespace DAVA;

    Assert::SetupDefaultHandlers();

    engine.gameLoopStarted.Connect(this, &UIViewerApp::OnAppStarted);
    engine.windowCreated.Connect(this, &UIViewerApp::OnWindowCreated);
    engine.gameLoopStopped.Connect(this, &UIViewerApp::OnAppFinished);
    engine.suspended.Connect(this, &UIViewerApp::OnSuspend);
    engine.resumed.Connect(this, &UIViewerApp::OnResume);
    engine.beginFrame.Connect(this, &UIViewerApp::BeginFrame);
    engine.endFrame.Connect(this, &UIViewerApp::EndFrame);

    options.AddOption("-project", VariantType(String("")), "Path to project folder");
    options.AddOption("-blankYaml", VariantType(String("")), "Path to placeholder yaml");
    options.AddOption("-blankRoot", VariantType(String("")), "Name of placeholder root control");
    options.AddOption("-blankPath", VariantType(String("")), "Path to placeholder control");
    options.AddOption("-testedYaml", VariantType(String("")), "Path to tested yaml");
    options.AddOption("-testedCtrl", VariantType(String("")), "Name of tested control");
    options.AddOption("-testedPath", VariantType(String("")), "Path of tested control");

    options.AddOption("-screenWidth", VariantType(1024), "Requested width of screen");
    options.AddOption("-screenHeight", VariantType(1024), "Requested height of screen");

    options.AddOption("-virtualWidth", VariantType(1024), "Requested virtual width of screen");
    options.AddOption("-virtualHeight", VariantType(1024), "Requested virtual height of screen");

    options.AddOption("-virtualKeyboardHeight", VariantType(0), "Requested height of virtual keyboard placeholder");

    options.AddOption("-fontsDir", VariantType(String("~res:/")), "Fonts Directory");

    options.AddOption("-locale", VariantType(String("en")), "Language");
    options.AddOption("-isRtl", VariantType(true), "Use rtl or no");
    options.AddOption("-isFlow", VariantType(false), "Load target yaml as UI Flow config");

    optionsAreParsed = options.Parse(cmdLine);

    QualitySettingsSystem::Instance()->Load("~res:/UIViewer/quality.yaml");
}

void UIViewerApp::OnAppStarted()
{
    using namespace DAVA;

    servicesProvider->AddService(DAVA::Net::LOG_SERVICE_ID, netLogger);
    servicesProvider->Start();

    ActionSystem* actionSystem = GetEngineContext()->actionSystem;
    actionSystem->ActionTriggered.Connect(this, &UIViewerApp::OnActionTriggered);

    Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
    if (kb != nullptr)
    {
        ActionSet set;
        set.name = "Shortcuts";

        DigitalBinding db;
        db.actionId = UIViewerAppPrivate::SHOW_VIRTUAL_KEYBOARD_ACTION;
        db.digitalStates[0] = DigitalElementState::JustPressed();
        db.digitalElements[0] = eInputElements::KB_F7;
        set.digitalBindings.push_back(db);

        actionSystem->BindSet(set, kb->GetId());
    }
}

void UIViewerApp::OnActionTriggered(DAVA::Action action)
{
    using namespace DAVA;

    if (action.actionId == UIViewerAppPrivate::SHOW_VIRTUAL_KEYBOARD_ACTION)
    {
        static bool keyboardShowed = false;
        Window* w = GetPrimaryWindow();
        int32 virtualKeyboardHeight = options.GetOption("-virtualKeyboardHeight").AsInt32();
        keyboardShowed = !keyboardShowed;
        if (keyboardShowed)
        {
            w->visibleFrameChanged.Emit(w, Rect(0, 0, w->GetSize().dx, static_cast<float32>(virtualKeyboardHeight)));
        }
        else
        {
            w->visibleFrameChanged.Emit(w, Rect(0, 0, w->GetSize().dx, w->GetSize().dy));
        }
    }
}

void UIViewerApp::OnWindowCreated(DAVA::Window* w)
{
    using namespace DAVA;

    engine.PrimaryWindow()->draw.Connect(this, &UIViewerApp::Draw);
    engine.PrimaryWindow()->sizeChanged.Connect(this, &UIViewerApp::OnWindowSizeChanged);

    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;

    const Size2i& physicalSize = vcs->GetPhysicalScreenSize();
    float32 screenAspect = static_cast<float32>(physicalSize.dx) / static_cast<float32>(physicalSize.dy);

    Size2f windowSize = { 1024.f, 1024.f / screenAspect };
    Size2i virtualSize(static_cast<int32>(windowSize.dx), static_cast<int32>(windowSize.dy));
    if (optionsAreParsed)
    {
        windowSize.dx = static_cast<float32>(options.GetOption("-screenWidth").AsInt32());
        windowSize.dy = static_cast<float32>(options.GetOption("-screenHeight").AsInt32());

        virtualSize.dx = options.GetOption("-virtualWidth").AsInt32();
        virtualSize.dy = options.GetOption("-virtualHeight").AsInt32();
    }

    physicalToVirtualScale.dx = static_cast<float32>(virtualSize.dx) / windowSize.dx;
    physicalToVirtualScale.dy = static_cast<float32>(virtualSize.dy) / windowSize.dy;

    String title = Version::CreateAppVersion("UI Viewer");
    w->SetTitleAsync(title);
    w->SetSizeAsync(windowSize);
    w->SetVirtualSize(static_cast<float32>(virtualSize.dx), static_cast<float32>(virtualSize.dy));

    Renderer::SetDesiredFPS(60);

    uiViewScreen = new UIViewScreen(w, (optionsAreParsed) ? &options : nullptr);
    GetEngineContext()->uiScreenManager->SetFirst(uiViewScreen->GetScreenID());
}

void UIViewerApp::OnWindowDestroyed(DAVA::Window* w)
{
    DAVA::GetEngineContext()->uiScreenManager->ResetScreen();
    SafeRelease(uiViewScreen);
}

void UIViewerApp::OnWindowSizeChanged(DAVA::Window* window, DAVA::Size2f size, DAVA::Size2f surfaceSize)
{
    using namespace DAVA;

    VirtualCoordinatesSystem* vcs = window->GetUIControlSystem()->vcs;
    Size2i vSize(static_cast<int32>(size.dx * physicalToVirtualScale.dx), static_cast<int32>(size.dy * physicalToVirtualScale.dy));

    vcs->SetVirtualScreenSize(vSize.dx, vSize.dy);
    vcs->UnregisterAllAvailableResourceSizes();
    vcs->RegisterAvailableResourceSize(vSize.dx, vSize.dy, "Gfx");
    vcs->RegisterAvailableResourceSize(vSize.dx * 2, vSize.dy * 2, "Gfx2");

    String title = Format("%s | [%d x %d] - [%d x %d]",
                          Version::CreateAppVersion("UI Viewer").c_str(),
                          static_cast<int32>(size.dx), static_cast<int32>(size.dy), vSize.dx, vSize.dy);
    window->SetTitleAsync(title);
}

void UIViewerApp::OnAppFinished()
{
    SafeRelease(uiViewScreen);
    servicesProvider.reset();
    netLogger.reset();
}

void UIViewerApp::OnSuspend()
{
}

void UIViewerApp::OnResume()
{
}

void UIViewerApp::BeginFrame()
{
}

void UIViewerApp::Draw(DAVA::Window* /*window*/)
{
}

void UIViewerApp::EndFrame()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////

DAVA::KeyedArchive* CreateOptions()
{
    DAVA::KeyedArchive* appOptions = new DAVA::KeyedArchive();

    appOptions->SetInt32("shader_const_buffer_size", 4 * 1024 * 1024);

    appOptions->SetInt32("max_index_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_vertex_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_const_buffer_count", 16 * 1024);
    appOptions->SetInt32("max_texture_count", 2048);
    appOptions->SetInt32("max_texture_set_count", 2048);
    appOptions->SetInt32("max_sampler_state_count", 128);
    appOptions->SetInt32("max_pipeline_state_count", 1024);
    appOptions->SetInt32("max_depthstencil_state_count", 256);
    appOptions->SetInt32("max_render_pass_count", 64);
    appOptions->SetInt32("max_command_buffer_count", 64);
    appOptions->SetInt32("max_packet_list_count", 64);
    appOptions->SetInt32("rhi_threaded_frame_count", 2);

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    //appOptions->SetInt32("renderer", rhi::RHI_METAL);
    appOptions->SetBool("iPhone_autodetectScreenScaleFactor", true);

#elif defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("bpp", 32);
    appOptions->SetInt32("renderer", rhi::RHI_DX11);

#else
#if defined(__DAVAENGINE_WIN32__)
    //appOptions->SetInt32("renderer", rhi::RHI_DX9);
    //appOptions->SetInt32("renderer", rhi::RHI_DX9);
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#elif defined(__DAVAENGINE_MACOS__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

    appOptions->SetInt32("bpp", 32);
#endif

    return appOptions;
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    DAVA::Assert::AddHandler(DAVA::Assert::DefaultLoggerHandler);
    DAVA::Assert::AddHandler(DAVA::Assert::DefaultDialogBoxHandler);

    DAVA::Vector<DAVA::String> modules =
    {
      "JobManager",
      "NetCore",
      "LocalizationSystem",
      "SoundSystem",
      "DownloadManager",
    };
    DAVA::Engine e;
    e.Init(DAVA::eEngineRunMode::GUI_STANDALONE, modules, CreateOptions());
    DAVA::FileSystem* fileSystem = e.GetContext()->fileSystem;

    DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "UIViewer");
    e.GetContext()->logger->SetLogFilename("UIViewer.txt");

    UIViewerApp app(e, cmdline);
    return e.Run();
}
