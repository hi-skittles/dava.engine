#pragma once

#if defined(__DAVAENGINE_QT__)

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "Engine/EngineTypes.h"
#include "Engine/Private/Qt/IWindowDelegate.h"
#include "Engine/Qt/IClientDelegate.h"
#include "Functional/Signal.h"
#include "Render/RHI/rhi_Public.h"

namespace DAVA
{
class RenderWidgetBackend
{
public:
    RenderWidgetBackend(IWindowDelegate* windowDelegate);

    virtual ~RenderWidgetBackend() = default;
    virtual bool IsInitialized() const = 0;
    virtual void Update() = 0;
    virtual void InitCustomRenderParams(rhi::InitParam& params) = 0;
    virtual void AcquireContext() = 0;
    virtual void ReleaseContext() = 0;

    void SetClientDelegate(IClientDelegate* clientDelegate);
    void SetFrameBlocked(bool isBlocked);

    Signal<uint32, uint32> resized;

protected:
    virtual void OnCreated() = 0;
    virtual void OnFrame() = 0;
    virtual void OnDestroyed() = 0;
    virtual bool IsInFullScreen() const = 0;

    virtual QWindow* GetQWindow() = 0;

protected:
    IWindowDelegate* windowDelegate = nullptr;
    IClientDelegate* clientDelegate = nullptr;

    bool isClosing = false;
    bool isFrameBlocked = false;
};

template <typename TBase>
class RenderWidgetBackendImpl : public TBase, public RenderWidgetBackend
{
public:
    RenderWidgetBackendImpl(IWindowDelegate* windowDelegate, uint32 width, uint32 height, QWidget* parent);

protected:
    struct QtScreenParams
    {
        int screenScale = 0;
        int logicalDPI = 0;
    };

    void OnCreated() override;
    void OnFrame() override;
    void OnDestroyed() override;

    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;

protected:
    QtScreenParams screenParams;
};

} // namespace DAVA
#endif // __DAVAENGINE_QT__
