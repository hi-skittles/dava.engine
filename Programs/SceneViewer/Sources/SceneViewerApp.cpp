#include "SceneViewerApp.h"
#include "UIScreens/ViewSceneScreen.h"
#include "UIScreens/PerformanceResultsScreen.h"
#include "Quality/QualityPreferences.h"

#include <DocDirSetup/DocDirSetup.h>
#include <LoggerService/ServiceInfo.h>
#include <LoggerService/NetLogger.h>
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include <MemoryProfilerService/ServiceInfo.h>
#include <MemoryProfilerService/MMNetServer.h>
#endif

#include <Version/Version.h>

#include <Engine/Engine.h>
#include <Engine/Window.h>
#include <Network/NetCore.h>
#include <Render/RHI/rhi_Public.h>
#include <Render/RHI/dbg_Draw.h>
#include <Render/RHI/Common/dbg_StatSet.h>
#include <Render/RHI/Common/rhi_Private.h>
#include <Render/ShaderCache.h>
#include <Render/Material/FXCache.h>

SceneViewerApp::SceneViewerApp(DAVA::Engine& engine)
    : data({ engine })
{
    engine.gameLoopStarted.Connect(this, &SceneViewerApp::OnAppStarted);
    engine.windowCreated.Connect(this, &SceneViewerApp::OnWindowCreated);
    engine.windowDestroyed.Connect(this, &SceneViewerApp::OnWindowDestroyed);
    engine.gameLoopStopped.Connect(this, &SceneViewerApp::OnAppFinished);
    engine.suspended.Connect(this, &SceneViewerApp::OnSuspend);
    engine.resumed.Connect(this, &SceneViewerApp::OnResume);
    engine.beginFrame.Connect(this, &SceneViewerApp::BeginFrame);
    engine.endFrame.Connect(this, &SceneViewerApp::EndFrame);

    DAVA::FileSystem* fileSystem = engine.GetContext()->fileSystem;
    DAVA::FileSystem::eCreateDirectoryResult createResult = DAVA::DocumentsDirectorySetup::CreateApplicationDocDirectory(fileSystem, "SceneViewer");

    if (createResult != DAVA::FileSystem::DIRECTORY_EXISTS) // todo: remove this if-case some versions after
    {
        data.settings.Load(); // load from old doc directory
        DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "SceneViewer");
        data.settings.Save();
    }
    else
    {
        DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "SceneViewer");
        data.settings.Load();
    }

    DAVA::QualitySettingsSystem::Instance()->SetKeepUnusedEntities(true);
    DAVA::QualitySettingsSystem::Instance()->SetRuntimeQualitySwitching(true);

    DAVA::QualitySettingsSystem::Instance()->Load("~res:/SceneViewer/quality.yaml");

    QualityPreferences::LoadFromSettings(data.settings);
    data.scenePath = data.settings.GetLastOpenedScenePath();

    servicesProvider.reset(new DAVA::Net::ServicesProvider(engine, "SceneViewer"));
    netLogger.reset(new DAVA::Net::NetLogger);
    netLoggerDispatched.reset(new DAVA::Net::ChannelListenerDispatched(std::weak_ptr<DAVA::Net::IChannelListener>(netLogger), engine.GetContext()->netCore->GetNetEventsDispatcher()));
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memprofServer.reset(new DAVA::Net::MMNetServer);
    memprofServerDispatched.reset(new DAVA::Net::ChannelListenerDispatched(std::weak_ptr<DAVA::Net::IChannelListener>(memprofServer), engine.GetContext()->netCore->GetNetEventsDispatcher()));
#endif
}

void SceneViewerApp::OnAppStarted()
{
    DAVA::GetEngineContext()->soundSystem->InitFromQualitySettings();
}

