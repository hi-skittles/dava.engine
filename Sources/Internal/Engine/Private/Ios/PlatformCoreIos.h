#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/EnginePrivateFwd.h"
#include "Functional/Signal.h"

namespace DAVA
{
namespace Private
{
class PlatformCore final
{
public:
    PlatformCore(EngineBackend* engineBackend);
    ~PlatformCore();

    void Init();
    void Run();
    void PrepareToQuit();
    void Quit();

    void SetScreenTimeoutEnabled(bool enabled);

    int32 OnFrame();

    // Signals for distribution UIApplicationDelegate's notifications:
    //  - applicationDidBecomeActive/applicationWillResignActive
    //  - applicationWillEnterForeground/applicationDidEnterBackground
    // WindowImpls usually connect to these signals to manage its focus
    // and visibility states
    Signal<bool> didBecomeResignActive;
    Signal<bool> didEnterForegroundBackground;

    EngineBackend* engineBackend = nullptr;
    MainDispatcher* dispatcher = nullptr;

    std::unique_ptr<CoreNativeBridge> bridge;

    // Friends
    friend struct CoreNativeBridge;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
