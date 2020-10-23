#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)

#include "Engine/Private/EnginePrivateFwd.h"
#include "Functional/Signal.h"

class QApplication;
namespace DAVA
{
class RenderWidget;
namespace Private
{
class PlatformCore final
{
public:
    PlatformCore(EngineBackend* engineBackend);
    ~PlatformCore();

    PlatformCore(const PlatformCore&) = delete;
    PlatformCore& operator=(const PlatformCore&) = delete;

    void Init();
    void Run();
    void PrepareToQuit();
    void Quit();

    void SetScreenTimeoutEnabled(bool enabled);

    QApplication* GetApplication();
    RenderWidget* GetRenderWidget();

private:
    EngineBackend& engineBackend;
    WindowImpl* primaryWindowImpl = nullptr;

    Signal<bool> applicationFocusChanged;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_QT__
