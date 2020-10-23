#include "Engine/Private/Qt/RenderWidgetBackend.h"

#if defined(__DAVAENGINE_QT__)

#include "Debug/DVAssert.h"
#include "Engine/PlatformApiQt.h"
#include "Logger/Logger.h"

#include <QQuickWidget>
#include <QWidget>
#include <QDesktopWidget>
#include <QApplication>

namespace DAVA
{
namespace RenderWidgetBackendDetails
{
//there is a bug in Qt: https://bugreports.qt.io/browse/QTBUG-50465
void Kostil_ForceUpdateCurrentScreen(QWidget* renderWidget, QWindow* wnd, QApplication* application)
{
    QDesktopWidget* desktop = application->desktop();
    int screenNumber = desktop->screenNumber(renderWidget);
    DVASSERT(screenNumber >= 0 && screenNumber < qApp->screens().size());

    if (wnd != nullptr)
    {
        QWindow* parent = wnd;
        while (parent->parent() != nullptr)
        {
            parent = parent->parent();
        }
        parent->setScreen(application->screens().at(screenNumber));
    }
}
} //unnamed namespace

RenderWidgetBackend::RenderWidgetBackend(IWindowDelegate* windowDelegate_)
    : windowDelegate(windowDelegate_)
{
}

void RenderWidgetBackend::SetClientDelegate(IClientDelegate* clientDelegate_)
{
    DVASSERT(clientDelegate == nullptr || clientDelegate_ == nullptr);
    clientDelegate = clientDelegate_;
}

void RenderWidgetBackend::SetFrameBlocked(bool isBlocked)
{
    isFrameBlocked = isBlocked;
}

//////////////////////////////////////////////////////////////////////////////////////////////

template <typename TBase>
RenderWidgetBackendImpl<TBase>::RenderWidgetBackendImpl(IWindowDelegate* windowDelegate, uint32 width, uint32 height, QWidget* parent)
    : TBase(parent)
    , RenderWidgetBackend(windowDelegate)
{
    this->setAcceptDrops(true);
    this->setMouseTracking(true);

    this->setFocusPolicy(Qt::StrongFocus);
    this->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    this->setMinimumSize(QSize(width, height));
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::OnCreated()
{
    RenderWidgetBackendDetails::Kostil_ForceUpdateCurrentScreen(this, GetQWindow(), PlatformApi::Qt::GetApplication());
    screenParams.screenScale = this->devicePixelRatio();
    screenParams.logicalDPI = this->logicalDpiX();

    windowDelegate->OnCreated();

    QSize size = this->geometry().size();
    windowDelegate->OnResized(size.width(), size.height(), IsInFullScreen());
    resized.Emit(size.width(), size.height());
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::OnFrame()
{
    if (isFrameBlocked == true)
    {
        return;
    }

    if (screenParams.screenScale != this->devicePixelRatio())
    {
        screenParams.screenScale = this->devicePixelRatio();
        QSize size = this->geometry().size();
        windowDelegate->OnResized(size.width(), size.height(), IsInFullScreen());
    }

    if (screenParams.logicalDPI != this->logicalDpiX())
    {
        screenParams.logicalDPI = this->logicalDpiX();
        windowDelegate->OnDpiChanged(static_cast<float32>(screenParams.logicalDPI));
    }

    windowDelegate->OnFrame();
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::OnDestroyed()
{
    if (isClosing)
    {
        windowDelegate->OnDestroyed();
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::resizeEvent(QResizeEvent* e)
{
    QSize size = e->size();
    windowDelegate->OnResized(size.width(), size.height(), IsInFullScreen());
    resized.Emit(size.width(), size.height());
    TBase::resizeEvent(e);
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::showEvent(QShowEvent* e)
{
    TBase::showEvent(e);
    windowDelegate->OnVisibilityChanged(true);
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::hideEvent(QHideEvent* e)
{
    windowDelegate->OnVisibilityChanged(false);
    TBase::hideEvent(e);
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::closeEvent(QCloseEvent* e)
{
    if (windowDelegate->OnUserCloseRequest())
    {
        isClosing = true;
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::dragEnterEvent(QDragEnterEvent* e)
{
    if (clientDelegate != nullptr)
    {
        clientDelegate->OnDragEntered(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::dragMoveEvent(QDragMoveEvent* e)
{
    windowDelegate->OnDragMoved(e);
    if (clientDelegate != nullptr)
    {
        clientDelegate->OnDragMoved(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::dragLeaveEvent(QDragLeaveEvent* e)
{
    if (clientDelegate != nullptr)
    {
        clientDelegate->OnDragLeaved(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::dropEvent(QDropEvent* e)
{
    if (clientDelegate != nullptr)
    {
        clientDelegate->OnDrop(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::mousePressEvent(QMouseEvent* e)
{
    TBase::mousePressEvent(e);
    windowDelegate->OnMousePressed(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnMousePressed(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::mouseReleaseEvent(QMouseEvent* e)
{
    TBase::mouseReleaseEvent(e);
    windowDelegate->OnMouseReleased(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnMouseReleased(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::mouseDoubleClickEvent(QMouseEvent* e)
{
    TBase::mouseDoubleClickEvent(e);
    windowDelegate->OnMouseDBClick(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnMouseDBClick(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::mouseMoveEvent(QMouseEvent* e)
{
    TBase::mouseMoveEvent(e);
    windowDelegate->OnMouseMove(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnMouseMove(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::wheelEvent(QWheelEvent* e)
{
    TBase::wheelEvent(e);
    windowDelegate->OnWheel(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnWheel(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::keyPressEvent(QKeyEvent* e)
{
    TBase::keyPressEvent(e);
    windowDelegate->OnKeyPressed(e);

    if (clientDelegate != nullptr)
    {
        clientDelegate->OnKeyPressed(e);
    }
}

template <typename TBase>
void RenderWidgetBackendImpl<TBase>::keyReleaseEvent(QKeyEvent* e)
{
    TBase::keyReleaseEvent(e);
    windowDelegate->OnKeyReleased(e);
    if (clientDelegate != nullptr)
    {
        clientDelegate->OnKeyReleased(e);
    }
}

template <typename TBase>
bool RenderWidgetBackendImpl<TBase>::event(QEvent* e)
{
    if (e->type() == QEvent::NativeGesture)
    {
        QNativeGestureEvent* gestureEvent = static_cast<QNativeGestureEvent*>(e);
        windowDelegate->OnNativeGesture(gestureEvent);
        if (clientDelegate != nullptr)
        {
            clientDelegate->OnNativeGesture(gestureEvent);
        }
    }

    return TBase::event(e);
}

#if __clang__
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wweak-template-vtables\"")
#endif

template class RenderWidgetBackendImpl<QQuickWidget>;
template class RenderWidgetBackendImpl<QWidget>;

#if __clang__
_Pragma("clang diagnostic pop")
#endif

} // namespace DAVA
#endif // __DAVAENGINE_QT__
