#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include <bitset>

#include "Engine/EngineTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/EngineTypes.h"

namespace DAVA
{
namespace Private
{
ref struct WindowNativeBridge sealed
{
    internal :
    WindowNativeBridge(WindowImpl* windowImpl);

    void* GetHandle() const;

    void BindToXamlWindow(::Windows::UI::Xaml::Window ^ xamlWindow_);

    void AddXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl);
    void RemoveXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl);
    void PositionXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl, float32 x, float32 y);
    void UnfocusXamlControl();
    ::Windows::UI::Xaml::Input::Pointer ^ GetLastPressedPointer() const;

    void TriggerPlatformEvents();

    void ResizeWindow(float32 width, float32 height);
    void CloseWindow();
    void SetTitle(const char8* title);
    void SetMinimumSize(float32 width, float32 height);
    void SetFullscreen(eFullscreen newMode);
    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);

    void SetSurfaceScale(const float32 scale);

private:
    void OnTriggerPlatformEvents();
    void HandleSizeChanged(float32 w, float32 h, bool dpiChanged);
    void HandleFocusChanging(bool gotFocus);

    void OnActivated(Windows::UI::Core::CoreWindow ^ coreWindow, Windows::UI::Core::WindowActivatedEventArgs ^ arg);
    void OnVisibilityChanged(Windows::UI::Core::CoreWindow ^ coreWindow, Windows::UI::Core::VisibilityChangedEventArgs ^ arg);

    void OnCharacterReceived(::Windows::UI::Core::CoreWindow ^ coreWindow, ::Windows::UI::Core::CharacterReceivedEventArgs ^ arg);
    void OnAcceleratorKeyActivated(::Windows::UI::Core::CoreDispatcher ^ dispatcher, ::Windows::UI::Core::AcceleratorKeyEventArgs ^ arg);
    void OnShiftKeyActivated();

    void OnSizeChanged(::Platform::Object ^ sender, ::Windows::UI::Xaml::SizeChangedEventArgs ^ arg);
    void OnCompositionScaleChanged(::Windows::UI::Xaml::Controls::SwapChainPanel ^ panel, ::Platform::Object ^ obj);

    void OnPointerPressed(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnPointerReleased(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnPointerCaptureLost(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnPointerCancelled(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnPointerMoved(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnPointerWheelChanged(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg);
    void OnMouseMoved(Windows::Devices::Input::MouseDevice ^ mouseDevice, ::Windows::Devices::Input::MouseEventArgs ^ args);

    void OnKeyboardShowing(Windows::UI::ViewManagement::InputPane ^ sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^ args);
    void OnKeyboardHiding(Windows::UI::ViewManagement::InputPane ^ sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^ args);

    eModifierKeys GetModifierKeys() const;
    static eMouseButtons GetMouseButtonState(::Windows::UI::Input::PointerUpdateKind buttonUpdateKind, bool* isPressed);

    void CreateBaseXamlUI();
    void InstallEventHandlers();
    void UninstallEventHandlers();

private:
    WindowImpl* windowImpl = nullptr;
    Window* window = nullptr;
    MainDispatcher* mainDispatcher = nullptr;

    float32 surfaceScale = 1.0f;

    ::Windows::UI::Xaml::Window ^ xamlWindow = nullptr;
    ::Windows::UI::Xaml::Controls::SwapChainPanel ^ xamlSwapChainPanel = nullptr;
    ::Windows::UI::Xaml::Controls::Canvas ^ xamlCanvas = nullptr;
    ::Windows::UI::Xaml::Controls::Button ^ xamlControlThatStealsFocus = nullptr;
    ::Windows::UI::Xaml::Input::Pointer ^ lastPressedPointer = nullptr;

    // List of pointer ids which are currently pressed
    Vector<uint32> pressedPointerIds;

    // Tokens & handler objects to unsubscribe from event handlers
    ::Windows::Foundation::EventRegistrationToken tokenActivated;
    ::Windows::Foundation::EventRegistrationToken tokenVisibilityChanged;
    ::Windows::Foundation::EventRegistrationToken tokenCharacterReceived;
    ::Windows::Foundation::EventRegistrationToken tokenAcceleratorKeyActivated;
    ::Windows::Foundation::EventRegistrationToken tokenSizeChanged;
    ::Windows::Foundation::EventRegistrationToken tokenCompositionScaleChanged;
    ::Windows::Foundation::EventRegistrationToken tokenPointerPressed;
    ::Windows::Foundation::EventRegistrationToken tokenPointerMoved;
    ::Windows::Foundation::EventRegistrationToken tokenPointerWheelChanged;
    ::Windows::Foundation::EventRegistrationToken tokenMouseMoved;
    ::Windows::Foundation::EventRegistrationToken tokenKeyboardShowing;
    ::Windows::Foundation::EventRegistrationToken tokenKeyboardHiding;
    ::Windows::UI::Xaml::Input::PointerEventHandler ^ pointerReleasedHandler;
    ::Windows::UI::Xaml::Input::PointerEventHandler ^ pointerCaptureLostHandler;
    ::Windows::UI::Xaml::Input::PointerEventHandler ^ pointerCancelledHandler;

    static ::Platform::String ^ xamlWorkaroundWebViewProblems;
    static ::Platform::String ^ xamlWorkaroundTextBoxProblems;

    ::Windows::UI::Core::CoreCursor ^ defaultCursor = ref new ::Windows::UI::Core::CoreCursor(::Windows::UI::Core::CoreCursorType::Arrow, 0);
    bool hasFocus = false;
    bool mouseVisible = true;
    eCursorCapture captureMode = eCursorCapture::OFF;
    uint32 mouseMoveSkipCount = 0;
    const uint32 SKIP_N_MOUSE_MOVE_EVENTS = 4;

    bool lastShiftStates[2];
};

inline void* WindowNativeBridge::GetHandle() const
{
    return reinterpret_cast<void*>(xamlSwapChainPanel);
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
