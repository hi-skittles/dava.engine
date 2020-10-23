#pragma once

#include "Settings.h"

#ifdef WITH_SCENE_PERFORMANCE_TESTS
#include <GridTest.h>
#endif
#include <NetworkHelpers/ChannelListenerDispatched.h>

#include <Network/ServicesProvider.h>
#include <DAVAEngine.h>
#include <Base/ScopedPtr.h>
#include <Scene3D/Scene.h>
#include <UI/UI3DView.h>
#include <FileSystem/FilePath.h>

namespace DAVA
{
class Engine;
class Window;
}

class ViewSceneScreen;
class PerformanceResultsScreen;

struct SceneViewerData
{
    DAVA::Engine& engine;
    Settings settings;
    DAVA::float32 screenAspect;
    DAVA::FilePath scenePath;
    DAVA::ScopedPtr<DAVA::Scene> scene;
#ifdef WITH_SCENE_PERFORMANCE_TESTS
    GridTestResult gridTestResult;
#endif
};

class SceneViewerApp final
{
public:
    SceneViewerApp(DAVA::Engine& e);

    void OnAppStarted();
    void OnWindowCreated(DAVA::Window* w);
    void OnWindowDestroyed(DAVA::Window* w);
    void OnAppFinished();

    void OnSuspend();
    void OnResume();

    void BeginFrame();
    void Draw(DAVA::Window* window);
    void EndFrame();

private:
    ViewSceneScreen* viewSceneScreen = nullptr;
    PerformanceResultsScreen* performanceResultsScreen = nullptr;

    SceneViewerData data;

    std::shared_ptr<DAVA::Net::NetService> netLogger;
    std::shared_ptr<DAVA::Net::IChannelListener> netLoggerDispatched;
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    std::shared_ptr<DAVA::Net::NetService> memprofServer;
    std::shared_ptr<DAVA::Net::IChannelListener> memprofServerDispatched;
#endif
    std::unique_ptr<DAVA::Net::ServicesProvider> servicesProvider;
};
