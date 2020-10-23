#include "Base/Platform.h"

#if defined(__DAVAENGINE_QT__)
#elif defined(__DAVAENGINE_WIN32__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Win32/WindowImplWin32.h"

namespace DAVA
{
namespace PlatformApi
{
namespace Win32
{
void SetWindowIcon(Window* targetWindow, int32 iconResourceId)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->SetIcon(MAKEINTRESOURCEW(iconResourceId));
}

void SetWindowCursor(Window* targetWindow, HCURSOR hcursor)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->SetCursor(hcursor);
}

} // namespace Win32
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
