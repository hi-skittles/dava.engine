#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/Dispatcher/UIDispatcher.h"
#include "Engine/EngineTypes.h"

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

    void SetSurfaceScaleAsync(const float32 scale);

    void BindXamlWindow(::Windows::UI::Xaml::Window ^ xamlWindow);

    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);

    void UIEventHandler(const UIDispatcherEvent& e);

    EngineBackend* engineBackend = nullptr;
    Window* window = nullptr; // Window frontend reference
    MainDispatcher* mainDispatcher = nullptr; // Dispatcher that dispatches events to DAVA main thread
    UIDispatcher uiDispatcher; // Dispatcher that dispatches events to window UI thread

    ref struct WindowNativeBridge ^ bridge = nullptr;
};

inline void WindowImpl::InitCustomRenderParams(rhi::InitParam& /*params*/)
{
    // No custom render params
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
