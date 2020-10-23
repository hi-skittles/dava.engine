#include "Engine/Private/Win10/WindowNativeBridgeWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Window.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#include "Engine/Private/Win10/WindowImplWin10.h"

#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
namespace Private
{
WindowNativeBridge::WindowNativeBridge(WindowImpl* windowImpl)
    : windowImpl(windowImpl)
    , window(windowImpl->window)
    , mainDispatcher(windowImpl->mainDispatcher)
{
    lastShiftStates[0] = lastShiftStates[1] = false;
}

void WindowNativeBridge::BindToXamlWindow(::Windows::UI::Xaml::Window ^ xamlWindow_)
{
    using ::Windows::Foundation::Size;
    using namespace ::Windows::UI::ViewManagement;

    DVASSERT(xamlWindow == nullptr);
    DVASSERT(xamlWindow_ != nullptr);

    xamlWindow = xamlWindow_;

    // By default window has dimension 1024x768
    ApplicationView::GetForCurrentView()->PreferredLaunchViewSize = Size(1024, 768);
    ApplicationView::PreferredLaunchWindowingMode = ApplicationViewWindowingMode::PreferredLaunchViewSize;

    // Limit minimum window size to some reasonable value
    ApplicationView::GetForCurrentView()->SetPreferredMinSize(Size(static_cast<float32>(Window::smallestWidth), static_cast<float32>(Window::smallestHeight)));
    ApplicationView::GetForCurrentView()->FullScreenSystemOverlayMode = FullScreenSystemOverlayMode::Minimal;

    {
        using ::Windows::Graphics::Display::DisplayInformation;
        using ::Windows::Graphics::Display::DisplayOrientations;

        // TODO: temporal hardcode, separate task for setting rotation
        DisplayInformation::GetForCurrentView()->AutoRotationPreferences = DisplayOrientations::Landscape | DisplayOrientations::LandscapeFlipped;
    }

    if (PlatformCore::IsPhoneContractPresent())
    {
        using ::Windows::UI::ViewManagement::StatusBar;
        StatusBar::GetForCurrentView()->HideAsync();
    }

    CreateBaseXamlUI();
    InstallEventHandlers();

    float32 w = xamlWindow->Bounds.Width;
    float32 h = xamlWindow->Bounds.Height;
    float32 surfW = w * xamlSwapChainPanel->CompositionScaleX * surfaceScale;
    float32 surfH = h * xamlSwapChainPanel->CompositionScaleY * surfaceScale;
    float32 dpi = ::Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
    eFullscreen fullscreen = ApplicationView::GetForCurrentView()->IsFullScreenMode ? eFullscreen::On : eFullscreen::Off;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCreatedEvent(window, w, h, surfW, surfH, dpi, fullscreen));

    xamlWindow->Activate();
}

void WindowNativeBridge::AddXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl)
{
    xamlCanvas->Children->Append(xamlControl);
}

void WindowNativeBridge::RemoveXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl)
{
    unsigned int index = 0;
    for (auto x = xamlCanvas->Children->First(); x->HasCurrent; x->MoveNext(), ++index)
    {
        if (x->Current == xamlControl)
        {
            xamlCanvas->Children->RemoveAt(index);
            break;
        }
    }
}

void WindowNativeBridge::PositionXamlControl(::Windows::UI::Xaml::UIElement ^ xamlControl, float32 x, float32 y)
{
    xamlCanvas->SetLeft(xamlControl, x);
    xamlCanvas->SetTop(xamlControl, y);
}

void WindowNativeBridge::UnfocusXamlControl()
{
    // XAML controls cannot be unfocused programmatically, this is especially useful for text fields
    // So use dummy offscreen control that steals focus
    xamlControlThatStealsFocus->Focus(::Windows::UI::Xaml::FocusState::Pointer);
}

::Windows::UI::Xaml::Input::Pointer ^ WindowNativeBridge::GetLastPressedPointer() const
{
    return lastPressedPointer;
}

