#include "Engine/Private/Mac/PlatformCoreMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSApplication.h>

#include "Engine/Engine.h"
#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Mac/CoreNativeBridgeMac.h"
#include "Engine/Private/Mac/WindowImplMac.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Private
{
PlatformCore::PlatformCore(EngineBackend* engineBackend)
    : engineBackend(engineBackend)
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
    bridge->Quit();
}

void PlatformCore::SetScreenTimeoutEnabled(bool enabled)
{
    if (enabled)
    {
        // Free the previous assertion, if any
        if (screenTimeoutAssertionId != kIOPMNullAssertionID)
        {
            const IOReturn releaseResult = IOPMAssertionRelease(screenTimeoutAssertionId);
            DVASSERT(releaseResult == kIOReturnSuccess);

            screenTimeoutAssertionId = kIOPMNullAssertionID;
        }
    }
    else if (screenTimeoutAssertionId == kIOPMNullAssertionID)
    {
        // Otherwise, create custom assertion if it hasn't been yet
        const IOReturn createResult = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                                                  kIOPMAssertionLevelOn,
                                                                  CFSTR("Dava Engine application is running"),
                                                                  &screenTimeoutAssertionId);
        DVASSERT(createResult == kIOReturnSuccess);
    }
}

int32 PlatformCore::OnFrame()
{
    return engineBackend->OnFrame();
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
