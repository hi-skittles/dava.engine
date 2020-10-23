#include "Engine/Private/Ios/WindowImplIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Ios/PlatformCoreIos.h"
#include "Engine/Private/Ios/WindowNativeBridgeIos.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
namespace Private
{
WindowImpl::WindowImpl(EngineBackend* engineBackend, Window* window)
    : engineBackend(engineBackend)
    , window(window)
    , mainDispatcher(engineBackend->GetDispatcher())
    , uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler), MakeFunction(this, &WindowImpl::TriggerPlatformEvents))
    , bridge(new WindowNativeBridge(this, engineBackend->GetOptions()))
{
}

WindowImpl::~WindowImpl()
{
    PlatformCore* core = engineBackend->GetPlatformCore();
    core->didBecomeResignActive.Disconnect(appBecomeOrResignActiveToken);
    core->didEnterForegroundBackground.Disconnect(appDidEnterForegroundOrBackgroundToken);
}

void* WindowImpl::GetHandle() const
{
    return bridge->GetHandle();
}

bool WindowImpl::Create()
{
    // iOS windows are always created with size same as screen size
    if (bridge->CreateWindow())
    {
        PlatformCore* core = engineBackend->GetPlatformCore();
        appBecomeOrResignActiveToken = core->didBecomeResignActive.Connect(bridge.get(), &WindowNativeBridge::ApplicationDidBecomeOrResignActive);
        appDidEnterForegroundOrBackgroundToken = core->didEnterForegroundBackground.Connect(bridge.get(), &WindowNativeBridge::ApplicationDidEnterForegroundOrBackground);
        return true;
    }
    return false;
}

void WindowImpl::Resize(float32 /*width*/, float32 /*height*/)
{
    // iOS windows are always stretched to screen size
}

void WindowImpl::Activate()
{
    // not supported on ios
}

void WindowImpl::Close(bool appIsTerminating)
{
    // iOS windows cannot be closed
    // TODO: later add ability to close secondary windows

    if (appIsTerminating)
    {
        // If application is terminating then send event as if window has been destroyed.
        // Engine ensures that Close with appIsTerminating with true value is always called on termination.
        mainDispatcher->SendEvent(MainDispatcherEvent::CreateWindowDestroyedEvent(window), MainDispatcher::eSendPolicy::IMMEDIATE_EXECUTION);
    }
}

void WindowImpl::SetTitle(const String& title)
{
    // iOS window does not have title
}

void WindowImpl::SetMinimumSize(Size2f /*size*/)
{
    // Minimum size does not apply to iOS window
}

void WindowImpl::SetFullscreen(eFullscreen /*newMode*/)
{
    // Fullscreen mode cannot be changed on iOS
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
        bridge->TriggerPlatformEvents();
    }
}

void WindowImpl::ProcessPlatformEvents()
{
    uiDispatcher.ProcessEvents();
}

void WindowImpl::SetSurfaceScaleAsync(const float32 scale)
{
    DVASSERT(scale > 0.0f && scale <= 1.0f);

    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetSurfaceScaleEvent(scale));
}

void WindowImpl::SetCursorCapture(eCursorCapture mode)
{
    // not supported
}

void WindowImpl::SetCursorVisibility(bool visible)
{
    // not supported
}

void WindowImpl::UIEventHandler(const UIDispatcherEvent& e)
{
    switch (e.type)
    {
    // iOS windows cannot be closed and are always stretched to screen size
    // case UIDispatcherEvent::CLOSE_WINDOW:
    // case UIDispatcherEvent::RESIZE_WINDOW:
    case UIDispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case UIDispatcherEvent::SET_SURFACE_SCALE:
        bridge->SetSurfaceScale(e.setSurfaceScaleEvent.scale);
        break;
    default:
        break;
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
