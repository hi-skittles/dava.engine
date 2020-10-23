#if defined(__DAVAENGINE_QT__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Qt/PlatformCoreQt.h"
#include "Engine/Private/Qt/WindowImplQt.h"

namespace DAVA
{
namespace PlatformApi
{
namespace Qt
{
void AcquireWindowContext(Window* targetWindow)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->AcquireContext();
}

void ReleaseWindowContext(Window* targetWindow)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    wb->ReleaseContext();
}

QApplication* GetApplication()
{
    using namespace DAVA::Private;
    return EngineBackend::Instance()->GetPlatformCore()->GetApplication();
}

RenderWidget* GetRenderWidget()
{
    using namespace DAVA::Private;
    return EngineBackend::Instance()->GetPlatformCore()->GetRenderWidget();
}

} // namespace Qt
} // namespace PlatformApi
} // namespace DAVA

#endif // defined(__DAVAENGINE_QT__)
