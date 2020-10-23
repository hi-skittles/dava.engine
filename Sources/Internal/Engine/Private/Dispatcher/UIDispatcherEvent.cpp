#include "Engine/Private/Dispatcher/UIDispatcherEvent.h"

namespace DAVA
{
namespace Private
{
UIDispatcherEvent UIDispatcherEvent::CreateResizeEvent(float32 width, float32 height)
{
    UIDispatcherEvent e(RESIZE_WINDOW);
    e.resizeEvent.width = width;
    e.resizeEvent.height = height;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateActivateEvent()
{
    return UIDispatcherEvent(ACTIVATE_WINDOW);
}

UIDispatcherEvent UIDispatcherEvent::CreateMinimumSizeEvent(float32 width, float32 height)
{
    UIDispatcherEvent e(SET_MINIMUM_SIZE);
    e.resizeEvent.width = width;
    e.resizeEvent.height = height;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateCloseEvent()
{
    UIDispatcherEvent e(CLOSE_WINDOW);
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateSetTitleEvent(const String& title)
{
    char8* buf = new char8[title.length() + 1];
    std::copy(begin(title), end(title), buf);
    buf[title.length()] = '\0';

    UIDispatcherEvent e(SET_TITLE);
    e.setTitleEvent.title = buf;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateSetFullscreenEvent(eFullscreen mode)
{
    UIDispatcherEvent e(SET_FULLSCREEN);
    e.setFullscreenEvent.mode = mode;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateSetCursorCaptureEvent(eCursorCapture mode)
{
    UIDispatcherEvent e(SET_CURSOR_CAPTURE);
    e.setCursorCaptureEvent.mode = mode;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateSetCursorVisibilityEvent(bool visible)
{
    UIDispatcherEvent e(SET_CURSOR_VISIBILITY);
    e.setCursorVisibilityEvent.visible = visible;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateFunctorEvent(const Function<void()>& functor)
{
    UIDispatcherEvent e(FUNCTOR);
    e.functor = functor;
    return e;
}

UIDispatcherEvent UIDispatcherEvent::CreateSetSurfaceScaleEvent(const float32 scale)
{
    UIDispatcherEvent e(SET_SURFACE_SCALE);
    e.setSurfaceScaleEvent.scale = scale;
    return e;
}

} // namespace Private
} // namespace DAVA
