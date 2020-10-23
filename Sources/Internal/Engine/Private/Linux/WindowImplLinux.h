#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_LINUX__)

#include "Base/Platform.h"
#include "Engine/EngineTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/Dispatcher/UIDispatcher.h"

namespace rhi
{
struct InitParam;
}

namespace DAVA
{
namespace Private
{
class WindowImpl final
{
public:
    WindowImpl(EngineBackend* engineBackend, Window* window);
    ~WindowImpl();

    bool Create(float32 width, float32 height);
    void Resize(float32 width, float32 height);
    void Activate();
    void Close(bool appIsTerminating);
    void SetTitle(const String& title);
    void SetMinimumSize(Size2f size);
    void SetFullscreen(eFullscreen newMode);

    void RunAsyncOnUIThread(const Function<void()>& task);
    void RunAndWaitOnUIThread(const Function<void()>& task);

    void* GetHandle() const;

    bool IsWindowReadyForRender() const;
    void InitCustomRenderParams(rhi::InitParam& params);

    void TriggerPlatformEvents();
    void ProcessPlatformEvents();

    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);

    void SetSurfaceScaleAsync(const float32 scale);

private:
    void UIEventHandler(const UIDispatcherEvent& e);

    EngineBackend* engineBackend = nullptr;
    Window* window = nullptr; // Window frontend reference
    UIDispatcher uiDispatcher; // Dispatcher that dispatches events to window UI thread
};

inline void* WindowImpl::GetHandle() const
{
    return nullptr;
}

inline void WindowImpl::InitCustomRenderParams(rhi::InitParam& /*params*/)
{
    // No custom render params
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_LINUX__
