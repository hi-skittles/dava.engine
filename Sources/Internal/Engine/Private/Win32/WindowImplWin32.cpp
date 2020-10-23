#include "Engine/Private/Win32/WindowImplWin32.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_WIN32__)

#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Win32/DllImportWin32.h"
#include "Engine/Private/Win32/PlatformCoreWin32.h"

#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"
#include "Time/SystemTimer.h"
#include "Render/Renderer.h"

namespace DAVA
{
namespace Private
{
HCURSOR WindowImpl::defaultCursor = nullptr;
bool WindowImpl::windowClassRegistered = false;
const wchar_t WindowImpl::windowClassName[] = L"DAVA_WND_CLASS";

WindowImpl::WindowImpl(EngineBackend* engineBackend, Window* window)
    : engineBackend(engineBackend)
    , window(window)
    , mainDispatcher(engineBackend->GetDispatcher())
    , uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler), MakeFunction(this, &WindowImpl::TriggerPlatformEvents))
    , minWidth(Window::smallestWidth)
    , minHeight(Window::smallestHeight)
    , lastCursorPosition({ 0, 0 })
{
    ::memset(&windowPlacement, 0, sizeof(windowPlacement));
    windowPlacement.length = sizeof(WINDOWPLACEMENT);

    lastShiftStates[0] = lastShiftStates[1] = false;
}

WindowImpl::~WindowImpl()
{
    DVASSERT(hwnd == nullptr);
}

bool WindowImpl::Create(float32 width, float32 height)
{
    if (!RegisterWindowClass())
    {
        Logger::Error("Failed to register win32 window class: %d", GetLastError());
        return false;
    }

    HWND handle = ::CreateWindowExW(windowExStyle,
                                    windowClassName,
                                    L"",
                                    windowedStyle,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    nullptr,
                                    nullptr,
                                    PlatformCore::Win32AppInstance(),
                                    this);
    if (handle != nullptr)
    {
        DoResizeWindow(width, height, CENTER_ON_DISPLAY);
        ::ShowWindow(handle, SW_SHOWNORMAL);
        ::UpdateWindow(handle);
        return true;
    }
    else
    {
        Logger::Error("Failed to create win32 window: %d", GetLastError());
    }
    return false;
}

void WindowImpl::Resize(float32 width, float32 height)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateResizeEvent(width, height));
}

void WindowImpl::Activate()
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateActivateEvent());
}

void WindowImpl::Close(bool /*appIsTerminating*/)
{
    closeRequestByApp = true;
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateCloseEvent());
}

void WindowImpl::SetTitle(const String& title)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetTitleEvent(title));
}

void WindowImpl::SetMinimumSize(Size2f size)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateMinimumSizeEvent(size.dx, size.dy));
}

void WindowImpl::SetFullscreen(eFullscreen newMode)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetFullscreenEvent(newMode));
}

void WindowImpl::RunAsyncOnUIThread(const Function<void()>& task)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void WindowImpl::RunAndWaitOnUIThread(const Function<void()>& task)
{
    uiDispatcher.SendEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void WindowImpl::SetIcon(const wchar_t* iconResourceName)
{
    DVASSERT(hwnd != nullptr);

    HICON hicon = reinterpret_cast<HICON>(::LoadImageW(PlatformCore::Win32AppInstance(), iconResourceName, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (hicon != nullptr)
    {
        ::SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hicon));
        ::SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hicon));
    }
}

void WindowImpl::SetCursor(HCURSOR hcursor)
{
    DVASSERT(hwnd != nullptr);

    hcurCursor = hcursor;
    if (hcurCursor == nullptr)
    {
        hcurCursor = defaultCursor;
    }
}

bool WindowImpl::IsWindowReadyForRender() const
{
    return GetHandle() != nullptr;
}

void WindowImpl::TriggerPlatformEvents()
{
    if (uiDispatcher.HasEvents())
    {
        ::PostMessage(hwnd, WM_TRIGGER_EVENTS, 0, 0);
    }
}

void WindowImpl::SetCursorCapture(eCursorCapture mode)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetCursorCaptureEvent(mode));
}

void WindowImpl::SetCursorVisibility(bool visible)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetCursorVisibilityEvent(visible));
}

LRESULT WindowImpl::OnSetCursor(LPARAM lparam)
{
    uint16 hittest = LOWORD(lparam);
    if (hittest == HTCLIENT)
    {
        forceCursorHide = false;
        ::SetCursor(mouseVisible ? hcurCursor : nullptr);
        return TRUE;
    }
    return FALSE;
}

void WindowImpl::SetCursorInCenter()
{
    RECT clientRect;
    ::GetClientRect(hwnd, &clientRect);
    POINT point;
    point.x = ((clientRect.left + clientRect.right) / 2);
    point.y = ((clientRect.bottom + clientRect.top) / 2);
    ::ClientToScreen(hwnd, &point);
    ::SetCursorPos(point.x, point.y);
}

void WindowImpl::ProcessPlatformEvents()
{
    // Prevent processing UI dispatcher events to exclude cases when WM_TRIGGER_EVENTS is delivered
    // when modal dialog is open as Dispatcher::ProcessEvents is not reentrant now
    if (!EngineBackend::showingModalMessageBox)
    {
        uiDispatcher.ProcessEvents();
    }
}

void WindowImpl::SetSurfaceScaleAsync(const float32 scale)
{
    DVASSERT(scale > 0.0f && scale <= 1.0f);

    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetSurfaceScaleEvent(scale));
}

void WindowImpl::DoSetSurfaceScale(const float32 scale)
{
    if (Renderer::GetAPI() == rhi::RHI_DX9)
    {
        surfaceScale = scale;
        HandleSizeChanged(lastWidth, lastHeight, false);
    }
}

