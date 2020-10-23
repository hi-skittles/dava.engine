#pragma once

/**
    \defgroup engine_qt Engine facilities specific to Qt
*/

#if defined(__DAVAENGINE_QT__)

class QApplication;

namespace DAVA
{
class RenderWidget;
class Window;
namespace PlatformApi
{
namespace Qt
{
void AcquireWindowContext(Window* targetWindow);
void ReleaseWindowContext(Window* targetWindow);

QApplication* GetApplication();
RenderWidget* GetRenderWidget();

} // namespace Qt
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_QT__
