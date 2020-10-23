#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <IOKit/pwr_mgt/IOPMLib.h>

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

    int OnFrame();

    // Through this signal WindowImpl gets notified about application hidden/unhidden state has changed
    // to update its visibility state
    Signal<bool> didHideUnhide;

    EngineBackend* engineBackend = nullptr;

    std::unique_ptr<CoreNativeBridge> bridge;

private:
    bool screenTimeoutEnabled = true;
    IOPMAssertionID screenTimeoutAssertionId = kIOPMNullAssertionID;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
