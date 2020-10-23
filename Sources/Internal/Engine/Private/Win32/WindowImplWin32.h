#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_WIN32__)

#include <bitset>

#include "Base/Platform.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/Dispatcher/UIDispatcher.h"

namespace rhi
{
struct InitParam;
}

namespace DAVA
{
namespace Private
{
class WindowImpl final
{
public:
    WindowImpl(EngineBackend* engineBackend, Window* window);
    ~WindowImpl();

    WindowImpl(const WindowImpl&) = delete;
    WindowImpl& operator=(const WindowImpl&) = delete;

    bool Create(float32 width, float32 height);
    void Resize(float32 width, float32 height);
    void Activate();
    void Close(bool appIsTerminating);
    void SetTitle(const String& title);
    void SetMinimumSize(Size2f size);
    void SetFullscreen(eFullscreen newMode);

    void RunAsyncOnUIThread(const Function<void()>& task);
    void RunAndWaitOnUIThread(const Function<void()>& task);

    void* GetHandle() const;
    HWND GetHWND() const;

    void SetIcon(const wchar_t* iconResourceName);
    void SetCursor(HCURSOR hcursor);

    bool IsWindowReadyForRender() const;
    void InitCustomRenderParams(rhi::InitParam& params);

    void TriggerPlatformEvents();
    void ProcessPlatformEvents();

    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);

    void SetSurfaceScaleAsync(const float32 scale);

private:
    // Shortcut for eMouseButtons::COUNT
    static const size_t MOUSE_BUTTON_COUNT = static_cast<size_t>(eMouseButtons::COUNT);

    // Flags used with DoResizeWindow
    enum eResizeFlags
    {
        CENTER_ON_DISPLAY = 0x01, //!< Resize and place window on the center of display
        RESIZE_WHOLE_WINDOW = 0x02, //!< Size given for whole window including border and caption
        NO_TRANSLATE_TO_DIPS = 0x04, //!< Do not translate size to DIPs
    };

    void DoSetSurfaceScale(const float32 scale);

    void SetCursorInCenter();
    void DoResizeWindow(float32 width, float32 height, int resizeFlags);
    void DoActivateWindow();
    void DoCloseWindow();
    void DoSetTitle(const char8* title);
    void DoSetMinimumSize(float32 width, float32 height);
    void DoSetFullscreen(eFullscreen newMode);

    void SetFullscreenMode();
    void SetWindowedMode();
    void DoSetCursorCapture(eCursorCapture mode);
    void DoSetCursorVisibility(bool visible);
    void SwitchToPinning();
    void UpdateClipCursor();
    void HandleWindowFocusChanging(bool hasFocus);

    void HandleSizeChanged(int32 w, int32 h, bool dpiChanged);

    void UIEventHandler(const UIDispatcherEvent& e);

    LRESULT OnSize(int32 resizingType, int32 width, int32 height);
    LRESULT OnEnterSizeMove();
    LRESULT OnExitSizeMove();
    LRESULT OnEnterMenuLoop();
    LRESULT OnExitMenuLoop();
    LRESULT OnGetMinMaxInfo(MINMAXINFO* minMaxInfo);
    LRESULT OnDpiChanged(RECT* suggestedRect);
    LRESULT OnActivate(WPARAM wparam);
    LRESULT OnMouseMoveEvent(int32 x, int32 y);
    LRESULT OnMouseMoveRelativeEvent(int x, int y);
    LRESULT OnMouseWheelEvent(int32 deltaX, int32 deltaY, int32 x, int32 y);
    LRESULT OnMouseClickEvent(UINT message, uint16 xbutton, int32 x, int32 y);
    LRESULT OnCaptureChanged();
    LRESULT OnTouch(uint32 ntouch, HTOUCHINPUT htouch);
    LRESULT OnPointerClick(uint32 pointerId, int32 x, int32 y);
    LRESULT OnPointerUpdate(uint32 pointerId, int32 x, int32 y);
    LRESULT OnKeyEvent(uint32 key, uint32 scanCode, bool isPressed, bool isExtended, bool isRepeated);
    LRESULT OnShiftKeyEvent();
    LRESULT OnCharEvent(uint32 key, bool isRepeated);
    LRESULT OnCreate();
    LRESULT OnSetCursor(LPARAM lparam);
    bool OnClose();
    bool OnSysCommand(int sysCommand);
    LRESULT OnInputLanguageChanged();
    LRESULT OnDestroy();
    LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam, bool& isHandled);
    static LRESULT CALLBACK WndProcStart(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static bool RegisterWindowClass();
    static eModifierKeys GetModifierKeys();
    static eInputDevices GetInputEventSourceLegacy(LPARAM messageExtraInfo);
    static eMouseButtons GetMouseButtonLegacy(uint32 curState, uint32 newState, bool* isPressed);
    static eMouseButtons GetMouseButton(POINTER_BUTTON_CHANGE_TYPE buttonChangeType, bool* isPressed);

    float32 GetDpi() const;

private:
    EngineBackend* engineBackend = nullptr;
    Window* window = nullptr; // Window frontend reference
    MainDispatcher* mainDispatcher = nullptr; // Dispatcher that dispatches events to DAVA main thread
    UIDispatcher uiDispatcher; // Dispatcher that dispatches events to window UI thread

    HWND hwnd = nullptr;
    HCURSOR hcurCursor = nullptr;

    bool isMinimized = false;
    bool hasFocus = false;

    bool isEnteredSizingModalLoop = false;
    bool closeRequestByApp = false;
    bool isFullscreen = false;
    int minWidth = 128;
    int minHeight = 128;
    int32 lastWidth = 0; // Track current window size to not post excessive WINDOW_SIZE_CHANGED events
    int32 lastHeight = 0;

    float32 surfaceScale = 1.0f;

    int32 lastMouseMoveX = -1; // Remember last mouse move position to detect
    int32 lastMouseMoveY = -1; // spurious mouse move events
    uint32 mouseButtonsState = 0; // Mouse buttons state for legacy mouse events (not new pointer input events)
    int32 mouseMoveSkipCount = 0;
    const int32 SKIP_N_MOUSE_MOVE_EVENTS = 2;

    bool forceCursorHide = false;
    eCursorCapture captureMode = eCursorCapture::OFF;
    bool mouseVisible = true;
    POINT lastCursorPosition;

    const float32 defaultDpi = 96.0f;
    float32 dpi = defaultDpi;

    // DIP is device independent pixel.
    // dipSize is number of physical pixels contained in one DIP.
    // Here DIPs are used to emulate Win10 behaviour on Win32:
    //  - window size is always in DIPS
    //  - surface rendering size in physical pixels
    //
    // More info here: https://msdn.microsoft.com/en-us/library/windows/desktop/ff684173(v=vs.85).aspx
    float32 dipSize = 1.f;
    Vector<TOUCHINPUT> touchInput;
    WINDOWPLACEMENT windowPlacement;

    static HCURSOR defaultCursor;
    static bool windowClassRegistered;
    static const wchar_t windowClassName[];
    static const UINT WM_TRIGGER_EVENTS = WM_USER + 39;
    static const DWORD windowedStyle = WS_OVERLAPPEDWINDOW;
    static const DWORD fullscreenStyle = WS_POPUP;
    static const DWORD windowExStyle = 0;

    bool lastShiftStates[2];
};

inline void* WindowImpl::GetHandle() const
{
    return static_cast<void*>(hwnd);
}

inline HWND WindowImpl::GetHWND() const
{
    return hwnd;
}

inline void WindowImpl::InitCustomRenderParams(rhi::InitParam& /*params*/)
{
    // No custom render params
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
