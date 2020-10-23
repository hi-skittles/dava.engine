#include "Engine/Private/Linux/PlatformCoreLinux.h"

#if defined(__DAVAENGINE_LINUX__)

#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Linux/WindowImplLinux.h"

namespace DAVA
{
namespace Private
{
PlatformCore::PlatformCore(EngineBackend* engineBackend)
    : engineBackend(*engineBackend)
{
}

PlatformCore::~PlatformCore() = default;

void PlatformCore::Init()
{
    DVASSERT(0, "Implement PlatformCore::Init");
}

void PlatformCore::Run()
{
    DVASSERT(0, "Implement PlatformCore::Run");
}

void PlatformCore::PrepareToQuit()
{
    DVASSERT(0, "Implement PlatformCore::PrepareToQuit");
}

void PlatformCore::Quit()
{
    DVASSERT(0, "Implement PlatformCore::Quit");
}

void PlatformCore::SetScreenTimeoutEnabled(bool /*enabled*/)
{
    DVASSERT(0, "Implement PlatformCore::SetScreenTimeoutEnabled");
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_LINUX__