void WindowNativeBridge::TriggerPlatformEvents()
{
    using namespace ::Windows::UI::Core;
    xamlWindow->Dispatcher->TryRunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler(this, &WindowNativeBridge::OnTriggerPlatformEvents));
}

void WindowNativeBridge::ResizeWindow(float32 width, float32 height)
{
    using namespace ::Windows::Foundation;
    using namespace ::Windows::UI::ViewManagement;

    // TODO: check width and height against zero, minimum window size and screen size
    ApplicationView ^ appView = ApplicationView::GetForCurrentView();
    appView->TryResizeView(Size(width, height));
}

void WindowNativeBridge::CloseWindow()
{
    // WinRT does not permit to close main window, so for primary window pretend that window has been closed.
    // For secondary window invoke Close() method, and also do not wait Closed event as stated in MSDN:
    //      Apps are typically suspended, not terminated. As a result, this event (Closed) is rarely fired, if ever.
    if (!window->IsPrimary())
    {
        xamlWindow->CoreWindow->Close();
    }

    UninstallEventHandlers();
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, false));
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, false));
    mainDispatcher->SendEvent(MainDispatcherEvent::CreateWindowDestroyedEvent(window));
}

void WindowNativeBridge::SetTitle(const char8* title)
{
    using ::Windows::UI::ViewManagement::ApplicationView;

    WideString wideTitle = UTF8Utils::EncodeToWideString(title);
    ApplicationView::GetForCurrentView()->Title = ref new ::Platform::String(wideTitle.c_str());
}

void WindowNativeBridge::SetMinimumSize(float32 width, float32 height)
{
    using ::Windows::Foundation::Size;
    using ::Windows::UI::ViewManagement::ApplicationView;

    ApplicationView::GetForCurrentView()->SetPreferredMinSize(Size(width, height));
}

void WindowNativeBridge::SetSurfaceScale(const float32 scale)
{
    surfaceScale = scale;

    const float32 width = static_cast<float32>(xamlSwapChainPanel->ActualWidth);
    const float32 height = static_cast<float32>(xamlSwapChainPanel->ActualHeight);
    HandleSizeChanged(width, height, false);
}

void WindowNativeBridge::SetFullscreen(eFullscreen newMode)
{
    using ::Windows::UI::ViewManagement::ApplicationView;
    bool isFullscreenRequested = newMode == eFullscreen::On;

    ApplicationView ^ view = ApplicationView::GetForCurrentView();
    if (isFullscreenRequested == view->IsFullScreenMode)
    {
        return;
    }

    if (isFullscreenRequested)
    {
        view->TryEnterFullScreenMode();
    }
    else
    {
        view->ExitFullScreenMode();
    }
}

void WindowNativeBridge::SetCursorVisibility(bool visible)
{
    if (mouseVisible != visible)
    {
        mouseVisible = visible;
        xamlWindow->CoreWindow->PointerCursor = visible ? defaultCursor : nullptr;
    }
}

void WindowNativeBridge::SetCursorCapture(eCursorCapture mode)
{
    using ::Windows::Foundation::TypedEventHandler;
    using ::Windows::Devices::Input::MouseDevice;
    using ::Windows::Devices::Input::MouseEventArgs;
    using ::Windows::UI::Xaml::Input::PointerEventHandler;

    if (captureMode != mode)
    {
        captureMode = mode;

        MouseDevice ^ mouseDevice = MouseDevice::GetForCurrentView();
        switch (captureMode)
        {
        case DAVA::eCursorCapture::OFF:
            mouseDevice->MouseMoved -= tokenMouseMoved;
            break;
        case DAVA::eCursorCapture::FRAME:
            // now, not implemented
            break;
        case DAVA::eCursorCapture::PINNING:
            tokenMouseMoved = mouseDevice->MouseMoved += ref new TypedEventHandler<MouseDevice ^, MouseEventArgs ^>(this, &WindowNativeBridge::OnMouseMoved);
            mouseMoveSkipCount = SKIP_N_MOUSE_MOVE_EVENTS;
            break;
        }
    }
}

