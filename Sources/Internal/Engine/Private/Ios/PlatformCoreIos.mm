#include "Engine/Private/Ios/PlatformCoreIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Ios/WindowImplIos.h"
#include "Engine/Private/Ios/CoreNativeBridgeIos.h"

namespace DAVA
{
namespace Private
{
PlatformCore::PlatformCore(EngineBackend* engineBackend)
    : engineBackend(engineBackend)
    , dispatcher(engineBackend->GetDispatcher())
    , bridge(new CoreNativeBridge(this))
{
}

PlatformCore::~PlatformCore() = default;

void PlatformCore::Init()
{
    engineBackend->InitializePrimaryWindow();
}

void PlatformCore::Run()
{
    bridge->Run();
}

void PlatformCore::PrepareToQuit()
{
    engineBackend->PostAppTerminate(true);
}

void PlatformCore::Quit()
{
    int exitCode = engineBackend->GetExitCode();
    engineBackend->OnGameLoopStopped();
    engineBackend->OnEngineCleanup();

    std::exit(exitCode);
}

void PlatformCore::SetScreenTimeoutEnabled(bool enabled)
{
    const BOOL idleTimerDisabled = enabled ? NO : YES;
    [UIApplication sharedApplication].idleTimerDisabled = idleTimerDisabled;
}

int32 PlatformCore::OnFrame()
{
    return engineBackend->OnFrame();
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
