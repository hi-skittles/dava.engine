#include "Engine/Window.h"

#include "Engine/EngineContext.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/WindowImpl.h"

#include "Utils/StringFormat.h"
#include "Animation/AnimationManager.h"
#include "Autotesting/AutotestingSystem.h"
#include "Input/InputSystem.h"
#include "Input/InputEvent.h"
#include "Input/Keyboard.h"
#include "Logger/Logger.h"
#include "Time/SystemTimer.h"
#include "Render/2D/TextBlock.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIControlSystem.h"
#include "DeviceManager/DeviceManager.h"

namespace DAVA
{
Window::Window(Private::EngineBackend* engineBackend, bool primary)
    : engineBackend(engineBackend)
    , mainDispatcher(engineBackend->GetDispatcher())
    , windowImpl(new Private::WindowImpl(engineBackend, this))
    , isPrimary(primary)
{
    // TODO: Add platfom's caps check
    //if (windowImpl->IsPlatformSupported(SET_CURSOR_VISIBILITY))
    {
        cursorVisible = true;
    }
}

Window::~Window() = default;

void Window::SetSizeAsync(Size2f sz)
{
    // Window cannot be resized in embedded mode as window lifetime
    // is controlled by highlevel framework
    if (!engineBackend->IsEmbeddedGUIMode())
    {
        windowImpl->Resize(sz.dx, sz.dy);
    }
}

void Window::ActivateAsync()
{
    if (!engineBackend->IsEmbeddedGUIMode())
    {
        windowImpl->Activate();
    }
}

void Window::SetMinimumSize(Size2f size)
{
    DVASSERT(size.dx >= 0.f && size.dy >= 0.f);

    if (!engineBackend->IsEmbeddedGUIMode())
    {
        size.dx = std::max(size.dx, static_cast<float32>(smallestWidth));
        size.dy = std::max(size.dy, static_cast<float32>(smallestHeight));

        windowImpl->SetMinimumSize(size);
    }
}

void Window::SetVirtualSize(float32 w, float32 h)
{
    uiControlSystem->vcs->SetVirtualScreenSize(static_cast<int32>(w), static_cast<int32>(h));
}

Size2f Window::GetVirtualSize() const
{
    Size2i sz = uiControlSystem->vcs->GetVirtualScreenSize();
    return Size2f(static_cast<float32>(sz.dx), static_cast<float32>(sz.dy));
}

void Window::CloseAsync()
{
    // Window cannot be close in embedded mode as window lifetime
    // is controlled by highlevel framework
    if (!engineBackend->IsEmbeddedGUIMode())
    {
        windowImpl->Close(false);
    }
}

void Window::SetTitleAsync(const String& title)
{
    // It does not make sense to set window title in embedded mode
    if (!engineBackend->IsEmbeddedGUIMode())
    {
        windowImpl->SetTitle(title);
    }
}

void Window::SetFullscreenAsync(eFullscreen newMode)
{
    // Window's fullscreen mode cannot be changed in embedded mode
    if (!engineBackend->IsEmbeddedGUIMode() && newMode != fullscreenMode)
    {
        windowImpl->SetFullscreen(newMode);
    }
}

Engine* Window::GetEngine() const
{
    return engineBackend->GetEngine();
}

void* Window::GetNativeHandle() const
{
    return windowImpl->GetHandle();
}

void Window::RunOnUIThreadAsync(const Function<void()>& task)
{
    windowImpl->RunAsyncOnUIThread(task);
}

void Window::RunOnUIThread(const Function<void()>& task)
{
    windowImpl->RunAndWaitOnUIThread(task);
}

void Window::InitCustomRenderParams(rhi::InitParam& params)
{
    windowImpl->InitCustomRenderParams(params);
}

void Window::SetCursorCapture(eCursorCapture mode)
{
    // For now FRAME is not supported, introduce somehow later
    if (mode == eCursorCapture::FRAME)
        return;

    /*if (windowImpl->IsPlatformSupported(SET_CURSOR_CAPTURE))*/ // TODO: Add platfom's caps check
    {
        if (cursorCapture != mode)
        {
            cursorCapture = mode;
            cursorCaptureChanged.Emit(this, cursorCapture);
            if (cursorCapture == eCursorCapture::PINNING)
            {
                waitInputActivation |= !hasFocus;
                if (!waitInputActivation)
                {
                    windowImpl->SetCursorCapture(cursorCapture);
                    windowImpl->SetCursorVisibility(false);
                }
            }
            else if (hasFocus)
            {
                windowImpl->SetCursorCapture(cursorCapture);
                windowImpl->SetCursorVisibility(cursorVisible);
            }
        }
    }
}

eCursorCapture Window::GetCursorCapture() const
{
    return cursorCapture;
}

void Window::SetCursorVisibility(bool visible)
{
    /*if (windowImpl->IsPlatformSupported(SET_CURSOR_VISIBILITY))*/ // TODO: Add platfom's caps check
    {
        if (cursorVisible != visible)
        {
            cursorVisible = visible;
            if (hasFocus && cursorCapture != eCursorCapture::PINNING)
            {
                windowImpl->SetCursorVisibility(cursorVisible);
            }
        }
    }
}

bool Window::GetCursorVisibility() const
{
    return cursorVisible && cursorCapture != eCursorCapture::PINNING;
}

void Window::SetInputHandlingMode(eInputHandlingModes mode)
{
    inputHandlingMode = mode;
}

void Window::Update(float32 frameDelta)
{
    update.Emit(this, frameDelta);

    const EngineContext* context = engineBackend->GetContext();

#if defined(__DAVAENGINE_AUTOTESTING__)
    float32 realFrameDelta = SystemTimer::GetRealFrameDelta();
    context->autotestingSystem->Update(realFrameDelta);
#endif

    context->animationManager->Update(frameDelta);

    uiControlSystem->Update();
}

void Window::Draw()
{
    const EngineContext* context = engineBackend->GetContext();
    context->renderSystem2D->BeginFrame();

    uiControlSystem->Draw();

#if defined(__DAVAENGINE_AUTOTESTING__)
    context->autotestingSystem->Draw();
#endif
    draw.Emit(this);

    context->renderSystem2D->EndFrame();
}

bool Window::EventHandler(const Private::MainDispatcherEvent& e)
{
    using Private::MainDispatcherEvent;
    if (e.window != this)
        return false;

    if (MainDispatcherEvent::IsInputEvent(e.type))
    {
        // Skip input events if window does not have focus or pinning switching logic tells to ignore input event
        if ((inputHandlingMode == eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED && !hasFocus) || HandleInputActivation(e))
        {
            return true;
        }
    }

    bool isHandled = true;
    switch (e.type)
    {
    case MainDispatcherEvent::TRACKPAD_GESTURE:
        HandleTrackpadGesture(e);
        break;
    case MainDispatcherEvent::WINDOW_SIZE_CHANGED:
        HandleSizeChanged(e);
        break;
    case MainDispatcherEvent::WINDOW_DPI_CHANGED:
        HandleDpiChanged(e);
        break;
    case MainDispatcherEvent::WINDOW_FOCUS_CHANGED:
        HandleFocusChanged(e);
        break;
    case MainDispatcherEvent::WINDOW_VISIBILITY_CHANGED:
        HandleVisibilityChanged(e);
        break;
    case MainDispatcherEvent::WINDOW_CREATED:
        HandleWindowCreated(e);
        break;
    case MainDispatcherEvent::WINDOW_DESTROYED:
        HandleWindowDestroyed(e);
        break;
    case MainDispatcherEvent::WINDOW_CAPTURE_LOST:
        HandleCursorCaptureLost(e);
        break;
    case MainDispatcherEvent::WINDOW_CANCEL_INPUT:
        HandleCancelInput(e);
        isHandled = false; // To send it further to other input-dependent subscribers
        break;
    case MainDispatcherEvent::WINDOW_VISIBLE_FRAME_CHANGED:
        HandleVisibleFrameChanged(e);
        break;
    case MainDispatcherEvent::WINDOW_SAFE_AREA_INSETS_CHANGED:
        HandleSafeAreaInsetsChanged(e);
        break;
    default:
        isHandled = false;
        break;
    }
    return isHandled;
}

void Window::FinishEventHandlingOnCurrentFrame()
{
    sizeEventsMerged = false;
    windowImpl->TriggerPlatformEvents();
}

void Window::HandleWindowCreated(const Private::MainDispatcherEvent& e)
{
    Logger::Info("Window::HandleWindowCreated: enter");

    isAlive = true;
    MergeSizeChangedEvents(e);
    sizeEventsMerged = true;

    engineBackend->InitRenderer(this);

    const EngineContext* context = engineBackend->GetContext();
    inputSystem = context->inputSystem;
    uiControlSystem = context->uiControlSystem;

    UpdateVirtualCoordinatesSystem();
    engineBackend->OnWindowCreated(this);

    sizeChanged.Emit(this, GetSize(), GetSurfaceSize());

    Logger::Info("Window::HandleWindowCreated: leave");
}

void Window::HandleWindowDestroyed(const Private::MainDispatcherEvent& e)
{
    Logger::Info("Window::HandleWindowDestroyed: enter");

    engineBackend->OnWindowDestroyed(this);

    inputSystem = nullptr;
    uiControlSystem = nullptr;

    engineBackend->DeinitRender(this);
    isAlive = false;

    Logger::Info("Window::HandleWindowDestroyed: leave");
}

void Window::HandleCursorCaptureLost(const Private::MainDispatcherEvent& e)
{
    // If the native window loses the cursor capture, restore it and visibility when input activated.
    waitInputActivation = true;
}

void Window::HandleSizeChanged(const Private::MainDispatcherEvent& e)
{
    if (!sizeEventsMerged)
    {
        Logger::FrameworkDebug("=========== WINDOW_SIZE_CHANGED");

        MergeSizeChangedEvents(e);
        sizeEventsMerged = true;

        engineBackend->ResetRenderer(this, !windowImpl->IsWindowReadyForRender());
        if (windowImpl->IsWindowReadyForRender())
        {
            UpdateVirtualCoordinatesSystem();
            sizeChanged.Emit(this, GetSize(), GetSurfaceSize());

            // TODO:
            // Resources must be separated from VirtualCoordinateSystem
            // Each resource consumer have to care for his resources by itself,
            // e.g. sprites reloading mechanism should be implemented in Sprite.cpp
            // by handling Window::sizeChanged signal and making sprites reload
            // inside that particular handler...
            //
            // Unfortunately we have only temporary solution:
            // call reloadig sprites/fonts from this point ((
            if (uiControlSystem->vcs->GetReloadResourceOnResize())
            {
// Disable sprite reloading on macos and windows
// Game uses separate thread for loading battle and its resources.
// Window resizing during battle loading may lead to crash as sprite
// reloading is not ready for multiple threads.
// TODO: do something with sprite reloading
//
// !!! At the moment this is a huge architectural problem,
// that we do not know how to solve.
// More detail can be found in DF-13044
//
#if !defined(__DAVAENGINE_MACOS__) && !defined(__DAVAENGINE_WINDOWS__)
                Sprite::ValidateForSize();
#endif
            }
        }
    }
}

void Window::HandleDpiChanged(const Private::MainDispatcherEvent& e)
{
    Logger::FrameworkDebug("=========== WINDOW_DPI_CHANGED: dpi=%f", e.dpiEvent.dpi);

    dpi = e.dpiEvent.dpi;
    dpiChanged.Emit(this, dpi);
}

void Window::MergeSizeChangedEvents(const Private::MainDispatcherEvent& e)
{
    // Look into dispatcher queue and compress size events into one event to allow:
    //  - single render init/reset call during one frame
    //  - emit signals about window creation or size changing immediately on event receiving
    using Private::MainDispatcherEvent;

    MainDispatcherEvent::WindowSizeEvent compressedSize(e.sizeEvent);
    mainDispatcher->ViewEventQueue([this, &compressedSize](const MainDispatcherEvent& e) {
        if (e.window == this && e.type == MainDispatcherEvent::WINDOW_SIZE_CHANGED)
        {
            compressedSize = e.sizeEvent;
        }
    });

    width = compressedSize.width;
    height = compressedSize.height;
    surfaceWidth = compressedSize.surfaceWidth;
    surfaceHeight = compressedSize.surfaceHeight;
    surfaceScale = compressedSize.surfaceScale;
    fullscreenMode = compressedSize.fullscreen;
    dpi = compressedSize.dpi;

    Logger::FrameworkDebug("=========== SizeChanged merged to: width=%.1f, height=%.1f, surfaceW=%.3f, surfaceH=%.3f, dpi=%f", width, height, surfaceWidth, surfaceHeight, dpi);
}

void Window::UpdateVirtualCoordinatesSystem()
{
    int32 w = static_cast<int32>(width);
    int32 h = static_cast<int32>(height);

    int32 sw = static_cast<int32>(surfaceWidth);
    int32 sh = static_cast<int32>(surfaceHeight);

    uiControlSystem->vcs->SetInputScreenAreaSize(w, h);
    uiControlSystem->vcs->SetPhysicalScreenSize(sw, sh);
}

bool Window::HandleInputActivation(const Private::MainDispatcherEvent& e)
{
    using Private::MainDispatcherEvent;
    if (waitInputActivation && cursorCapture == eCursorCapture::PINNING)
    {
        bool skipEvent = true;
        bool enablePinning = true;
        switch (e.type)
        {
        case MainDispatcherEvent::MOUSE_BUTTON_DOWN:
            skipEvent = true;
            enablePinning = true;
            break;
        case MainDispatcherEvent::MOUSE_BUTTON_UP:
            skipEvent = true;
            enablePinning = true;
            waitInputActivation = false;
            break;
        case MainDispatcherEvent::MOUSE_MOVE:
            skipEvent = true;
            enablePinning = false;
            break;
        default:
            skipEvent = false;
            enablePinning = true;
            waitInputActivation = false;
            break;
        }

        if (enablePinning)
        {
            windowImpl->SetCursorCapture(eCursorCapture::PINNING);
            windowImpl->SetCursorVisibility(false);
        }
        return skipEvent;
    }
    waitInputActivation = false;
    return false;
}

void Window::HandleCancelInput(const Private::MainDispatcherEvent& e)
{
    uiControlSystem->CancelAllInputs();
}

void Window::HandleVisibleFrameChanged(const Private::MainDispatcherEvent& e)
{
    Rect visibleRect(e.visibleFrameEvent.x, e.visibleFrameEvent.y, e.visibleFrameEvent.width, e.visibleFrameEvent.height);
    visibleFrameChanged.Emit(this, visibleRect);
}

void Window::HandleSafeAreaInsetsChanged(const Private::MainDispatcherEvent& e)
{
    if (uiControlSystem)
    {
        uiControlSystem->SetPhysicalSafeAreaInsets(e.safeAreaInsetsEvent.left,
                                                   e.safeAreaInsetsEvent.top,
                                                   e.safeAreaInsetsEvent.right,
                                                   e.safeAreaInsetsEvent.bottom,
                                                   e.safeAreaInsetsEvent.isLeftNotch,
                                                   e.safeAreaInsetsEvent.isRightNotch);
    }
}

void Window::HandleFocusChanged(const Private::MainDispatcherEvent& e)
{
    bool gainsFocus = e.stateEvent.state != 0;
    if (hasFocus != gainsFocus)
    {
        Logger::FrameworkDebug("=========== WINDOW_FOCUS_CHANGED: state=%s", e.stateEvent.state ? "got_focus" : "lost_focus");

        uiControlSystem->CancelAllInputs();
        hasFocus = gainsFocus;
        /*if (windowImpl->IsPlatformSupported(SET_CURSOR_CAPTURE))*/ // TODO: Add platfom's caps check
        {
            // When the native window loses focus, it restores the original cursor capture and visibility.
            // After the window gives the focus back, set the current visibility state, if not set pinning mode.
            // If the cursor capture mode is pinning, set the visibility state and capture mode when input activated.
            if (hasFocus && cursorCapture != eCursorCapture::PINNING)
            {
                windowImpl->SetCursorVisibility(cursorVisible);
                windowImpl->SetCursorCapture(cursorCapture);
            }
        }
        focusChanged.Emit(this, hasFocus);
    }
}

void Window::HandleVisibilityChanged(const Private::MainDispatcherEvent& e)
{
    bool becomesVisible = e.stateEvent.state != 0;
    if (isVisible != becomesVisible)
    {
        Logger::Info("Window::HandleVisibilityChanged: become %s", e.stateEvent.state ? "visible" : "hidden");

        isVisible = becomesVisible;
        visibilityChanged.Emit(this, isVisible);

        waitInputActivation = isVisible;
    }
}

void Window::HandleTrackpadGesture(const Private::MainDispatcherEvent& e)
{
    // TODO: move gestures to the new input system

    UIEvent uie;
    uie.window = e.window;
    uie.timestamp = e.timestamp / 1000.0;
    uie.modifiers = e.trackpadGestureEvent.modifierKeys;
    uie.device = eInputDevices::TOUCH_PAD;
    uie.phase = UIEvent::Phase::GESTURE;
    uie.physPoint = Vector2(e.trackpadGestureEvent.x, e.trackpadGestureEvent.y);
    uie.gesture.magnification = e.trackpadGestureEvent.magnification;
    uie.gesture.rotation = e.trackpadGestureEvent.rotation;
    uie.gesture.dx = e.trackpadGestureEvent.deltaX;
    uie.gesture.dy = e.trackpadGestureEvent.deltaY;

    inputSystem->HandleInputEvent(&uie);
}

void Window::SetSurfaceScaleAsync(float32 scale)
{
    if (scale <= 0.0f || scale > 1.0f)
    {
        DVASSERT(false, Format("Window::SetSurfaceScale: specified scale (%f) is out of range (0;1], ignoring", scale).c_str());
        return;
    }

    const float32 currentScale = GetSurfaceScale();
    if (FLOAT_EQUAL(currentScale, scale))
    {
        return;
    }

    windowImpl->SetSurfaceScaleAsync(scale);
}

} // namespace DAVA