void WindowNativeBridge::OnTriggerPlatformEvents()
{
    windowImpl->ProcessPlatformEvents();
}

void WindowNativeBridge::HandleFocusChanging(bool gotFocus)
{
    if (hasFocus != gotFocus)
    {
        hasFocus = gotFocus;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, hasFocus));

        PlatformCore::EnableHighResolutionTimer(hasFocus);

        if (!hasFocus)
        {
            if (captureMode != eCursorCapture::OFF)
            {
                SetCursorCapture(eCursorCapture::OFF);
                mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCaptureLostEvent(window));
            }
            SetCursorVisibility(true);
        }
    }
}

void WindowNativeBridge::OnActivated(Windows::UI::Core::CoreWindow ^ coreWindow, Windows::UI::Core::WindowActivatedEventArgs ^ arg)
{
    // System does not send Activated event for window when user locks (Win+L) and unlocks screen even if
    // user clicks on window after unlocking screen.
    // To ensure proper events delivering to dava.engine force window focus in mouse click and key press event
    // handlers (if hasFocus member variable is false).

    using ::Windows::UI::Core::CoreWindowActivationState;
    HandleFocusChanging(arg->WindowActivationState != CoreWindowActivationState::Deactivated);
}

void WindowNativeBridge::OnVisibilityChanged(Windows::UI::Core::CoreWindow ^ coreWindow, Windows::UI::Core::VisibilityChangedEventArgs ^ arg)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, arg->Visible));
}

void WindowNativeBridge::OnCharacterReceived(::Windows::UI::Core::CoreWindow ^ /*coreWindow*/, ::Windows::UI::Core::CharacterReceivedEventArgs ^ arg)
{
    eModifierKeys modifierKeys = GetModifierKeys();
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_CHAR, 0, arg->KeyCode, modifierKeys, arg->KeyStatus.WasKeyDown));
}

void WindowNativeBridge::OnAcceleratorKeyActivated(::Windows::UI::Core::CoreDispatcher ^ /*dispatcher*/, ::Windows::UI::Core::AcceleratorKeyEventArgs ^ arg)
{
    using ::Windows::System::VirtualKey;
    using namespace ::Windows::UI::Core;

    if (!hasFocus)
    { // See comment in OnActivated
        HandleFocusChanging(true);
    }

    // Process only KeyDown/KeyUp and SystemKeyDown/SystemKeyUp event types to skip unwanted messages, such as
    // Character (handled in OnCharacterReceived), DeadCharacter, SystemCharacter, etc.
    bool isPressed = false;
    CoreAcceleratorKeyEventType eventType = arg->EventType;
    switch (eventType)
    {
    case CoreAcceleratorKeyEventType::KeyDown:
    case CoreAcceleratorKeyEventType::SystemKeyDown:
        isPressed = true; // Fall through below
    case CoreAcceleratorKeyEventType::KeyUp:
    case CoreAcceleratorKeyEventType::SystemKeyUp:
    {
        // Handle shifts separately to workaround some windows behaviours (see comment inside of OnShiftKeyActivated)
        if (arg->VirtualKey == VirtualKey::Shift)
        {
            OnShiftKeyActivated();
        }
        else
        {
            eModifierKeys modifierKeys = GetModifierKeys();
            CorePhysicalKeyStatus status = arg->KeyStatus;
            uint32 keyScancode = status.ScanCode;
            uint32 keyVirtual = static_cast<uint32>(arg->VirtualKey);

            if (status.IsExtendedKey)
            {
                // Windows uses 0xE000 mask throughout its API to distinguish between extended and non-extended keys
                // So, follow this convention and use the same mask
                keyScancode |= 0xE000;
            }

            MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, type, keyScancode, keyVirtual, modifierKeys, status.WasKeyDown));
        }

        break;
    }
    default:
        break;
    }
}

