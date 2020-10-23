#include "Engine/Qt/RenderWidget.h"

#if defined(__DAVAENGINE_QT__)
#include "Engine/Private/Qt/IWindowDelegate.h"
#include "Engine/Private/Qt/RenderWidgetOGL.h"
#include "Engine/Private/Qt/RenderWidgetDX.h"
#include "Engine/Private/Qt/RenderWidgetBackend.h"
#include "Engine/Engine.h"

#include "Debug/DVAssert.h"

#include "Input/InputSystem.h"

#include "FileSystem/KeyedArchive.h"
#include "Logger/Logger.h"
#include "Base/BaseTypes.h"
#include "Base/Exception.h"

#include <QVariant>
#include <QVBoxLayout>

namespace DAVA
{
/////////////////////////////////////////////////////////////////////////////////
//                      RenderWidget                                           //
/////////////////////////////////////////////////////////////////////////////////

RenderWidget::RenderWidget(IWindowDelegate* widgetDelegate, uint32 width, uint32 height)
{
    const DAVA::KeyedArchive* options = Engine::Instance()->GetOptions();
    int32 renderer = options->GetInt32("renderer", rhi::RHI_GLES2);

    if (renderer == rhi::RHI_GLES2)
    {
        RenderWidgetOGL* oglWidget = new RenderWidgetOGL(widgetDelegate, width, height, this);
        renderWidgetBackend = oglWidget;
        renderWidgetImpl = oglWidget;
    }
    else
    {
#if defined(__DAVAENGINE_WIN32__)
        RenderWidgetDX* dxWindow = new RenderWidgetDX(widgetDelegate, width, height, this);
        renderWidgetBackend = dxWindow;
        renderWidgetImpl = dxWindow /*QWidget::createWindowContainer(dxWindow, this)*/;
#else
        DVASSERT(false, "On not Win32 platforms we can use only OpenGL backend");
#endif
    }

    renderWidgetImpl->setObjectName(BackendWidgetName);
    setFocusProxy(renderWidgetImpl);
    QVBoxLayout* boxLayout = new QVBoxLayout(this);
    boxLayout->setSpacing(0);
    boxLayout->setMargin(0);
    boxLayout->addWidget(renderWidgetImpl);
    setLayout(boxLayout);

    renderWidgetBackend->resized.Connect(&resized, &Signal<uint32, uint32>::Emit);
}

RenderWidget::~RenderWidget() = default;

void RenderWidget::SetClientDelegate(IClientDelegate* delegate)
{
    renderWidgetBackend->SetClientDelegate(delegate);
}

void RenderWidget::SetFrameBlocked(bool isBlocked)
{
    renderWidgetBackend->SetFrameBlocked(isBlocked);
}

void RenderWidget::actionEvent(QActionEvent* event)
{
    if (event->type() == QEvent::ActionAdded)
    {
        renderWidgetImpl->insertAction(event->before(), event->action());
    }
    else if (event->type() == QEvent::ActionRemoved)
    {
        renderWidgetImpl->removeAction(event->action());
    }
    else
    {
        QWidget::actionEvent(event);
    }
}

void RenderWidget::AcquireContext()
{
    renderWidgetBackend->AcquireContext();
}

void RenderWidget::ReleaseContext()
{
    renderWidgetBackend->ReleaseContext();
}

bool RenderWidget::IsInitialized() const
{
    return renderWidgetBackend->IsInitialized();
}

void RenderWidget::Update()
{
    renderWidgetBackend->Update();
}

void RenderWidget::InitCustomRenderParams(rhi::InitParam& params)
{
    renderWidgetBackend->InitCustomRenderParams(params);
}

QString RenderWidget::BackendWidgetName = QStringLiteral("dava_render_widget_backend");

} // namespace DAVA

#endif // __DAVAENGINE_QT__