void WindowImpl::DoResizeWindow(float32 width, float32 height, int resizeFlags)
{
    if ((resizeFlags & NO_TRANSLATE_TO_DIPS) == 0)
    {
        width *= dipSize;
        height *= dipSize;
    }

    int32 w = static_cast<int32>(std::ceil(width));
    int32 h = static_cast<int32>(std::ceil(height));
    if ((resizeFlags & RESIZE_WHOLE_WINDOW) == 0)
    {
        RECT rc = { 0, 0, w, h };
        ::AdjustWindowRectEx(&rc, windowedStyle, FALSE, windowExStyle);

        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
    }

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    HMONITOR hmonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    ::GetMonitorInfoW(hmonitor, &mi);

    int monitorWidth = mi.rcWork.right - mi.rcWork.left;
    int monitorHeight = mi.rcWork.bottom - mi.rcWork.top;
    w = std::min(w, monitorWidth);
    h = std::min(h, monitorHeight);

    int x = 0;
    int y = 0;
    UINT flags = SWP_NOMOVE | SWP_NOZORDER;

    if (resizeFlags & CENTER_ON_DISPLAY)
    {
        RECT rc;
        ::GetWindowRect(hwnd, &rc);
        x = mi.rcWork.left + (monitorWidth - w) / 2;
        y = mi.rcWork.top + (monitorHeight - h) / 2;
        flags &= ~SWP_NOMOVE;
    }

    ::SetWindowPos(hwnd, nullptr, x, y, w, h, flags);
}

void WindowImpl::DoActivateWindow()
{
    if (::IsIconic(hwnd))
    {
        ::OpenIcon(hwnd);
    }
    else
    {
        ::SetForegroundWindow(hwnd);
    }
}

void WindowImpl::DoCloseWindow()
{
    ::DestroyWindow(hwnd);
}

void WindowImpl::DoSetTitle(const char8* title)
{
    WideString wideTitle = UTF8Utils::EncodeToWideString(title);
    ::SetWindowTextW(hwnd, wideTitle.c_str());
}

void WindowImpl::DoSetMinimumSize(float32 width, float32 height)
{
    minWidth = static_cast<int>(width);
    minHeight = static_cast<int>(height);
}

void WindowImpl::DoSetFullscreen(eFullscreen newMode)
{
    // Changing of fullscreen mode leads to size changing, so set mode before it applying
    if (newMode == eFullscreen::On)
    {
        isFullscreen = true;
        SetFullscreenMode();
    }
    else
    {
        isFullscreen = false;
        SetWindowedMode();
    }
}

void WindowImpl::SetFullscreenMode()
{
    // Get window placement which is needed for back to windowed mode
    ::GetWindowPlacement(hwnd, &windowPlacement);

    // Add WS_VISIBLE to fullscreen style to keep it visible (if it already is)
    // If it's not yet visible, the style should not be modified
    // since ShowWindow(..., SW_SHOW) will occur later
    uint32 style = fullscreenStyle | (::IsWindowVisible(hwnd) == TRUE ? WS_VISIBLE : 0);
    ::SetWindowLong(hwnd, GWL_STYLE, style);

    MONITORINFO monitorInfo = { sizeof(monitorInfo) };
    HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    ::GetMonitorInfo(monitor, &monitorInfo);

    ::SetWindowPos(hwnd,
                   HWND_TOP,
                   monitorInfo.rcMonitor.left,
                   monitorInfo.rcMonitor.top,
                   monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                   monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                   SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
}

void WindowImpl::SetWindowedMode()
{
    ::SetWindowLong(hwnd, GWL_STYLE, windowedStyle);
    ::SetWindowPlacement(hwnd, &windowPlacement);

    UINT flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, flags);
}

void WindowImpl::DoSetCursorCapture(eCursorCapture mode)
{
    if (captureMode != mode)
    {
        forceCursorHide = false;
        captureMode = mode;
        switch (mode)
        {
        case eCursorCapture::FRAME:
            //not implemented
            captureMode = mode;
            break;
        case eCursorCapture::PINNING:
        {
            // Windows 7 does not send WM_SETCURSOR message (which controls cursor visibility) while any mouse button is pressed.
            // As a result cursor is visible but pinning is on. So if any mouse button is pressed set flag forceCursorHide which
            // will hide cursor in mouse move events until WM_SETCURSOR will come.
            // Windows 8 and later send WM_SETCURSOR in any case.

            // clang-format off
            forceCursorHide =
                (::GetKeyState(VK_LBUTTON) & 0x80) != 0 ||
                (::GetKeyState(VK_RBUTTON) & 0x80) != 0 ||
                (::GetKeyState(VK_MBUTTON) & 0x80) != 0 ||
                (::GetKeyState(VK_XBUTTON1) & 0x80) != 0 ||
                (::GetKeyState(VK_XBUTTON2) & 0x80) != 0;
            // clang-format on
            SwitchToPinning();
            break;
        }
        case eCursorCapture::OFF:
        {
            ::SetCursorPos(lastCursorPosition.x, lastCursorPosition.y);
            break;
        }
        }
        UpdateClipCursor();
    }
}

void WindowImpl::SwitchToPinning()
{
    mouseMoveSkipCount = SKIP_N_MOUSE_MOVE_EVENTS;

    POINT pt;
    ::GetCursorPos(&pt);
    lastCursorPosition.x = pt.x;
    lastCursorPosition.y = pt.y;
    SetCursorInCenter();
}

void WindowImpl::UpdateClipCursor()
{
    ::ClipCursor(nullptr);
    if (captureMode == eCursorCapture::PINNING)
    {
        RECT rect;
        ::GetClientRect(hwnd, &rect);
        ::ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&rect));
        ::ClientToScreen(hwnd, reinterpret_cast<LPPOINT>(&rect) + 1);
        ::ClipCursor(&rect);
    }
}