void SceneViewerApp::OnWindowCreated(DAVA::Window* w)
{
    using namespace DAVA;

    data.engine.PrimaryWindow()->draw.Connect(this, &SceneViewerApp::Draw);

    const Size2i& physicalSize = GetEngineContext()->uiControlSystem->vcs->GetPhysicalScreenSize();
    data.screenAspect = static_cast<float32>(physicalSize.dx) / static_cast<float32>(physicalSize.dy);

    const Size2f windowSize = { 1024.f, 1024.f / data.screenAspect };

    const char* api = "";

    switch (rhi::HostApi())
    {
    case rhi::RHI_GLES2:
        api = "GLES2";
        break;
    case rhi::RHI_DX9:
        api = "DX9";
        break;
    case rhi::RHI_DX11:
        api = "DX11";
        break;
    case rhi::RHI_METAL:
        api = "Metal";
        break;
    case rhi::RHI_NULL_RENDERER:
        api = "NULL";
        break;
    case rhi::RHI_API_COUNT:
        break; // to shut up goddamn warning
    }

    String title = Version::CreateAppVersion("Scene Viewer");
    w->SetTitleAsync(title);
    w->SetSizeAsync(windowSize);
    w->SetVirtualSize(windowSize.dx, windowSize.dy);

    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;
    vcs->RegisterAvailableResourceSize(static_cast<int32>(windowSize.dx), static_cast<int32>(windowSize.dy), "Gfx");

    servicesProvider->AddService(DAVA::Net::LOG_SERVICE_ID, netLoggerDispatched);
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    servicesProvider->AddService(DAVA::Net::MEMORY_PROFILER_SERVICE_ID, memprofServerDispatched);
#endif

    servicesProvider->Start();

    Renderer::SetDesiredFPS(60);

    viewSceneScreen = new ViewSceneScreen(data);
#ifdef WITH_SCENE_PERFORMANCE_TESTS
    performanceResultsScreen = new PerformanceResultsScreen(data);
#endif

    //data.scenePath = "~doc:/05_amigosville_am/05_amigosville_am.sc2";
    //data.scenePath = "~doc:/06_rudniki_rd/06_rudniki_rd.sc2";
    //data.scenePath = "~doc:/09_savanna_sv/09_savanna_sv.sc2";
    //data.scenePath = "~doc:/10_asia_as/10_asia_as.sc2";
    //data.scenePath = "~doc:/12_malinovka_ma/12_malinovka_ma.sc2";
    //data.scenePath = "~doc:/13_pliego_pl/13_pliego_pl.sc2";
    //data.scenePath = "~doc:/14_port_pt/14_port_pt.sc2";
    //data.scenePath = "~doc:/15_ordeal_ord/15_ordeal_ord.sc2";
    //data.scenePath = "~doc:/18_canal_cn/18_canal_cn.sc2";
    //data.scenePath = "~doc:/19_himmelsdorf_hm/19_himmelsdorf_hm.sc2";
    //data.scenePath = "~doc:/20_lake_lk/20_lake_lk.sc2";
    //data.scenePath = "~doc:/21_mountain_mnt/21_mountain_mnt.sc2";
    //data.scenePath = "~doc:/23_karieri_kr/23_karieri_kr.sc2";
    //data.scenePath = "~doc:/24_milibase_mlb/24_milibase_mlb.sc2";
    //data.scenePath = "~doc:/25_canyon_ca/25_canyon_ca.sc2";
    //data.scenePath = "~doc:/";
    GetEngineContext()->uiScreenManager->SetFirst(viewSceneScreen->GetScreenID());
    //GetEngineContext()->uiScreenManager->SetFirst(selectSceneScreen->GetScreenID());

    DbgDraw::EnsureInited();
}

void SceneViewerApp::OnWindowDestroyed(DAVA::Window* w)
{
    if (viewSceneScreen)
        viewSceneScreen->UnloadResources();

    data.scene.reset();

    SafeRelease(viewSceneScreen);
#ifdef WITH_SCENE_PERFORMANCE_TESTS
    SafeRelease(performanceResultsScreen);
#endif
}

void SceneViewerApp::OnAppFinished()
{
    servicesProvider.reset();
    netLogger.reset();
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memprofServer.reset();
#endif

    DAVA::DbgDraw::Uninitialize();
}

void SceneViewerApp::OnSuspend()
{
}