void WindowNativeBridge::OnShiftKeyActivated()
{
    // Windows does not send event with separate keyup for second shift if first one is still pressed
    // So if it's a shift key event, request and store every shift state explicitly

    using ::Windows::System::VirtualKey;
    using namespace ::Windows::UI::Core;

    // These are left and right shift scancodes, taken from https://msdn.microsoft.com/en-us/library/aa299374(v=vs.60).aspx
    static const uint32 shiftKeyScancodes[2] = { 0x2A, 0x36 };

    static const uint32 shiftKeyVirtuals[2] = { static_cast<uint32>(VirtualKey::LeftShift), static_cast<uint32>(VirtualKey::RightShift) };

    CoreWindow ^ coreWindow = xamlWindow->CoreWindow;
    const bool lshiftPressed = static_cast<bool>(coreWindow->GetKeyState(VirtualKey::LeftShift) & CoreVirtualKeyStates::Down);
    const bool rshiftPressed = static_cast<bool>(coreWindow->GetKeyState(VirtualKey::RightShift) & CoreVirtualKeyStates::Down);
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
}

void WindowNativeBridge::HandleSizeChanged(float32 w, float32 h, bool dpiChanged)
{
    using ::Windows::Graphics::Display::DisplayInformation;
    using ::Windows::UI::ViewManagement::ApplicationView;

    float32 surfW = w * xamlSwapChainPanel->CompositionScaleX * surfaceScale;
    float32 surfH = h * xamlSwapChainPanel->CompositionScaleY * surfaceScale;
    float32 dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
    eFullscreen fullscreen = ApplicationView::GetForCurrentView()->IsFullScreenMode ? eFullscreen::On : eFullscreen::Off;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSizeChangedEvent(window, w, h, surfW, surfH, surfaceScale, dpi, fullscreen));

    if (dpiChanged)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowDpiChangedEvent(window, dpi));
    }
}

void WindowNativeBridge::OnSizeChanged(::Platform::Object ^ /*sender*/, ::Windows::UI::Xaml::SizeChangedEventArgs ^ arg)
{
    float32 w = arg->NewSize.Width;
    float32 h = arg->NewSize.Height;
    HandleSizeChanged(w, h, false);
}

void WindowNativeBridge::OnCompositionScaleChanged(::Windows::UI::Xaml::Controls::SwapChainPanel ^ /*panel*/, ::Platform::Object ^ /*obj*/)
{
    float32 w = static_cast<float32>(xamlSwapChainPanel->ActualWidth);
    float32 h = static_cast<float32>(xamlSwapChainPanel->ActualHeight);
    HandleSizeChanged(w, h, true);
}

void WindowNativeBridge::OnPointerPressed(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    using namespace ::Windows::UI::Input;
    using namespace ::Windows::Devices::Input;

    if (!hasFocus)
    { // See comment in OnActivated
        HandleFocusChanging(true);
    }

    lastPressedPointer = arg->Pointer;

    pressedPointerIds.push_back(lastPressedPointer->PointerId);

    PointerPoint ^ pointerPoint = arg->GetCurrentPoint(nullptr);
    PointerPointProperties ^ prop = pointerPoint->Properties;
    PointerDeviceType deviceType = pointerPoint->PointerDevice->PointerDeviceType;

    eModifierKeys modifierKeys = GetModifierKeys();
    float32 x = pointerPoint->Position.X;
    float32 y = pointerPoint->Position.Y;
    if (deviceType == PointerDeviceType::Mouse)
    {
        bool isPressed = false;
        eMouseButtons button = GetMouseButtonState(prop->PointerUpdateKind, &isPressed);
        bool isRelative = (captureMode == eCursorCapture::PINNING);
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, MainDispatcherEvent::MOUSE_BUTTON_DOWN, button, x, y, 1, modifierKeys, isRelative));
    }
    else if (deviceType == PointerDeviceType::Touch)
    {
        uint32 touchId = pointerPoint->PointerId;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, MainDispatcherEvent::TOUCH_DOWN, touchId, x, y, modifierKeys));
    }
}