void WindowImpl::HandleWindowFocusChanging(bool hasFocus)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, hasFocus));
    if (!hasFocus)
    {
        if (captureMode != eCursorCapture::OFF)
        {
            DoSetCursorCapture(eCursorCapture::OFF);
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCaptureLostEvent(window));
        }
        DoSetCursorVisibility(true);
    }

    PlatformCore::EnableHighResolutionTimer(hasFocus);
}

void WindowImpl::DoSetCursorVisibility(bool visible)
{
    mouseVisible = visible;
}

void WindowImpl::HandleSizeChanged(int32 w, int32 h, bool dpiChanged)
{
    lastWidth = w;
    lastHeight = h;

    float32 width = std::ceil(static_cast<float32>(w) / dipSize);
    float32 height = std::ceil(static_cast<float32>(h) / dipSize);

    float32 surfaceWidth = static_cast<float32>(w) * surfaceScale;
    float32 surfaceHeight = static_cast<float32>(h) * surfaceScale;

    eFullscreen fullscreen = isFullscreen ? eFullscreen::On : eFullscreen::Off;

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSizeChangedEvent(window, width, height, surfaceWidth, surfaceHeight, surfaceScale, dpi, fullscreen));
    if (dpiChanged)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowDpiChangedEvent(window, dpi));
    }
}

void WindowImpl::UIEventHandler(const UIDispatcherEvent& e)
{
    switch (e.type)
    {
    case UIDispatcherEvent::RESIZE_WINDOW:
        DoResizeWindow(e.resizeEvent.width, e.resizeEvent.height, CENTER_ON_DISPLAY);
        break;
    case UIDispatcherEvent::ACTIVATE_WINDOW:
        DoActivateWindow();
        break;
    case UIDispatcherEvent::CLOSE_WINDOW:
        DoCloseWindow();
        break;
    case UIDispatcherEvent::SET_TITLE:
        DoSetTitle(e.setTitleEvent.title);
        delete[] e.setTitleEvent.title;
        break;
    case UIDispatcherEvent::SET_MINIMUM_SIZE:
        DoSetMinimumSize(e.resizeEvent.width, e.resizeEvent.height);
        break;
    case UIDispatcherEvent::SET_FULLSCREEN:
        DoSetFullscreen(e.setFullscreenEvent.mode);
        break;
    case UIDispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case UIDispatcherEvent::SET_CURSOR_CAPTURE:
        DoSetCursorCapture(e.setCursorCaptureEvent.mode);
        break;
    case UIDispatcherEvent::SET_CURSOR_VISIBILITY:
        DoSetCursorVisibility(e.setCursorVisibilityEvent.visible);
        break;
    case UIDispatcherEvent::SET_SURFACE_SCALE:
        DoSetSurfaceScale(e.setSurfaceScaleEvent.scale);
        break;
    default:
        break;
    }
}

LRESULT WindowImpl::OnSize(int32 resizingType, int32 width, int32 height)
{
    UpdateClipCursor();
    if (resizingType == SIZE_MINIMIZED)
    {
        isMinimized = true;
        if (hasFocus)
        {
            hasFocus = false;
            HandleWindowFocusChanging(hasFocus);
        }
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, false));
        return 0;
    }

    if (resizingType == SIZE_RESTORED || resizingType == SIZE_MAXIMIZED)
    {
        if (isMinimized)
        {
            isMinimized = false;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
            return 0;
        }
    }
    if (!isEnteredSizingModalLoop)
    {
        HandleSizeChanged(width, height, false);
    }
    return 0;
}

LRESULT WindowImpl::OnEnterSizeMove()
{
    isEnteredSizingModalLoop = true;
    return 0;
}

LRESULT WindowImpl::OnExitSizeMove()
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);

    int32 w = rc.right - rc.left;
    int32 h = rc.bottom - rc.top;
    if (w != lastWidth || h != lastHeight)
    {
        HandleSizeChanged(w, h, false);
    }

    isEnteredSizingModalLoop = false;
    return 0;
}

LRESULT WindowImpl::OnEnterMenuLoop()
{
    ::ClipCursor(nullptr);
    return 0;
}

LRESULT WindowImpl::OnExitMenuLoop()
{
    UpdateClipCursor();

    // System menu is usually shown after pressing Alt+Space, window receives Alt up down,
    // but do not receives Alt key up. So send event to force clearing all inputs.
    // TODO: refactor this event usage according to the new input system?
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCancelInputEvent(window));
    if (captureMode == eCursorCapture::PINNING)
    {
        // Place cursor in window center to prevent big mouse move delta after menu is closed
        SetCursorInCenter();
    }
    return 0;
}

LRESULT WindowImpl::OnGetMinMaxInfo(MINMAXINFO* minMaxInfo)
{
    minMaxInfo->ptMinTrackSize.x = minWidth;
    minMaxInfo->ptMinTrackSize.y = minHeight;
    return 0;
}

LRESULT WindowImpl::OnDpiChanged(RECT* suggestedRect)
{
    float32 curDpi = GetDpi();
    if (dpi != curDpi)
    {
        // Ignore suggested size in fullscreen mode
        if (!isFullscreen)
        {
            float32 w = static_cast<float32>(suggestedRect->right - suggestedRect->left);
            float32 h = static_cast<float32>(suggestedRect->bottom - suggestedRect->top);
            DoResizeWindow(w, h, RESIZE_WHOLE_WINDOW | NO_TRANSLATE_TO_DIPS);
        }

        dpi = curDpi;
        dipSize = dpi / defaultDpi;
        HandleSizeChanged(lastWidth, lastHeight, true);
    }
    return 0;
}

LRESULT WindowImpl::OnActivate(WPARAM wparam)
{
    bool newFocus = (LOWORD(wparam) != WA_INACTIVE);
    if (hasFocus != newFocus)
    {
        hasFocus = newFocus;
        if (hasFocus && isMinimized)
        {
            isMinimized = false;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
        }
        HandleWindowFocusChanging(hasFocus);
    }
    return 0;
}

