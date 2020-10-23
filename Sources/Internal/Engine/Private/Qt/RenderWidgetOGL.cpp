#include "Engine/Private/Qt/RenderWidgetOGL.h"

#if defined(__DAVAENGINE_QT__)
#include "Engine/Engine.h"
#include "Engine/Qt/IClientDelegate.h"
#include "Engine/Private/Qt/IWindowDelegate.h"
#include "Input/InputSystem.h"
#include "Input/Keyboard.h"
#include "DeviceManager/DeviceManager.h"
#include "Logger/Logger.h"
#include "Debug/DVAssert.h"

#include <QQuickItem>
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QPointer>

namespace DAVA
{
class RenderWidgetOGL::OGLContextBinder final
{
public:
    OGLContextBinder(QSurface* surface, QOpenGLContext* context)
        : davaContext(surface, context)
    {
        DVASSERT(binder == nullptr);
        binder = this;
    }

    ~OGLContextBinder()
    {
        DVASSERT(binder != nullptr);
        binder = nullptr;
    }

    void AcquireContext()
    {
        QSurface* prevSurface = nullptr;
        QOpenGLContext* prevContext = QOpenGLContext::currentContext();
        if (prevContext != nullptr)
        {
            prevSurface = prevContext->surface();
        }

        contextStack.emplace(prevSurface, prevContext);

        if (prevContext != davaContext.context)
        {
            davaContext.context->makeCurrent(davaContext.surface);
        }
    }

    void ReleaseContext()
    {
        DVASSERT(!contextStack.empty());
        QOpenGLContext* currentContext = QOpenGLContext::currentContext();

        ContextNode topNode = contextStack.top();
        contextStack.pop();

        if (topNode.context == currentContext)
        {
            return;
        }
        else if (currentContext != nullptr)
        {
            currentContext->doneCurrent();
        }

        if (topNode.context != nullptr && topNode.surface != nullptr)
        {
            topNode.context->makeCurrent(topNode.surface);
        }
    }

    void ReplaceContextIfNeed(QSurface* surface, QOpenGLContext* context)
    {
        if (davaContext.context != context)
        {
            DVASSERT(contextStack.size() == 1);
            davaContext.context = context;
            davaContext.surface = surface;
            contextStack.pop();
            contextStack.push(davaContext);
        }
    }

    static OGLContextBinder* binder;

private:
    struct ContextNode
    {
        ContextNode(QSurface* surface_ = nullptr, QOpenGLContext* context_ = nullptr)
            : surface(surface_)
            , context(context_)
        {
        }

        QSurface* surface = nullptr;
        QPointer<QOpenGLContext> context;
    };