void WindowNativeBridge::OnPointerReleased(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    using namespace ::Windows::UI::Input;
    using namespace ::Windows::Devices::Input;

    lastPressedPointer = nullptr;

    // Check if we had according PointerPressed event
    auto matchingPressedPointerIdPos = std::find(pressedPointerIds.begin(), pressedPointerIds.end(), arg->Pointer->PointerId);
    if (matchingPressedPointerIdPos != pressedPointerIds.end())
    {
        pressedPointerIds.erase(matchingPressedPointerIdPos);

        PointerPoint ^ pointerPoint = arg->GetCurrentPoint(nullptr);
        PointerPointProperties ^ prop = pointerPoint->Properties;
        PointerDeviceType deviceType = pointerPoint->PointerDevice->PointerDeviceType;

        eModifierKeys modifierKeys = GetModifierKeys();
        float32 x = pointerPoint->Position.X;
        float32 y = pointerPoint->Position.Y;
        if (deviceType == PointerDeviceType::Mouse)
        {
            bool isPressed = false;
            eMouseButtons button = GetMouseButtonState(prop->PointerUpdateKind, &isPressed);
            bool isRelative = (captureMode == eCursorCapture::PINNING);
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, MainDispatcherEvent::MOUSE_BUTTON_UP, button, x, y, 1, modifierKeys, isRelative));
        }
        else if (deviceType == PointerDeviceType::Touch)
        {
            uint32 touchId = pointerPoint->PointerId;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, MainDispatcherEvent::TOUCH_UP, touchId, x, y, modifierKeys));
        }
    }
}

void WindowNativeBridge::OnPointerCaptureLost(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    OnPointerReleased(sender, arg);
}

void WindowNativeBridge::OnPointerCancelled(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    OnPointerReleased(sender, arg);
}

void WindowNativeBridge::OnPointerMoved(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    using namespace ::Windows::UI::Input;
    using namespace ::Windows::Devices::Input;

    PointerPoint ^ pointerPoint = arg->GetCurrentPoint(nullptr);
    PointerPointProperties ^ prop = pointerPoint->Properties;
    PointerDeviceType deviceType = pointerPoint->PointerDevice->PointerDeviceType;

    eModifierKeys modifierKeys = GetModifierKeys();
    float32 x = pointerPoint->Position.X;
    float32 y = pointerPoint->Position.Y;
    if (deviceType == PointerDeviceType::Mouse)
    {
        bool pinning = captureMode == eCursorCapture::PINNING;
        if (prop->PointerUpdateKind != PointerUpdateKind::Other)
        {
            // First mouse button down (and last mouse button up) comes through OnPointerPressed/OnPointerReleased, other mouse clicks come here
            bool isPressed = false;
            eMouseButtons button = GetMouseButtonState(prop->PointerUpdateKind, &isPressed);
            MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::MOUSE_BUTTON_DOWN : MainDispatcherEvent::MOUSE_BUTTON_UP;
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(window, type, button, x, y, 1, modifierKeys, pinning));
        }
        if (!pinning)
        {
            // In pinning mouse deltas are sent in OnMouseMoved method
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, x, y, modifierKeys, false));
        }
    }
    else if (deviceType == PointerDeviceType::Touch)
    {
        uint32 touchId = pointerPoint->PointerId;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, MainDispatcherEvent::TOUCH_MOVE, touchId, x, y, modifierKeys));
    }
}

void WindowNativeBridge::OnPointerWheelChanged(::Platform::Object ^ sender, ::Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ arg)
{
    using namespace ::Windows::UI::Input;

    PointerPoint ^ pointerPoint = arg->GetCurrentPoint(nullptr);
    PointerPointProperties ^ prop = pointerPoint->Properties;

    float32 x = pointerPoint->Position.X;
    float32 y = pointerPoint->Position.Y;
    float32 deltaX = 0.f;
    float32 deltaY = static_cast<float32>(prop->MouseWheelDelta / WHEEL_DELTA);
    if (prop->IsHorizontalMouseWheel)
    {
        using std::swap;
        swap(deltaX, deltaY);
    }
    eModifierKeys modifierKeys = GetModifierKeys();
    bool isRelative = (captureMode == eCursorCapture::PINNING);
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseWheelEvent(window, x, y, deltaX, deltaY, modifierKeys, isRelative));
}

