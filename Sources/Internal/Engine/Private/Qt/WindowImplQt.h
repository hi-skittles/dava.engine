#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)

#include "Engine/EngineTypes.h"
#include "Engine/Qt/RenderWidget.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/Qt/IWindowDelegate.h"
#include "Engine/Private/Dispatcher/UIDispatcher.h"

#include <QPointer>

namespace rhi
{
struct InitParam;
}

namespace DAVA
{
namespace Private
{
class WindowImpl final : public TrackedObject, private IWindowDelegate
{
public:
    WindowImpl(EngineBackend* engineBackend, Window* window);
    ~WindowImpl();

    WindowImpl(const WindowImpl&) = delete;
    WindowImpl& operator=(const WindowImpl&) = delete;

    void AcquireContext();
    void ReleaseContext();
    void OnApplicationFocusChanged(bool isInFocus);

    void Update();
    RenderWidget* GetRenderWidget();

    void Resize(float32 width, float32 height);
    void Activate();
    void Close(bool appIsTerminating);
    void SetTitle(const String& title);
    void SetMinimumSize(Size2f size);
    void SetFullscreen(eFullscreen newMode);

    void RunAsyncOnUIThread(const Function<void()>& task);
    void RunAndWaitOnUIThread(const Function<void()>& task);

    void* GetHandle() const;

    bool IsWindowReadyForRender() const;
    void InitCustomRenderParams(rhi::InitParam& params);

    void TriggerPlatformEvents();

    void SetSurfaceScaleAsync(const float32 scale);

    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);

private:
    void UIEventHandler(const UIDispatcherEvent& e);
    void DoResizeWindow(float32 width, float32 height);
    void DoCloseWindow();
    void DoSetTitle(const char8* title);
    void DoSetMinimumSize(float32 width, float32 height);

    // IWindowDelegate
    void OnCreated() override;
    bool OnUserCloseRequest() override;
    void OnDestroyed() override;
    void OnFrame() override;
    void OnResized(uint32 width, uint32 height, bool isFullScreen) override;
    void OnDpiChanged(float32 dpi_) override;
    void OnVisibilityChanged(bool isVisible) override;

    void OnMousePressed(QMouseEvent* e) override;
    void OnMouseReleased(QMouseEvent* e) override;
    void OnMouseMove(QMouseEvent* e) override;
    void OnDragMoved(QDragMoveEvent* e) override;
    void OnMouseDBClick(QMouseEvent* e) override;
    void OnWheel(QWheelEvent* e) override;
    void OnNativeGesture(QNativeGestureEvent* e) override;

    void OnKeyPressed(QKeyEvent* e) override;
    void OnKeyReleased(QKeyEvent* e) override;

    eModifierKeys GetModifierKeys() const;
    static eMouseButtons GetMouseButton(Qt::MouseButton button);
#if defined(Q_OS_OSX)
    uint32 ConvertQtKeyToSystemScanCode(int key);
#endif

private:
    EngineBackend* engineBackend = nullptr;
    Window* window = nullptr; // Window frontend reference
    MainDispatcher* mainDispatcher = nullptr; // Dispatcher that dispatches events to DAVA main thread
    UIDispatcher uiDispatcher; // Dispatcher that dispatches events to window UI thread

    // Use QPointer as renderWidget can be deleted outside WindowImpl in embedded mode
    QPointer<RenderWidget> renderWidget;

    bool closeRequestByApp = false;
    float32 dpi = 96.f;

    class QtEventListener;
    QtEventListener* qtEventListener = nullptr;
};

inline void* WindowImpl::GetHandle() const
{
    return nullptr;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_QT__
