#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_LINUX__)

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

    void Init();
    void Run();
    void PrepareToQuit();
    void Quit();

    void SetScreenTimeoutEnabled(bool enabled);

private:
    EngineBackend& engineBackend;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_LINUX__