void SceneViewerApp::OnResume()
{
}

void SceneViewerApp::BeginFrame()
{
}

void SceneViewerApp::Draw(DAVA::Window* /*window*/)
{
#if 0
    rhi::RenderPassConfig pass_desc;

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_NONE;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_NONE;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    pass_desc.priority = -10000;
    pass_desc.viewport.width = Renderer::GetFramebufferWidth();
    pass_desc.viewport.height = Renderer::GetFramebufferHeight();

    rhi::HPacketList pl;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, &pl);

    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(pl);
    DbgDraw::FlushBatched(pl, Matrix4(), Matrix4());
    rhi::EndPacketList(pl);
    rhi::EndRenderPass(pass);
#endif
}

void SceneViewerApp::EndFrame()
{
#if 0
    // stats must be obtained and reset AFTER frame is finished (and Present called)

    const char* backend = "";
    const uint32 color1 = rhi::NativeColorRGBA(0.9f, 0.9f, 1.0f, 1);
    const uint32 color2 = rhi::NativeColorRGBA(0.8f, 0.8f, 0.8f, 1);
    const int x0 = 10;
    const int y0 = 40;

    switch (rhi::HostApi())
    {
    case rhi::RHI_DX9:
        backend = "DX9";
        break;
    case rhi::RHI_DX11:
        backend = "DX11";
        break;
    case rhi::RHI_GLES2:
        backend = "GLES2";
        break;
    case rhi::RHI_METAL:
        backend = "Metal";
        break;
    }

    DbgDraw::SetScreenSize(VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dx, VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dy);
    //    DbgDraw::FilledRect2D( x0, y0, x0+13*DbgDraw::NormalCharW, y0+6*(DbgDraw::NormalCharH+1), rhi::NativeColorRGBA(0,0,0,0.4f) );
    DbgDraw::Text2D(x0, y0, rhi::NativeColorRGBA(1, 1, 1, 1), "RHI stats (%s)", backend);
    DbgDraw::Text2D(x0, y0 + 1 * (DbgDraw::NormalCharH + 1), color1, "  DIP     %u", StatSet::StatValue(rhi::stat_DIP));
    DbgDraw::Text2D(x0, y0 + 2 * (DbgDraw::NormalCharH + 1), color2, "  DP      %u", StatSet::StatValue(rhi::stat_DP));
    DbgDraw::Text2D(x0, y0 + 3 * (DbgDraw::NormalCharH + 1), color1, "  SET-PS  %u", StatSet::StatValue(rhi::stat_SET_PS));
    DbgDraw::Text2D(x0, y0 + 4 * (DbgDraw::NormalCharH + 1), color2, "  SET-TEX %u", StatSet::StatValue(rhi::stat_SET_TEX));
    DbgDraw::Text2D(x0, y0 + 5 * (DbgDraw::NormalCharH + 1), color1, "  SET-CB  %u", StatSet::StatValue(rhi::stat_SET_CB));

    StatSet::ResetAll();
#endif
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

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    //appOptions->SetInt32("renderer", rhi::RHI_METAL);
    appOptions->SetInt32("rhi_threaded_frame_count", 2);
    appOptions->SetBool("iPhone_autodetectScreenScaleFactor", true);

#elif defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("bpp", 32);
    appOptions->SetInt32("renderer", rhi::RHI_DX11);
    appOptions->SetInt32("rhi_threaded_frame_count", 2);

#else
#if defined(__DAVAENGINE_WIN32__)
    //appOptions->SetInt32("renderer", rhi::RHI_DX11);
    //appOptions->SetInt32("renderer", rhi::RHI_DX9);
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    appOptions->SetInt32("rhi_threaded_frame_count", 2);
#elif defined(__DAVAENGINE_MACOS__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

    //appOptions->SetInt("fullscreen.width",    1280);
    //appOptions->SetInt("fullscreen.height", 800);

    appOptions->SetInt32("bpp", 32);
#endif

    appOptions->SetBool("separate_net_thread", true);

    return appOptions;
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
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

    SceneViewerApp app(e);
    return e.Run();
}