LRESULT WindowImpl::OnMouseMoveRelativeEvent(int x, int y)
{
    if (mouseMoveSkipCount > 0)
    {
        mouseMoveSkipCount -= 1;
        return 0;
    }

    RECT clientRect;
    ::GetClientRect(hwnd, &clientRect);
    int clientCenterX((clientRect.left + clientRect.right) / 2);
    int clientCenterY((clientRect.bottom + clientRect.top) / 2);
    int deltaX = x - clientCenterX;
    int deltaY = y - clientCenterY;
    eModifierKeys modifierKeys = GetModifierKeys();
    if (deltaX != 0 || deltaY != 0)
    {
        SetCursorInCenter();
        float32 vdeltaX = static_cast<float32>(deltaX) / dipSize;
        float32 vdeltaY = static_cast<float32>(deltaY) / dipSize;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, vdeltaX, vdeltaY, modifierKeys, true));
    }
    else
    {
        // skip mouse moveEvent, which generate SetCursorPos
    }
    return 0;
}

LRESULT WindowImpl::OnMouseMoveEvent(int32 x, int32 y)
{
    if (forceCursorHide)
    {
        // Hide cursor here untill WM_SETCURSOR message (see comment in DoSetCursorCapture)
        ::SetCursor(nullptr);
    }

    if (captureMode == eCursorCapture::PINNING)
    {
        return OnMouseMoveRelativeEvent(x, y);
    }
    // Windows generates WM_MOUSEMOVE event for primary touch point so check and process
    // mouse move only from mouse device. Also skip spurious move events as described in:
    // https://blogs.msdn.microsoft.com/oldnewthing/20031001-00/?p=42343/
    eInputDevices source = GetInputEventSourceLegacy(::GetMessageExtraInfo());
    if (source == eInputDevices::MOUSE && (x != lastMouseMoveX || y != lastMouseMoveY))
    {
        eModifierKeys modifierKeys = GetModifierKeys();
        float32 vx = static_cast<float32>(x) / dipSize;
        float32 vy = static_cast<float32>(y) / dipSize;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, vx, vy, modifierKeys, false));

        lastMouseMoveX = x;
        lastMouseMoveY = y;
    }
    return 0;
}

LRESULT WindowImpl::OnMouseWheelEvent(int32 deltaX, int32 deltaY, int32 x, int32 y)
{
    eModifierKeys modifierKeys = GetModifierKeys();
    float32 vx = static_cast<float32>(x) / dipSize;
    float32 vy = static_cast<float32>(y) / dipSize;
    float32 vdeltaX = static_cast<float32>(deltaX);
    float32 vdeltaY = static_cast<float32>(deltaY);
    bool isRelative = (captureMode == eCursorCapture::PINNING);
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseWheelEvent(window, vx, vy, vdeltaX, vdeltaY, modifierKeys, isRelative));
    return 0;
}

LRESULT WindowImpl::OnMouseClickEvent(UINT message, uint16 xbutton, int32 x, int32 y)
{
    // Windows generates WM_xBUTTONDONW/WM_xBUTTONUP event for primary touch point so check and process
    // mouse clicks only from mouse device.
    eInputDevices source = GetInputEventSourceLegacy(::GetMessageExtraInfo());
    if (source == eInputDevices::MOUSE)
    {
        uint32 newMouseButtonsState = mouseButtonsState;
        switch (message)
        {
        case WM_LBUTTONDOWN:
            newMouseButtonsState |= MK_LBUTTON;
            break;
        case WM_LBUTTONUP:
            newMouseButtonsState &= ~MK_LBUTTON;
            break;
        case WM_RBUTTONDOWN:
            newMouseButtonsState |= MK_RBUTTON;
            break;
        case WM_RBUTTONUP:
            newMouseButtonsState &= ~MK_RBUTTON;
            break;
        case WM_MBUTTONDOWN:
            newMouseButtonsState |= MK_MBUTTON;
            break;
        case WM_MBUTTONUP:
            newMouseButtonsState &= ~MK_MBUTTON;
            break;
        case WM_XBUTTONDOWN:
            newMouseButtonsState |= (xbutton == XBUTTON1 ? MK_XBUTTON1 : MK_XBUTTON2);
            break;
        case WM_XBUTTONUP:
            newMouseButtonsState &= ~(xbutton == XBUTTON1 ? MK_XBUTTON1 : MK_XBUTTON2);
            break;
        default:
            return 0;
        }

        bool isPressed = false;
        eMouseButtons button = GetMouseButtonLegacy(mouseButtonsState, newMouseButtonsState, &isPressed);
        if (button != eMouseButtons::NONE)
        {
            eModifierKeys modifierKeys = GetModifierKeys();
            float32 vx = static_cast<float32>(x) / dipSize;
            float32 vy = static_cast<float32>(y) / dipSize;
            MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::MOUSE_BUTTON_DOWN : MainDispatcherEvent::MOUSE_BUTTON_UP;
            bool isRelative = (captureMode == eCursorCapture::PINNING);
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, type, button, vx, vy, 1, modifierKeys, isRelative));

            bool setCapture = newMouseButtonsState != 0 && mouseButtonsState == 0;
            mouseButtonsState = newMouseButtonsState;
            if (setCapture)
            {
                ::SetCapture(hwnd);
            }
            else if (newMouseButtonsState == 0)
            {
                ::ReleaseCapture();
            }
        }
    }
    return 0;
}

