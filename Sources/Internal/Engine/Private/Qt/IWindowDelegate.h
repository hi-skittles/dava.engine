#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)

#include <QtEvents>

namespace DAVA
{
class IWindowDelegate
{
public:
    virtual void OnCreated() = 0;
    virtual bool OnUserCloseRequest() = 0;
    virtual void OnDestroyed() = 0;
    virtual void OnFrame() = 0;
    virtual void OnResized(uint32 width, uint32 height, bool isFullScreen) = 0;
    virtual void OnDpiChanged(float32 dpi) = 0;
    virtual void OnVisibilityChanged(bool isVisible) = 0;

    virtual void OnMousePressed(QMouseEvent* e) = 0;
    virtual void OnMouseReleased(QMouseEvent* e) = 0;
    virtual void OnMouseMove(QMouseEvent* e) = 0;
    virtual void OnDragMoved(QDragMoveEvent* e) = 0;
    virtual void OnMouseDBClick(QMouseEvent* e) = 0;
    virtual void OnWheel(QWheelEvent* e) = 0;
    virtual void OnNativeGesture(QNativeGestureEvent* e) = 0;

    virtual void OnKeyPressed(QKeyEvent* e) = 0;
    virtual void OnKeyReleased(QKeyEvent* e) = 0;
};
} // namespace DAVA

#endif // __DAVAENGINE_QT__