void WindowNativeBridge::OnMouseMoved(Windows::Devices::Input::MouseDevice ^ mouseDevice, ::Windows::Devices::Input::MouseEventArgs ^ args)
{
    if (mouseMoveSkipCount > 0)
    {
        // Skip some first move events to discard large deltas
        mouseMoveSkipCount--;
        return;
    }

    float32 x = static_cast<float32>(args->MouseDelta.X);
    float32 y = static_cast<float32>(args->MouseDelta.Y);
    eModifierKeys modifierKeys = GetModifierKeys();
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, x, y, modifierKeys, true));
}

void WindowNativeBridge::OnKeyboardShowing(Windows::UI::ViewManagement::InputPane ^ sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^ args)
{
    // Notify Windows that we'll handle layout by ourselves
    args->EnsuredFocusedElementInView = true;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibleFrameChangedEvent(window, 0.f, 0.f, float32(xamlSwapChainPanel->ActualWidth), float32(args->OccludedRect.Y)));
}

void WindowNativeBridge::OnKeyboardHiding(Windows::UI::ViewManagement::InputPane ^ sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^ args)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibleFrameChangedEvent(window, 0.f, 0.f, float32(xamlSwapChainPanel->ActualWidth), float32(xamlSwapChainPanel->ActualHeight)));
}