LRESULT WindowImpl::OnCaptureChanged()
{
    if (mouseButtonsState != 0)
    {
        POINT pt;
        ::GetCursorPos(&pt);
        ::ScreenToClient(hwnd, &pt);

        eModifierKeys modifierKeys = GetModifierKeys();
        float32 vx = static_cast<float32>(pt.x);
        float32 vy = static_cast<float32>(pt.y);
        MainDispatcherEvent e = MainDispatcherEvent::CreateWindowMouseClickEvent(window, MainDispatcherEvent::MOUSE_BUTTON_UP, eMouseButtons::LEFT, vx, vy, 1, modifierKeys, false);

        if (mouseButtonsState & MK_LBUTTON)
        {
            e.mouseEvent.button = eMouseButtons::LEFT;
            mainDispatcher->PostEvent(e);
        }
        if (mouseButtonsState & MK_RBUTTON)
        {
            e.mouseEvent.button = eMouseButtons::RIGHT;
            mainDispatcher->PostEvent(e);
        }
        if (mouseButtonsState & MK_MBUTTON)
        {
            e.mouseEvent.button = eMouseButtons::MIDDLE;
            mainDispatcher->PostEvent(e);
        }
        if (mouseButtonsState & MK_XBUTTON1)
        {
            e.mouseEvent.button = eMouseButtons::EXTENDED1;
            mainDispatcher->PostEvent(e);
        }
        if (mouseButtonsState & MK_XBUTTON2)
        {
            e.mouseEvent.button = eMouseButtons::EXTENDED2;
            mainDispatcher->PostEvent(e);
        }

        mouseButtonsState = 0;
    }
    return 0;
}

LRESULT WindowImpl::OnTouch(uint32 ntouch, HTOUCHINPUT htouch)
{
    touchInput.resize(ntouch);
    TOUCHINPUT* pinput = touchInput.data();
    if (::GetTouchInputInfo(htouch, ntouch, pinput, sizeof(TOUCHINPUT)))
    {
        eModifierKeys modifierKeys = GetModifierKeys();
        MainDispatcherEvent e = MainDispatcherEvent::CreateWindowTouchEvent(window, MainDispatcherEvent::TOUCH_MOVE, 0, 0.f, 0.f, modifierKeys);
        for (TOUCHINPUT& touch : touchInput)
        {
            POINT pt = { touch.x / 100, touch.y / 100 };
            ::ScreenToClient(hwnd, &pt);
            if (touch.dwFlags & (TOUCHEVENTF_PRIMARY | TOUCHEVENTF_MOVE))
            {
                // Remember move position of primary touch point to skip spurious move events as
                // Windows generates WM_MOUSEMOVE event for primary touch point.
                lastMouseMoveX = pt.x;
                lastMouseMoveY = pt.y;
            }

            if (touch.dwFlags & TOUCHEVENTF_MOVE)
                e.type = MainDispatcherEvent::TOUCH_MOVE;
            else if (touch.dwFlags & TOUCHEVENTF_DOWN)
                e.type = MainDispatcherEvent::TOUCH_DOWN;
            else if (touch.dwFlags & TOUCHEVENTF_UP)
                e.type = MainDispatcherEvent::TOUCH_UP;
            else
                continue;
            e.touchEvent.touchId = static_cast<uint32>(touch.dwID);
            e.touchEvent.x = static_cast<float32>(pt.x) / dipSize;
            e.touchEvent.y = static_cast<float32>(pt.y) / dipSize;
            mainDispatcher->PostEvent(e);
        }
        ::CloseTouchInputHandle(htouch);
    }
    return 0;
}

LRESULT WindowImpl::OnPointerClick(uint32 pointerId, int32 x, int32 y)
{
    POINTER_INFO pointerInfo;
    DllImport::fnGetPointerInfo(pointerId, &pointerInfo);

    bool isPressed = false;
    float32 vx = static_cast<float32>(x) / dipSize;
    float32 vy = static_cast<float32>(y) / dipSize;
    eModifierKeys modifierKeys = GetModifierKeys();
    if (pointerInfo.pointerType == PT_MOUSE)
    {
        eMouseButtons button = GetMouseButton(pointerInfo.ButtonChangeType, &isPressed);
        MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::MOUSE_BUTTON_DOWN : MainDispatcherEvent::MOUSE_BUTTON_UP;
        bool isRelative = (captureMode == eCursorCapture::PINNING);
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, type, button, vx, vy, 1, modifierKeys, isRelative));
    }
    else if (pointerInfo.pointerType == PT_TOUCH)
    {
        isPressed = (pointerInfo.pointerFlags & POINTER_FLAG_DOWN) == POINTER_FLAG_DOWN;
        MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::TOUCH_DOWN : MainDispatcherEvent::TOUCH_UP;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, type, pointerId, vx, vy, modifierKeys));
    }
    return 0;
}

LRESULT WindowImpl::OnPointerUpdate(uint32 pointerId, int32 x, int32 y)
{
    POINTER_INFO pointerInfo;
    DllImport::fnGetPointerInfo(pointerId, &pointerInfo);

    float32 vx = static_cast<float32>(x) / dipSize;
    float32 vy = static_cast<float32>(y) / dipSize;
    eModifierKeys modifierKeys = GetModifierKeys();
    if (pointerInfo.pointerType == PT_MOUSE)
    {
        bool isRelative = (captureMode == eCursorCapture::PINNING);
        if (pointerInfo.ButtonChangeType != POINTER_CHANGE_NONE)
        {
            // First mouse button down (and last mouse button up) comes with WM_POINTERDOWN/WM_POINTERUP, other mouse clicks come here
            bool isPressed = false;
            eMouseButtons button = GetMouseButton(pointerInfo.ButtonChangeType, &isPressed);
            MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::MOUSE_BUTTON_DOWN : MainDispatcherEvent::MOUSE_BUTTON_UP;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, type, button, vx, vy, 1, modifierKeys, isRelative));
        }
        if (isRelative)
        {
            return OnMouseMoveRelativeEvent(x, y);
        }
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, vx, vy, modifierKeys, false));
    }
    else if (pointerInfo.pointerType == PT_TOUCH)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, MainDispatcherEvent::TOUCH_MOVE, pointerId, vx, vy, modifierKeys));
    }
    return 0;
}