    ContextNode davaContext;
    DAVA::Stack<ContextNode> contextStack;
};

RenderWidgetOGL::OGLContextBinder* RenderWidgetOGL::OGLContextBinder::binder = nullptr;

void AcquireContextImpl()
{
    DVASSERT(RenderWidgetOGL::OGLContextBinder::binder);
    RenderWidgetOGL::OGLContextBinder::binder->AcquireContext();
}

void ReleaseContextImpl()
{
    DVASSERT(RenderWidgetOGL::OGLContextBinder::binder);
    RenderWidgetOGL::OGLContextBinder::binder->ReleaseContext();
}

namespace RenderWidgetOGLDetail
{
const char* initializedPropertyName = "initialized";
}

RenderWidgetOGL::RenderWidgetOGL(IWindowDelegate* widgetDelegate, uint32 width, uint32 height, QWidget* parent)
    : TBase(widgetDelegate, width, height, parent)
{
    setResizeMode(QQuickWidget::SizeViewToRootObject);

    QQuickWindow* window = quickWindow();
    window->installEventFilter(this);
    window->setClearBeforeRendering(true);
    window->setColor(QColor(76, 76, 76, 255));
    connect(window, &QQuickWindow::sceneGraphInvalidated, this, &RenderWidgetOGL::OnSceneGraphInvalidated, Qt::DirectConnection);
    connect(window, &QQuickWindow::activeFocusItemChanged, this, &RenderWidgetOGL::OnActiveFocusItemChanged, Qt::DirectConnection);
    connect(window, &QQuickWindow::beforeSynchronizing, this, &RenderWidgetOGL::OnBeforeSyncronizing, Qt::DirectConnection);
}

void RenderWidgetOGL::OnCreated()
{
    setProperty(RenderWidgetOGLDetail::initializedPropertyName, true);

    // QuickWidnow in QQuickWidget is not "real" window, it doesn't have "platform window" handle,
    // so Qt can't make context current for that surface. Real surface is QOffscreenWindow that live inside
    // QQuickWidgetPrivate and we can get it only through context.
    // In applications with QMainWindow (where RenderWidget is a part of MainWindow) it's good solution,
    // But for TestBed for example this solution is not full,
    // because QQuickWidget "recreate" offscreenWindow every time on pair of show-hide events
    // I don't know what we can do with this.
    // Now i can only suggest: do not create Qt-based game! Never! Do you hear me??? Never! Never! Never! Never! Never! NEVER!!!
    QOpenGLContext* context = quickWindow()->openglContext();
    contextBinder.reset(new OGLContextBinder(context->surface(), context));

    TBase::OnCreated();
}

void RenderWidgetOGL::OnFrame()
{
    DVASSERT(isInPaint == false);
    isInPaint = true;
    SCOPE_EXIT
    {
        isInPaint = false;
    };

    QQuickWindow* wnd = quickWindow();

    QOpenGLContext* ctx = wnd->openglContext();
    if (contextBinder != nullptr)
    {
        contextBinder->ReplaceContextIfNeed(ctx->surface(), ctx);
    }

    QVariant nativeHandle = ctx->nativeHandle();
    if (!nativeHandle.isValid())
    {
        DAVA::Logger::Error("GL context is not valid!");
        DAVA_THROW(DAVA::Exception, "GL context is not valid!");
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    TBase::OnFrame();
    wnd->resetOpenGLState();
}

void RenderWidgetOGL::OnBeforeSyncronizing()
{
    disconnect(quickWindow(), &QQuickWindow::beforeSynchronizing, this, &RenderWidgetOGL::OnBeforeSyncronizing);
    isSynchronized = true;
    TryActivate();
}

void RenderWidgetOGL::TryActivate()
{
    if (IsInitialized() == false && isActivated && isSynchronized)
    {
        ActivateRendering();
        OnCreated();
    }
}

void RenderWidgetOGL::ActivateRendering()
{
    QQuickWindow* w = quickWindow();
    connect(w, &QQuickWindow::beforeRendering, this, &RenderWidgetOGL::OnFrame, Qt::DirectConnection);
    w->setClearBeforeRendering(false);
}

bool RenderWidgetOGL::IsInitialized() const
{
    return property(RenderWidgetOGLDetail::initializedPropertyName).isValid();
}

void RenderWidgetOGL::Update()
{
    quickWindow()->update();
}

void RenderWidgetOGL::InitCustomRenderParams(rhi::InitParam& params)
{
    params.threadedRenderEnabled = false;
    params.threadedRenderFrameCount = 1;
    params.acquireContextFunc = &AcquireContextImpl;
    params.releaseContextFunc = &ReleaseContextImpl;
    params.defaultFrameBuffer = reinterpret_cast<void*>(static_cast<intptr_t>(quickWindow()->renderTarget()->handle()));
}

void RenderWidgetOGL::AcquireContext()
{
    AcquireContextImpl();
}

void RenderWidgetOGL::ReleaseContext()
{
    ReleaseContextImpl();
}

void RenderWidgetOGL::OnActiveFocusItemChanged()
{
    QQuickItem* item = quickWindow()->activeFocusItem();
    bool focusRequested = item != nullptr;
    if (focusRequested)
    {
        item->installEventFilter(this);
    }

    Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
    if (kb != nullptr)
    {
        kb->ResetState(GetPrimaryWindow()); //we need only reset keyboard status on focus changing
    }
}

void RenderWidgetOGL::OnSceneGraphInvalidated()
{
    TBase::OnDestroyed();
}

bool RenderWidgetOGL::event(QEvent* e)
{
    QEvent::Type eventType = e->type();
    if (eventType == QEvent::WindowActivate || (eventType == QEvent::Polish && isActiveWindow()))
    {
        isActivated = true;
        TryActivate();
    }

    return TBase::event(e);
}

void RenderWidgetOGL::showEvent(QShowEvent* e)
{
    TBase::showEvent(e);
}

bool RenderWidgetOGL::eventFilter(QObject* object, QEvent* e)
{
    QEvent::Type t = e->type();
    if ((t == QEvent::KeyPress || t == QEvent::KeyRelease) && keyEventRecursiveGuard == false)
    {
        QQuickItem* focusObject = quickWindow()->activeFocusItem();
        if (object == quickWindow() || object == focusObject)
        {
            keyEventRecursiveGuard = true;
            SCOPE_EXIT
            {
                keyEventRecursiveGuard = false;
            };
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (t == QEvent::KeyPress)
            {
                keyPressEvent(keyEvent);
            }
            else
            {
                keyReleaseEvent(keyEvent);
            }
            return true;
        }
    }

    return false;
}

bool RenderWidgetOGL::IsInFullScreen() const
{
    QQuickWindow* wnd = quickWindow();
    return wnd->visibility() == QWindow::FullScreen;
}

QWindow* RenderWidgetOGL::GetQWindow()
{
    return quickWindow();
}

} // namespace DAVA

#endif // __DAVAENGINE_QT__
