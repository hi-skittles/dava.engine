#include "Base/Platform.h"

#if defined(__DAVAENGINE_QT__)
#elif defined(__DAVAENGINE_MACOS__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Mac/CoreNativeBridgeMac.h"
#include "Engine/Private/Mac/PlatformCoreMac.h"
#include "Engine/Private/Mac/WindowImplMac.h"
#include "Engine/Private/Mac/WindowNativeBridgeMac.h"

#import <AppKit/NSView.h>

namespace DAVA
{
namespace PlatformApi
{
namespace Mac
{
void AddNSView(Window* targetWindow, NSView* nsview)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    [wb->bridge->renderView addSubview:nsview];
}

void RemoveNSView(Window* targetWindow, NSView* nsview)
{
    [nsview removeFromSuperview];
}

void RegisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->bridge->RegisterDVEApplicationListener(listener);
}

void UnregisterDVEApplicationListener(id<DVEApplicationListener> listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->bridge->UnregisterDVEApplicationListener(listener);
}

} // namespace Mac
} // namespace PlatformApi
} // namespace DAVA

#endif // defined(__DAVAENGINE_MACOS__)