LRESULT WindowImpl::OnKeyEvent(uint32 keyVirtual, uint32 keyScancode, bool isPressed, bool isExtended, bool isRepeated)
{
    // Handle shifts separately to workaround some windows behaviours (see comment inside of OnShiftKeyEvent)
    if (keyVirtual == VK_SHIFT)
    {
        return OnShiftKeyEvent();
    }
    else
    {
        if (isExtended)
        {
            // Windows uses 0xE000 mask throughout its API to distinguish between extended and non-extended keys
            // So, follow this convention and use the same mask
            keyScancode = 0xE000 | keyScancode;
        }

        eModifierKeys modifierKeys = GetModifierKeys();
        MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, type, keyScancode, keyVirtual, modifierKeys, isRepeated));
        return 0;
    }
}

LRESULT WindowImpl::OnShiftKeyEvent()
{
    // Windows does not send event with separate WM_KEYUP for second shift if first one is still pressed
    // So if it's a shift key event, request and store every shift state explicitly

    // These are left and right shift scancodes, taken from https://msdn.microsoft.com/en-us/library/aa299374(v=vs.60).aspx
    static const uint32 shiftKeyScancodes[2] = { 0x2A, 0x36 };

    static const uint32 shiftKeyVirtuals[2] = { VK_LSHIFT, VK_RSHIFT };

    const bool lshiftPressed = ::GetKeyState(VK_LSHIFT) & 0x8000 ? true : false;
    const bool rshiftPressed = ::GetKeyState(VK_RSHIFT) & 0x8000 ? true : false;
    const bool currentShiftStates[2] = { lshiftPressed, rshiftPressed };

    eModifierKeys modifierKeys = GetModifierKeys();

    for (int i = 0; i < 2; ++i)
    {
        if (lastShiftStates[i] != currentShiftStates[i])
        {
            const MainDispatcherEvent::eType eventType = currentShiftStates[i] ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, eventType, shiftKeyScancodes[i], shiftKeyVirtuals[i], modifierKeys, false));
        }
        else if (currentShiftStates[i] == true)
        {
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_DOWN, shiftKeyScancodes[i], shiftKeyVirtuals[i], modifierKeys, true));
        }

        lastShiftStates[i] = currentShiftStates[i];
    }

    return 0;
}

LRESULT WindowImpl::OnCharEvent(uint32 key, bool isRepeated)
{
    eModifierKeys modifierKeys = GetModifierKeys();
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_CHAR, 0, key, modifierKeys, isRepeated));
    return 0;
}

LRESULT WindowImpl::OnCreate()
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);

    uiDispatcher.LinkToCurrentThread();
    hcurCursor = defaultCursor;

    // If new pointer input is available then do not handle legacy WM_TOUCH message
    if (DllImport::fnIsMouseInPointerEnabled == nullptr || !DllImport::fnIsMouseInPointerEnabled())
    {
        ::RegisterTouchWindow(hwnd, TWF_FINETOUCH | TWF_WANTPALM);
    }

    lastWidth = rc.right - rc.left;
    lastHeight = rc.bottom - rc.top;

    dpi = GetDpi();
    dipSize = dpi / defaultDpi;

    float32 width = std::ceil(static_cast<float32>(lastWidth) / dipSize);
    float32 height = std::ceil(static_cast<float32>(lastHeight) / dipSize);
    float32 surfaceWidth = static_cast<float32>(lastWidth) * surfaceScale;
    float32 surfaceHeight = static_cast<float32>(lastHeight) * surfaceScale;

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCreatedEvent(window, width, height, surfaceWidth, surfaceHeight, dpi, eFullscreen::Off));
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
    return 0;
}

bool WindowImpl::OnClose()
{
    if (!closeRequestByApp)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateUserCloseRequestEvent(window));
    }
    return closeRequestByApp;
}

bool WindowImpl::OnSysCommand(int sysCommand)
{
    // Ignore 'Move' and 'Size' commands from system menu as handling them takes more efforts than brings profit.
    // Window still can be moved and sized by mouse.
    // Also prevent system menu from showing triggered by keyboard (Alt+Space).
    if (sysCommand == SC_MOVE || sysCommand == SC_SIZE || sysCommand == SC_KEYMENU)
    {
        return true;
    }

    // If screen timeout is disabled and window is visible, do not show screen saver
    if (sysCommand == SC_SCREENSAVE && window->IsVisible() && !engineBackend->IsScreenTimeoutEnabled())
    {
        return true;
    }

    return false;
}

LRESULT WindowImpl::OnInputLanguageChanged()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateInputLanguageChangedEvent());
    return 0;
}

LRESULT WindowImpl::OnDestroy()
{
    if (!isMinimized)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, false));
    }

    mainDispatcher->SendEvent(MainDispatcherEvent::CreateWindowDestroyedEvent(window));
    hwnd = nullptr;
    return 0;
}

