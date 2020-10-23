#include "Engine/Private/Linux/WindowImplLinux.h"

#if defined(__DAVAENGINE_LINUX__)

namespace DAVA
{
namespace Private
{
WindowImpl::WindowImpl(EngineBackend* /*engineBackend*/, Window* /*window*/)
    : uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler), MakeFunction(this, &WindowImpl::TriggerPlatformEvents))
{
}

WindowImpl::~WindowImpl() = default;

bool WindowImpl::Create(float32 /*width*/, float32 /*height*/)
{
    DVASSERT(0, "Implement WindowImpl::Create");
    return false;
}

void WindowImpl::Resize(float32 width, float32 height)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateResizeEvent(width, height));
}

void WindowImpl::Activate()
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateActivateEvent());
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
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetFullscreenEvent(newMode));
}

void WindowImpl::RunAsyncOnUIThread(const Function<void()>& task)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void WindowImpl::RunAndWaitOnUIThread(const Function<void()>& task)
{
    uiDispatcher.SendEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

bool WindowImpl::IsWindowReadyForRender() const
{
    return GetHandle() != nullptr;
}

void WindowImpl::TriggerPlatformEvents()
{
    if (uiDispatcher.HasEvents())
    {
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

void WindowImpl::SetSurfaceScaleAsync(const float32 scale)
{
    DVASSERT(scale > 0.0f && scale <= 1.0f);

    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetSurfaceScaleEvent(scale));
}

void WindowImpl::UIEventHandler(const UIDispatcherEvent& e)
{
    switch (e.type)
    {
    case UIDispatcherEvent::RESIZE_WINDOW:
        break;
    case UIDispatcherEvent::ACTIVATE_WINDOW:
        break;
    case UIDispatcherEvent::CLOSE_WINDOW:
        break;
    case UIDispatcherEvent::SET_TITLE:
        delete[] e.setTitleEvent.title;
        break;
    case UIDispatcherEvent::SET_MINIMUM_SIZE:
        break;
    case UIDispatcherEvent::SET_FULLSCREEN:
        break;
    case UIDispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case UIDispatcherEvent::SET_CURSOR_CAPTURE:
        break;
    case UIDispatcherEvent::SET_CURSOR_VISIBILITY:
        break;
    case UIDispatcherEvent::SET_SURFACE_SCALE:
        break;
    default:
        break;
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_LINUX__
