#pragma once

#include "Base/BaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Functional/Function.h"

namespace DAVA
{
namespace Private
{
struct UIDispatcherEvent final
{
    enum eType : int32
    {
        DUMMY = 0,
        RESIZE_WINDOW,
        ACTIVATE_WINDOW,
        CREATE_WINDOW,
        CLOSE_WINDOW,
        SET_TITLE,
        SET_MINIMUM_SIZE,
        SET_FULLSCREEN,
        FUNCTOR,
        SET_CURSOR_CAPTURE,
        SET_CURSOR_VISIBILITY,
        SET_SURFACE_SCALE
    };

    struct ResizeEvent
    {
        float32 width;
        float32 height;
    };

    struct SetTitleEvent
    {
        const char8* title;
    };

    struct SetFullscreenEvent
    {
        eFullscreen mode;
    };

    struct SetCursorCaptureEvent
    {
        eCursorCapture mode;
    };

    struct SetCursorVisibilityEvent
    {
        bool visible;
    };

    struct SetSurfaceScaleEvent
    {
        float32 scale;
    };

    UIDispatcherEvent() = default;
    UIDispatcherEvent(eType type)
        : type(type)
    {
    }

    eType type = DUMMY;
    Function<void()> functor;
    union
    {
        ResizeEvent resizeEvent;
        SetTitleEvent setTitleEvent;
        SetFullscreenEvent setFullscreenEvent;
        SetCursorCaptureEvent setCursorCaptureEvent;
        SetCursorVisibilityEvent setCursorVisibilityEvent;
        SetSurfaceScaleEvent setSurfaceScaleEvent;
    };

    static UIDispatcherEvent CreateResizeEvent(float32 width, float32 height);
    static UIDispatcherEvent CreateActivateEvent();
    static UIDispatcherEvent CreateMinimumSizeEvent(float32 width, float32 height);
    static UIDispatcherEvent CreateCloseEvent();
    static UIDispatcherEvent CreateSetTitleEvent(const String& title);
    static UIDispatcherEvent CreateSetFullscreenEvent(eFullscreen mode);
    static UIDispatcherEvent CreateSetCursorCaptureEvent(eCursorCapture mode);
    static UIDispatcherEvent CreateSetCursorVisibilityEvent(bool visible);
    static UIDispatcherEvent CreateFunctorEvent(const Function<void()>& functor);
    static UIDispatcherEvent CreateSetSurfaceScaleEvent(const float32 scale);
};

} // namespace Private
} // namespace DAVA