LRESULT WindowImpl::WindowProc(UINT message, WPARAM wparam, LPARAM lparam, bool& isHandled)
{
    // Intentionally use 'if' instead of 'switch'
    LRESULT lresult = 0;
    if (message == WM_ACTIVATE)
    {
        lresult = OnActivate(wparam);
    }
    else if (message == WM_SIZE)
    {
        int32 w = GET_X_LPARAM(lparam);
        int32 h = GET_Y_LPARAM(lparam);
        lresult = OnSize(static_cast<int32>(wparam), w, h);
    }
    else if (message == WM_ERASEBKGND)
    {
        lresult = 1;
    }
    else if (message == WM_GETMINMAXINFO)
    {
        MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lparam);
        lresult = OnGetMinMaxInfo(minMaxInfo);
    }
    else if (message == WM_ENTERSIZEMOVE)
    {
        lresult = OnEnterSizeMove();
    }
    else if (message == WM_EXITSIZEMOVE)
    {
        lresult = OnExitSizeMove();
    }
    else if (message == WM_SETCURSOR)
    {
        lresult = OnSetCursor(lparam);
        isHandled = false;
    }
    else if (message == WM_POINTERDOWN || message == WM_POINTERUP)
    {
        uint32 pointerId = GET_POINTERID_WPARAM(wparam);
        POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ::ScreenToClient(hwnd, &pt);

        lresult = OnPointerClick(pointerId, pt.x, pt.y);
    }
    else if (message == WM_POINTERUPDATE)
    {
        uint32 pointerId = GET_POINTERID_WPARAM(wparam);
        POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ::ScreenToClient(hwnd, &pt);

        lresult = OnPointerUpdate(pointerId, pt.x, pt.y);
    }
    else if (message == WM_POINTERWHEEL || message == WM_POINTERHWHEEL)
    {
        int32 deltaX = 0;
        int32 deltaY = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        if (message == WM_POINTERHWHEEL)
        {
            std::swap(deltaX, deltaY);
        }
        POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ::ScreenToClient(hwnd, &pt);

        lresult = OnMouseWheelEvent(deltaX, deltaY, pt.x, pt.y);
    }
    else if (WM_MOUSEFIRST <= message && message <= WM_MOUSELAST)
    {
        if (message == WM_MOUSEMOVE)
        {
            int32 x = GET_X_LPARAM(lparam);
            int32 y = GET_Y_LPARAM(lparam);
            lresult = OnMouseMoveEvent(x, y);
        }
        else if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
        {
            int32 deltaX = 0;
            int32 deltaY = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
            if (message == WM_MOUSEHWHEEL)
            {
                std::swap(deltaX, deltaY);
            }

            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            ::ScreenToClient(hwnd, &pt);

            lresult = OnMouseWheelEvent(deltaX, deltaY, pt.x, pt.y);
        }
        else
        {
            uint16 xbutton = GET_XBUTTON_WPARAM(wparam);
            int32 x = GET_X_LPARAM(lparam);
            int32 y = GET_Y_LPARAM(lparam);
            lresult = OnMouseClickEvent(message, xbutton, x, y);
        }
    }
    else if (message == WM_CAPTURECHANGED)
    {
        lresult = OnCaptureChanged();
    }
    else if (message == WM_TOUCH)
    {
        uint32 ntouch = LOWORD(wparam);
        HTOUCHINPUT htouch = reinterpret_cast<HTOUCHINPUT>(lparam);
        lresult = OnTouch(ntouch, htouch);
    }
    else if (message == WM_KEYUP || message == WM_KEYDOWN || message == WM_SYSKEYUP || message == WM_SYSKEYDOWN)
    {
        uint32 keyVirtual = static_cast<uint32>(wparam);
        uint32 keyScancode = (static_cast<uint32>(lparam) >> 16) & 0xFF;
        bool isPressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        bool isExtended = (HIWORD(lparam) & KF_EXTENDED) == KF_EXTENDED;
        bool isRepeated = (HIWORD(lparam) & KF_REPEAT) == KF_REPEAT;
        lresult = OnKeyEvent(keyVirtual, keyScancode, isPressed, isExtended, isRepeated);
        // Mark only WM_SYSKEYUP message as handled to prevent entering modal loop when Alt is released,
        // but leave WM_SYSKEYDOWN and other as unhandled to allow system shortcuts such as Alt+F4.
        isHandled = (message == WM_SYSKEYUP);
    }
    else if (message == WM_UNICHAR)
    {
        uint32 key = static_cast<uint32>(wparam);
        if (key != UNICODE_NOCHAR)
        {
            bool isRepeated = (HIWORD(lparam) & KF_REPEAT) == KF_REPEAT;
            lresult = OnCharEvent(key, isRepeated);
        }
        else
        {
            lresult = TRUE;
        }
    }
    else if (message == WM_CHAR || message == WM_SYSCHAR)
    {
        uint32 key = static_cast<uint32>(wparam);
        bool isRepeated = (HIWORD(lparam) & KF_REPEAT) == KF_REPEAT;
        lresult = OnCharEvent(key, isRepeated);
        // Leave WM_SYSCHAR unhandled to allow system shortcuts such as Alt+F4.
        isHandled = (message == WM_CHAR);
    }
    else if (message == WM_TRIGGER_EVENTS)
    {
        ProcessPlatformEvents();
    }
    else if (message == WM_CREATE)
    {
        lresult = OnCreate();
    }
    else if (message == WM_CLOSE)
    {
        isHandled = !OnClose();
    }
    else if (message == WM_DESTROY)
    {
        lresult = OnDestroy();
    }
    else if (message == WM_DPICHANGED)
    {
        RECT* suggestedRect = reinterpret_cast<RECT*>(lparam);
        lresult = OnDpiChanged(suggestedRect);
    }
    else if (message == WM_DISPLAYCHANGE)
    {
        engineBackend->UpdateDisplayConfig();
    }
    else if (message == WM_ENTERMENULOOP)
    {
        lresult = OnEnterMenuLoop();
    }
    else if (message == WM_EXITMENULOOP)
    {
        lresult = OnExitMenuLoop();
    }
    else if (message == WM_SYSCOMMAND)
    {
        isHandled = OnSysCommand(static_cast<int>(wparam));
    }
    else if (message == WM_INPUTLANGCHANGE)
    {
        lresult = OnInputLanguageChanged();
    }
    else
    {
        isHandled = false;
    }
    return lresult;
}

