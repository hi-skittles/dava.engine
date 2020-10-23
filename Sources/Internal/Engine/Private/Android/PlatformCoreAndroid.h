#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Private/EnginePrivateFwd.h"

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

    void OnGamepadAdded(int32 deviceId, const String& name, bool hasTriggerButtons);
    void OnGamepadRemoved(int32 deviceId);

private:
    WindowImpl* ActivityOnCreate();
    void ActivityOnFileIntent(String filename, bool onStartup);
    void ActivityOnResume();
    void ActivityOnPause();
    void ActivityOnDestroy();
    void ActivityOnTrimMemory(int32 level);

    void GameThread();

private:
    EngineBackend* engineBackend = nullptr;
    MainDispatcher* mainDispatcher = nullptr;

    bool quitGameThread = false;

    int64 goBackgroundTimeRelativeToBoot = 0;
    int64 goBackgroundTime = 0;

    // Friends
    friend struct AndroidBridge;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
