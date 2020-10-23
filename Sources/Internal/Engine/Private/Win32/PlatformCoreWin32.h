#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_WIN32__)

#include "Base/Platform.h"
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

    PlatformCore(const PlatformCore&) = delete;
    PlatformCore& operator=(const PlatformCore&) = delete;

    static HINSTANCE Win32AppInstance();
    static void EnableHighResolutionTimer(bool enable);

    void Init();
    void Run();
    void PrepareToQuit();
    void Quit();

    void SetScreenTimeoutEnabled(bool enabled);

private:
    void QueryLowMemoryNotification();

    static HINSTANCE hinstance;

    EngineBackend& engineBackend;
    HANDLE lowMemoryNotificationHandle;
};

inline HINSTANCE PlatformCore::Win32AppInstance()
{
    return hinstance;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