LRESULT CALLBACK WindowImpl::WndProcStart(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    bool isHandled = true;
    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
        WindowImpl* pthis = static_cast<WindowImpl*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(pthis));
        pthis->hwnd = hwnd;
    }

    // NOTE: first message coming to wndproc is not always WM_NCCREATE
    // It can be e.g. WM_GETMINMAXINFO, so do not handle all messages before WM_NCCREATE
    LRESULT lresult = 0;
    WindowImpl* pthis = reinterpret_cast<WindowImpl*>(GetWindowLongPtrW(hwnd, 0));
    if (pthis != nullptr)
    {
        lresult = pthis->WindowProc(message, wparam, lparam, isHandled);
        if (message == WM_NCDESTROY)
        {
            isHandled = false;
        }
    }
    else
    {
        isHandled = false;
    }

    if (!isHandled)
    {
        return ::DefWindowProc(hwnd, message, wparam, lparam);
    }
    return lresult;
}

bool WindowImpl::RegisterWindowClass()
{
    if (!windowClassRegistered)
    {
        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = &WndProcStart;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(void*); // Reserve room to store 'this' pointer
        wcex.hInstance = PlatformCore::Win32AppInstance();
        wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hCursor = nullptr;
        wcex.hbrBackground = nullptr;
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = windowClassName;
        wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        windowClassRegistered = ::RegisterClassExW(&wcex) != 0;

        defaultCursor = ::LoadCursor(nullptr, IDC_ARROW);
    }
    return windowClassRegistered;
}

float32 WindowImpl::GetDpi() const
{
    float32 result = 0.0f;
    if (DllImport::fnGetDpiForMonitor != nullptr)
    {
        UINT x = 0;
        UINT y = 0;
        HMONITOR hmonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
        DllImport::fnGetDpiForMonitor(hmonitor, MDT_EFFECTIVE_DPI, &x, &y);
        result = static_cast<float32>(x);
    }
    else
    {
        // default behavior for Windows with version < 8.1
        HDC hdcScreen = ::GetDC(nullptr);
        result = static_cast<float32>(::GetDeviceCaps(hdcScreen, LOGPIXELSX));
        ::ReleaseDC(nullptr, hdcScreen);
    }
    return result;
}

eModifierKeys WindowImpl::GetModifierKeys()
{
    eModifierKeys result = eModifierKeys::NONE;
    BYTE keyState[256];
    if (::GetKeyboardState(keyState))
    {
        if ((keyState[VK_LSHIFT] & 0x80) || (keyState[VK_RSHIFT] & 0x80))
        {
            result |= eModifierKeys::SHIFT;
        }
        if ((keyState[VK_LCONTROL] & 0x80) || (keyState[VK_RCONTROL] & 0x80))
        {
            result |= eModifierKeys::CONTROL;
        }
        if ((keyState[VK_LMENU] & 0x80) || (keyState[VK_RMENU] & 0x80))
        {
            result |= eModifierKeys::ALT;
        }
    }
    return result;
}

eInputDevices WindowImpl::GetInputEventSourceLegacy(LPARAM messageExtraInfo)
{
    // How to distinguish pen input from mouse and touch
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms703320(v=vs.85).aspx

    const LPARAM MI_WP_SIGNATURE = 0xFF515700;
    const LPARAM SIGNATURE_MASK = 0xFFFFFF00;

    if ((messageExtraInfo & SIGNATURE_MASK) == MI_WP_SIGNATURE)
    {
        return eInputDevices::TOUCH_SURFACE;
    }
    return eInputDevices::MOUSE;
}

eMouseButtons WindowImpl::GetMouseButtonLegacy(uint32 curState, uint32 newState, bool* isPressed)
{
    uint32 changed = curState ^ newState;
    if (changed & MK_LBUTTON)
    {
        *isPressed = (newState & MK_LBUTTON) != 0;
        return eMouseButtons::LEFT;
    }
    if (changed & MK_RBUTTON)
    {
        *isPressed = (newState & MK_RBUTTON) != 0;
        return eMouseButtons::RIGHT;
    }
    if (changed & MK_MBUTTON)
    {
        *isPressed = (newState & MK_MBUTTON) != 0;
        return eMouseButtons::MIDDLE;
    }
    if (changed & MK_XBUTTON1)
    {
        *isPressed = (newState & MK_XBUTTON1) != 0;
        return eMouseButtons::EXTENDED1;
    }
    if (changed & MK_XBUTTON2)
    {
        *isPressed = (newState & MK_XBUTTON2) != 0;
        return eMouseButtons::EXTENDED2;
    }
    return eMouseButtons::NONE;
}

eMouseButtons WindowImpl::GetMouseButton(POINTER_BUTTON_CHANGE_TYPE buttonChangeType, bool* isPressed)
{
    *isPressed = false;
    switch (buttonChangeType)
    {
    case POINTER_CHANGE_FIRSTBUTTON_DOWN:
        *isPressed = true;
    case POINTER_CHANGE_FIRSTBUTTON_UP:
        return eMouseButtons::LEFT;
    case POINTER_CHANGE_SECONDBUTTON_DOWN:
        *isPressed = true;
    case POINTER_CHANGE_SECONDBUTTON_UP:
        return eMouseButtons::RIGHT;
    case POINTER_CHANGE_THIRDBUTTON_DOWN:
        *isPressed = true;
    case POINTER_CHANGE_THIRDBUTTON_UP:
        return eMouseButtons::MIDDLE;
    case POINTER_CHANGE_FOURTHBUTTON_DOWN:
        *isPressed = true;
    case POINTER_CHANGE_FOURTHBUTTON_UP:
        return eMouseButtons::EXTENDED1;
    case POINTER_CHANGE_FIFTHBUTTON_DOWN:
        *isPressed = true;
    case POINTER_CHANGE_FIFTHBUTTON_UP:
        return eMouseButtons::EXTENDED2;
    default:
        return eMouseButtons::NONE;
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
