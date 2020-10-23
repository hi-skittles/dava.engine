#include "Engine/Private/Win10/WindowImplWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#include "Engine/Private/Win10/WindowNativeBridgeWin10.h"
#include "Platform/DeviceInfo.h"

namespace DAVA
{
namespace Private
{
WindowImpl::WindowImpl(EngineBackend* engineBackend, Window* window)
    : engineBackend(engineBackend)
    , window(window)
    , mainDispatcher(engineBackend->GetDispatcher())
    , uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler), MakeFunction(this, &WindowImpl::TriggerPlatformEvents))
    , bridge(ref new WindowNativeBridge(this))
{
}

WindowImpl::~WindowImpl() = default;

void WindowImpl::Resize(float32 width, float32 height)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateResizeEvent(width, height));
}

void WindowImpl::Activate()
{
    // do nothing as it seems that win10 does not allow to activate window
}

void WindowImpl::Close(bool /*appIsTerminating*/)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateCloseEvent());
}

void WindowImpl::SetTitle(const String& title)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetTitleEvent(title));
}

void WindowImpl::SetMinimumSize(Size2f size)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateMinimumSizeEvent(size.dx, size.dy));
}

void WindowImpl::SetFullscreen(eFullscreen newMode)
{
    // Fullscreen mode cannot be changed on phones
    if (!PlatformCore::IsPhoneContractPresent())
    {
        uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetFullscreenEvent(newMode));
    }
}

void WindowImpl::RunAsyncOnUIThread(const Function<void()>& task)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void WindowImpl::RunAndWaitOnUIThread(const Function<void()>& task)
{
    uiDispatcher.SendEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void* WindowImpl::GetHandle() const
{
    return bridge->GetHandle();
}

bool WindowImpl::IsWindowReadyForRender() const
{
    return GetHandle() != nullptr;
}

void WindowImpl::TriggerPlatformEvents()
{
    if (uiDispatcher.HasEvents())
    {
        bridge->TriggerPlatformEvents();
    }
}

void WindowImpl::ProcessPlatformEvents()
{
    // Method executes in context of XAML::Window's UI thread
    uiDispatcher.ProcessEvents();
}

void WindowImpl::SetSurfaceScaleAsync(const float32 scale)
{
    DVASSERT(scale > 0.0f && scale <= 1.0f);

    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetSurfaceScaleEvent(scale));
}

void WindowImpl::BindXamlWindow(::Windows::UI::Xaml::Window ^ xamlWindow)
{
    // Method executes in context of XAML::Window's UI thread
    uiDispatcher.LinkToCurrentThread();
    bridge->BindToXamlWindow(xamlWindow);
}

void WindowImpl::UIEventHandler(const UIDispatcherEvent& e)
{
    // Method executes in context of XAML::Window's UI thread
    switch (e.type)
    {
    case UIDispatcherEvent::RESIZE_WINDOW:
        bridge->ResizeWindow(e.resizeEvent.width, e.resizeEvent.height);
        break;
    case UIDispatcherEvent::CLOSE_WINDOW:
        bridge->CloseWindow();
        break;
    case UIDispatcherEvent::SET_TITLE:
        bridge->SetTitle(e.setTitleEvent.title);
        delete[] e.setTitleEvent.title;
        break;
    case UIDispatcherEvent::SET_MINIMUM_SIZE:
        bridge->SetMinimumSize(e.resizeEvent.width, e.resizeEvent.height);
        break;
    case UIDispatcherEvent::SET_FULLSCREEN:
        bridge->SetFullscreen(e.setFullscreenEvent.mode);
        break;
    case UIDispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case UIDispatcherEvent::SET_CURSOR_CAPTURE:
        bridge->SetCursorCapture(e.setCursorCaptureEvent.mode);
        break;
    case UIDispatcherEvent::SET_CURSOR_VISIBILITY:
        bridge->SetCursorVisibility(e.setCursorVisibilityEvent.visible);
        break;
    case UIDispatcherEvent::SET_SURFACE_SCALE:
        bridge->SetSurfaceScale(e.setSurfaceScaleEvent.scale);
        break;
    default:
        break;
    }
}

void WindowImpl::SetCursorCapture(eCursorCapture mode)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetCursorCaptureEvent(mode));
}

void WindowImpl::SetCursorVisibility(bool visible)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetCursorVisibilityEvent(visible));
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
