#pragma once

#include <Network/NetCore.h>
#include <Network/PeerDesription.h>
#include <Network/ServicesProvider.h>

namespace DAVA
{
class Engine;
class Window;
}

class TestData;
class BaseScreen;
class TestListScreen;
struct NativeDelegateWin10;

class TestBed
{
    struct ErrorData
    {
        DAVA::int32 line;
        DAVA::String command;
        DAVA::String filename;
        DAVA::String testName;
        DAVA::String testMessage;
    };

public:
    TestBed(DAVA::Engine& engine);

    DAVA::Engine& GetEngine() const;

    void OnGameLoopStarted();
    void OnGameLoopStopped();
    void OnEngineCleanup();

    void OnWindowSizeChanged(DAVA::Window* w, DAVA::Size2f size, DAVA::Size2f surfaceSize);
    void OnWindowCreated(DAVA::Window* w);
    void OnWindowDestroyed(DAVA::Window* w);

    void OnSuspended();
    void OnResumed();

    void OnUpdateConsole(DAVA::float32 frameDelta);
    void OnBackgroundUpdate(DAVA::float32 frameDelta);
    void OnUpdate(DAVA::float32 frameDelta);

    void RegisterScreen(BaseScreen* screen);
    void ShowStartScreen();

    DAVA::Vector<DAVA::String> startupActivationFilenames;

private:
    void RegisterTests();
    void RunTests();

    void CreateDocumentsFolder();
    DAVA::File* CreateDocumentsFile(const DAVA::String& filePathname);

    void RunOnlyThisTest();
    void OnError();
    bool IsNeedSkipTest(const BaseScreen& screen) const;

    // Network support
    void InitNetwork();

    DAVA::Engine& engine;

    DAVA::String runOnlyThisTest;

    BaseScreen* currentScreen;
    TestListScreen* testListScreen;

    DAVA::Vector<BaseScreen*> screens;

    std::shared_ptr<DAVA::Net::IChannelListener> netLogger;
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    std::shared_ptr<DAVA::Net::IChannelListener> memprofServer;
#endif

    std::unique_ptr<DAVA::Net::ServicesProvider> servicesProvider;

#if defined(__DAVAENGINE_MACOS__)
#elif defined(__DAVAENGINE_WIN_UAP__)
    std::unique_ptr<NativeDelegateWin10> nativeDelegate;
#else
    std::unique_ptr<char> nativeDelegate; // Dummy pointer on platforms which do not provide native delegates
#endif
};

inline DAVA::Engine& TestBed::GetEngine() const
{
    return engine;
}
