#include "Engine/AppInstanceMonitor.h"

#include "Engine/Engine.h"
#include "Engine/Private/EngineBackend.h"

#if defined(__DAVAENGINE_WIN32__)
#include "Engine/Private/Win32/AppInstanceMonitorImplWin32.h"
#else
// clang-format off
namespace DAVA
{
namespace Private
{
class AppInstanceMonitorImpl final
{
public:
    AppInstanceMonitorImpl(AppInstanceMonitor*, const char*) {}
    bool IsAnotherInstanceRunning() const { return false; } // Note return value is false
    void PassActivationFilenameToAnotherInstance(const String&) {}
};
} // namespace Private
} // namespace DAVA
// clang-format on
#endif

namespace DAVA
{
AppInstanceMonitor::AppInstanceMonitor(const char* uniqueAppId)
    : engineBackend(Private::EngineBackend::Instance())
    , impl(std::make_unique<Private::AppInstanceMonitorImpl>(this, uniqueAppId))
{
}

AppInstanceMonitor::~AppInstanceMonitor() = default;

bool AppInstanceMonitor::IsAnotherInstanceRunning() const
{
    return impl->IsAnotherInstanceRunning();
}

void AppInstanceMonitor::PassActivationFilename(const String& filename)
{
    if (!filename.empty())
    {
        if (IsAnotherInstanceRunning())
        {
            impl->PassActivationFilenameToAnotherInstance(filename);
        }
        else
        {
            engineBackend->AddActivationFilename(filename);
        }
    }
}

void AppInstanceMonitor::ActivatedWithFilenameFromAnotherInstance(String filename)
{
    RunOnMainThreadAsync([ this, filename = std::move(filename) ]() mutable {
        engineBackend->AddActivationFilename(std::move(filename));
        engineBackend->OnFileActivated();

        Window* w = GetPrimaryWindow();
        if (w != nullptr)
        {
            w->ActivateAsync();
        }
    });
}

} // namespace DAVA