eModifierKeys WindowNativeBridge::GetModifierKeys() const
{
    using ::Windows::System::VirtualKey;
    using ::Windows::UI::Core::CoreWindow;
    using ::Windows::UI::Core::CoreVirtualKeyStates;

    eModifierKeys result = eModifierKeys::NONE;
    CoreWindow ^ coreWindow = xamlWindow->CoreWindow;

    CoreVirtualKeyStates keyState = coreWindow->GetKeyState(VirtualKey::Shift);
    if ((keyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
    {
        result |= eModifierKeys::SHIFT;
    }

    keyState = coreWindow->GetKeyState(VirtualKey::Control);
    if ((keyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
    {
        result |= eModifierKeys::CONTROL;
    }

    keyState = coreWindow->GetKeyState(VirtualKey::Menu);
    if ((keyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
    {
        result |= eModifierKeys::ALT;
    }
    return result;
}

eMouseButtons WindowNativeBridge::GetMouseButtonState(::Windows::UI::Input::PointerUpdateKind buttonUpdateKind, bool* isPressed)
{
    using ::Windows::UI::Input::PointerUpdateKind;

    *isPressed = false;
    switch (buttonUpdateKind)
    {
    case PointerUpdateKind::LeftButtonPressed:
        *isPressed = true;
    case PointerUpdateKind::LeftButtonReleased:
        return eMouseButtons::LEFT;
    case PointerUpdateKind::RightButtonPressed:
        *isPressed = true;
    case PointerUpdateKind::RightButtonReleased:
        return eMouseButtons::RIGHT;
    case PointerUpdateKind::MiddleButtonPressed:
        *isPressed = true;
    case PointerUpdateKind::MiddleButtonReleased:
        return eMouseButtons::MIDDLE;
    case PointerUpdateKind::XButton1Pressed:
        *isPressed = true;
    case PointerUpdateKind::XButton1Released:
        return eMouseButtons::EXTENDED1;
    case PointerUpdateKind::XButton2Pressed:
        *isPressed = true;
    case PointerUpdateKind::XButton2Released:
        return eMouseButtons::EXTENDED2;
    default:
        return eMouseButtons::NONE;
    }
}

void WindowNativeBridge::CreateBaseXamlUI()
{
    using ::Windows::UI::Xaml::Markup::XamlReader;
    using ::Windows::UI::Xaml::ResourceDictionary;
    using namespace ::Windows::UI::Xaml::Controls;

    xamlCanvas = ref new Canvas;

    xamlSwapChainPanel = ref new SwapChainPanel;
    xamlSwapChainPanel->Children->Append(xamlCanvas);

    {
        // Universal Windows Platfrom has some problems and bugs when application works with XAML UI:
        //  - MouseDevice::MouseMoved events are lost on Surface devices
        //  - exception is thrown when inserting some text into programmatically created TextBox
        // Solution to resolve these problems is to create invisible dummy WebView and TextBox controls
        // from XAML sheet and add them to control hierarchy
        using ::Windows::UI::Xaml::Controls::TextBox;
        WebView ^ dummyWebView = static_cast<WebView ^>(XamlReader::Load(xamlWorkaroundWebViewProblems));
        TextBox ^ dummyTextBox = static_cast<TextBox ^>(XamlReader::Load(xamlWorkaroundTextBoxProblems));

        AddXamlControl(dummyWebView);
        AddXamlControl(dummyTextBox);
    }

    // Windows UAP doesn't allow to unfocus UI control programmatically
    // It only permits to set focus at another control
    // So create dummy offscreen button that steals focus when there is
    // a need to unfocus native control, especially useful for text fields
    xamlControlThatStealsFocus = ref new Windows::UI::Xaml::Controls::Button();
    xamlControlThatStealsFocus->Content = L"I steal your focus";
    xamlControlThatStealsFocus->Width = 30;
    xamlControlThatStealsFocus->Height = 20;
    xamlControlThatStealsFocus->TabNavigation = ::Windows::UI::Xaml::Input::KeyboardNavigationMode::Cycle;
    AddXamlControl(xamlControlThatStealsFocus);
    PositionXamlControl(xamlControlThatStealsFocus, -1000.0f, -1000.0f);

    xamlWindow->Content = xamlSwapChainPanel;
}

void WindowNativeBridge::InstallEventHandlers()
{
    using namespace ::Platform;
    using namespace ::Windows::Foundation;
    using namespace ::Windows::UI::Core;
    using namespace ::Windows::UI::Xaml;
    using namespace ::Windows::UI::Xaml::Input;
    using namespace ::Windows::UI::Xaml::Controls;
    using namespace ::Windows::UI::ViewManagement;

    CoreWindow ^ coreWindow = xamlWindow->CoreWindow;

    tokenActivated = coreWindow->Activated += ref new TypedEventHandler<CoreWindow ^, WindowActivatedEventArgs ^>(this, &WindowNativeBridge::OnActivated);
    tokenVisibilityChanged = coreWindow->VisibilityChanged += ref new TypedEventHandler<CoreWindow ^, VisibilityChangedEventArgs ^>(this, &WindowNativeBridge::OnVisibilityChanged);

    tokenCharacterReceived = coreWindow->CharacterReceived += ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(this, &WindowNativeBridge::OnCharacterReceived);
    tokenAcceleratorKeyActivated = xamlWindow->Dispatcher->AcceleratorKeyActivated += ref new TypedEventHandler<CoreDispatcher ^, AcceleratorKeyEventArgs ^>(this, &WindowNativeBridge::OnAcceleratorKeyActivated);

    tokenSizeChanged = xamlSwapChainPanel->SizeChanged += ref new SizeChangedEventHandler(this, &WindowNativeBridge::OnSizeChanged);
    tokenCompositionScaleChanged = xamlSwapChainPanel->CompositionScaleChanged += ref new TypedEventHandler<SwapChainPanel ^, Object ^>(this, &WindowNativeBridge::OnCompositionScaleChanged);

    tokenPointerPressed = xamlSwapChainPanel->PointerPressed += ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerPressed);
    tokenPointerMoved = xamlSwapChainPanel->PointerMoved += ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerMoved);
    tokenPointerWheelChanged = xamlSwapChainPanel->PointerWheelChanged += ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerWheelChanged);

    tokenKeyboardShowing = InputPane::GetForCurrentView()->Showing += ref new TypedEventHandler<InputPane ^, InputPaneVisibilityEventArgs ^>(this, &WindowNativeBridge::OnKeyboardShowing);
    tokenKeyboardHiding = InputPane::GetForCurrentView()->Hiding += ref new TypedEventHandler<InputPane ^, InputPaneVisibilityEventArgs ^>(this, &WindowNativeBridge::OnKeyboardHiding);

    // We want to receive a pointer release event even if it already has been handled
    // Since there might be cases when pressed event isn't handled but released event is, even though it's the same pointer
    // In this case we still want to send TOUCH_DOWN event to make sure TOUCH_UP & TOUCH_DOWN always come in pairs

    // We also should handle PointerReleasedEvent, PointerCaptureLostEvent, PointerCanceledEvent since any of them can be sent for according PointerPressedEvent

    pointerReleasedHandler = ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerReleased);
    xamlSwapChainPanel->AddHandler(xamlSwapChainPanel->PointerReleasedEvent, pointerReleasedHandler, true);

    pointerCaptureLostHandler = ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerCaptureLost);
    xamlSwapChainPanel->AddHandler(xamlSwapChainPanel->PointerCaptureLostEvent, pointerCaptureLostHandler, true);

    pointerCancelledHandler = ref new PointerEventHandler(this, &WindowNativeBridge::OnPointerCancelled);
    xamlSwapChainPanel->AddHandler(xamlSwapChainPanel->PointerCanceledEvent, pointerCancelledHandler, true);
}

void WindowNativeBridge::UninstallEventHandlers()
{
    using ::Windows::UI::Core::CoreWindow;
    using ::Windows::Devices::Input::MouseDevice;

    using namespace ::Windows::UI::ViewManagement;

    CoreWindow ^ coreWindow = xamlWindow->CoreWindow;
    MouseDevice ^ mouseDevice = MouseDevice::GetForCurrentView();

    coreWindow->Activated -= tokenActivated;
    coreWindow->VisibilityChanged -= tokenVisibilityChanged;

    coreWindow->CharacterReceived -= tokenCharacterReceived;
    xamlWindow->Dispatcher->AcceleratorKeyActivated -= tokenAcceleratorKeyActivated;

    xamlSwapChainPanel->SizeChanged -= tokenSizeChanged;
    xamlSwapChainPanel->CompositionScaleChanged -= tokenCompositionScaleChanged;

    xamlSwapChainPanel->PointerPressed -= tokenPointerPressed;
    xamlSwapChainPanel->PointerMoved -= tokenPointerMoved;
    xamlSwapChainPanel->PointerWheelChanged -= tokenPointerWheelChanged;

    if (pointerReleasedHandler != nullptr)
    {
        xamlSwapChainPanel->RemoveHandler(xamlSwapChainPanel->PointerReleasedEvent, pointerReleasedHandler);
        pointerReleasedHandler = nullptr;
    }

    if (pointerCaptureLostHandler != nullptr)
    {
        xamlSwapChainPanel->RemoveHandler(xamlSwapChainPanel->PointerCaptureLostEvent, pointerCaptureLostHandler);
        pointerCaptureLostHandler = nullptr;
    }

    if (pointerCancelledHandler != nullptr)
    {
        xamlSwapChainPanel->RemoveHandler(xamlSwapChainPanel->PointerCanceledEvent, pointerCancelledHandler);
        pointerCancelledHandler = nullptr;
    }

    mouseDevice->MouseMoved -= tokenMouseMoved;

    InputPane::GetForCurrentView()->Showing -= tokenKeyboardShowing;
    InputPane::GetForCurrentView()->Hiding -= tokenKeyboardHiding;
}

::Platform::String ^ WindowNativeBridge::xamlWorkaroundWebViewProblems = LR"(
<WebView x:Name="dummyWebView" Visibility="Collapsed"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" 
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
</WebView>
)";

::Platform::String ^ WindowNativeBridge::xamlWorkaroundTextBoxProblems = LR"(
<TextBox x:Name="dummyTextBox" Visibility="Collapsed"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" 
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
</TextBox>
)";

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
