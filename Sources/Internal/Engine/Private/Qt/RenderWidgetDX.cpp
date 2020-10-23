#include "Engine/Private/Qt/RenderWidgetDX.h"

#if defined(__DAVAENGINE_QT__)
#include "Engine/Private/Qt/RenderWidgetBackend.h"

#include <QWindow>
#include <QHBoxLayout>

namespace DAVA
{
class RenderWidgetDX::RenderSurface : public QWindow
{
public:
    using Callback = DAVA::Function<void()>;
    RenderSurface(const Callback& created, const Callback& frame, const Callback& destroyed)
        : createCallback(created)
        , frameCallback(frame)
        , destroyCallback(destroyed)
    {
        setSurfaceType(QSurface::RasterSurface);
    }

    ~RenderSurface()
    {
        destroyCallback();
    }

    void Update()
    {
        requestUpdate();
    }

    bool IsCreated() const
    {
        return created;
    }

protected:
    void exposeEvent(QExposeEvent* e)
    {
        Q_UNUSED(e);
        if (isExposed() && created == false)
        {
            created = true;
            createCallback();
        }
    }

    bool event(QEvent* e)
    {
        if (e->type() == QEvent::UpdateRequest)
        {
            DVASSERT(created == true);
            frameCallback();
            return true;
        }

        return QWindow::event(e);
    }

private:
    DAVA::Function<void()> createCallback;
    DAVA::Function<void()> frameCallback;
    DAVA::Function<void()> destroyCallback;

    bool created = false;
};

RenderWidgetDX::RenderWidgetDX(IWindowDelegate* windowDelegate, uint32 width, uint32 height, QWidget* parent)
    : TBase(windowDelegate, width, height, parent)
{
    surface = new RenderWidgetDX::RenderSurface(MakeFunction(this, &RenderWidgetDX::OnCreated),
                                                MakeFunction(this, &RenderWidgetDX::OnFrame),
                                                MakeFunction(this, &RenderWidgetDX::OnDestroyed));

    surface->installEventFilter(this);

    QWidget* container = QWidget::createWindowContainer(surface, this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(container);
    container->setFocusProxy(this);
    setFocusPolicy(Qt::FocusPolicy::WheelFocus);
}

bool RenderWidgetDX::IsInitialized() const
{
    return surface->IsCreated();
}

void RenderWidgetDX::Update()
{
    surface->Update();
}

void RenderWidgetDX::InitCustomRenderParams(rhi::InitParam& params)
{
    params.threadedRenderEnabled = false;
    params.threadedRenderFrameCount = 1;
    params.window = reinterpret_cast<void*>(surface->winId());
    params.useBackBufferExtraSize = true;
}

void RenderWidgetDX::AcquireContext()
{
}

void RenderWidgetDX::ReleaseContext()
{
}

bool RenderWidgetDX::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == surface)
    {
        switch (e->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            setFocus();
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::DragEnter:
        case QEvent::DragLeave:
        case QEvent::DragMove:
        case QEvent::Drop:
        case QEvent::NativeGesture:
            return event(e);
        default:
            break;
        }
    }

    return false;
}

bool RenderWidgetDX::IsInFullScreen() const
{
    return surface->visibility() == QWindow::FullScreen;
}

void RenderWidgetDX::OnCreated()
{
    TBase::OnCreated();
}

void RenderWidgetDX::OnFrame()
{
    TBase::OnFrame();
}

void RenderWidgetDX::OnDestroyed()
{
    TBase::OnDestroyed();
}

QWindow* RenderWidgetDX::GetQWindow()
{
    return surface;
}

QPaintEngine* RenderWidgetDX::paintEngine() const
{
    return nullptr;
}

} // namespace DAVA
#endif // __DAVAENGINE_QT__
