#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#include "Engine/Private/Win10/WindowImplWin10.h"
#include "Engine/Private/Win10/WindowNativeBridgeWin10.h"

namespace DAVA
{
namespace PlatformApi
{
namespace Win10
{
void AddXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->AddXamlControl(xamlControl);
}

void RemoveXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->RemoveXamlControl(xamlControl);
}

void PositionXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl, float32 x, float32 y)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->PositionXamlControl(xamlControl, x, y);
}

void UnfocusXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->bridge->UnfocusXamlControl();
}

::Windows::UI::Xaml::Input::Pointer ^ GetLastPressedPointer(Window* targetWindow)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    return wb->bridge->GetLastPressedPointer();
}

void RegisterXamlApplicationListener(XamlApplicationListener* listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->RegisterXamlApplicationListener(listener);
}

void UnregisterXamlApplicationListener(XamlApplicationListener* listener)
{
    using namespace DAVA::Private;
    PlatformCore* core = EngineBackend::Instance()->GetPlatformCore();
    core->UnregisterXamlApplicationListener(listener);
}

} // namespace Win10
} // namespace PlatformApi
} // namespace DAVA

#endif // defined(__DAVAENGINE_WIN_UAP__)
