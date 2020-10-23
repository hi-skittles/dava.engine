#pragma once

#if defined(__DAVAENGINE_QT__)

#include "Base/BaseTypes.h"
#include "Engine/Private/Qt/RenderWidgetBackend.h"

#include <QQuickWidget>
#include <QEvent>

namespace DAVA
{
class RenderWidgetOGL : public RenderWidgetBackendImpl<QQuickWidget>
{
    using TBase = RenderWidgetBackendImpl<QQuickWidget>;

public:
    RenderWidgetOGL(IWindowDelegate* widgetDelegate_, uint32 width, uint32 height, QWidget* parent);

    bool IsInitialized() const override;
    void Update() override;
    void InitCustomRenderParams(rhi::InitParam& params) override;
    void AcquireContext() override;
    void ReleaseContext() override;

protected:
    void showEvent(QShowEvent* e) override;
    bool event(QEvent* e) override;
    bool eventFilter(QObject* object, QEvent* e) override;
    bool IsInFullScreen() const override;

    virtual QWindow* GetQWindow() override;

private:
    void OnCreated() override;
    void OnFrame() override;
    void OnBeforeSyncronizing();
    void OnActiveFocusItemChanged();
    void OnSceneGraphInvalidated();

    void TryActivate();
    void ActivateRendering();

private:
    bool keyEventRecursiveGuard = false;
    bool isInPaint = false;
    bool isSynchronized = false;
    bool isActivated = false;

    class OGLContextBinder;
    friend void AcquireContextImpl();
    friend void ReleaseContextImpl();
    std::unique_ptr<OGLContextBinder> contextBinder;
};

} // namespace DAVA

#endif // __DAVAENGINE_QT__
