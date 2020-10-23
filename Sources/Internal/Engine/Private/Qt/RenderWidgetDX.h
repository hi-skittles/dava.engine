#pragma once

#if defined(__DAVAENGINE_QT__)

#include "Base/BaseTypes.h"
#include "Engine/Private/Qt/RenderWidgetBackend.h"

#include <QWidget>

namespace DAVA
{
class RenderWidgetDX : public RenderWidgetBackendImpl<QWidget>
{
    using TBase = RenderWidgetBackendImpl<QWidget>;

public:
    RenderWidgetDX(IWindowDelegate* windowDelegate, uint32 width, uint32 height, QWidget* parent);

    bool IsInitialized() const override;
    void Update() override;
    void InitCustomRenderParams(rhi::InitParam& params) override;
    void AcquireContext() override;
    void ReleaseContext() override;

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

    bool IsInFullScreen() const override;
    QWindow* GetQWindow() override;
    QPaintEngine* paintEngine() const override;
    void OnCreated() override;
    void OnFrame() override;
    void OnDestroyed() override;

    class RenderSurface;
    RenderSurface* surface = nullptr;
};
} // namespace DAVA
#endif // __DAVAENGINE_QT__
