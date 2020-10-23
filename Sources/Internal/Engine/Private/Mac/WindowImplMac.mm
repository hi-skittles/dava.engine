#include "Engine/Private/Mac/WindowImplMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSScreen.h>

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Mac/PlatformCoreMac.h"
#include "Engine/Private/Mac/WindowNativeBridgeMac.h"

#include "Logger/Logger.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
namespace Private
{
WindowImpl::WindowImpl(EngineBackend* engineBackend, Window* window)
    : engineBackend(engineBackend)
    , window(window)
    , mainDispatcher(engineBackend->GetDispatcher())
    , uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler))
    , bridge(new WindowNativeBridge(this))
{
}

WindowImpl::~WindowImpl() = default;

void* WindowImpl::GetHandle() const
{
    return bridge->renderView;
}

bool WindowImpl::Create(float32 width, float32 height)
{
    engineBackend->GetPlatformCore()->didHideUnhide.Connect(bridge.get(), &WindowNativeBridge::ApplicationDidHideUnhide);

    NSSize screenSize = [[NSScreen mainScreen] frame].size;
    float32 x = (screenSize.width - width) / 2.0f;
    float32 y = (screenSize.height - height) / 2.0f;
    return bridge->CreateWindow(x, y, width, height);
}

void WindowImpl::Activate()
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateActivateEvent());
}

void WindowImpl::Resize(float32 width, float32 height)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateResizeEvent(width, height));
}

void WindowImpl::Close(bool /*appIsTerminating*/)
{
    closeRequestByApp = true;
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
    bridge->TriggerPlatformEvents();
}

void WindowImpl::ProcessPlatformEvents()
{
    uiDispatcher.ProcessEvents();
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
        bridge->ResizeWindow(e.resizeEvent.width, e.resizeEvent.height);
        break;
    case UIDispatcherEvent::ACTIVATE_WINDOW:
        bridge->ActivateWindow();
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

void WindowImpl::WindowWillClose()
{
    engineBackend->GetPlatformCore()->didHideUnhide.Disconnect(bridge.get());
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
