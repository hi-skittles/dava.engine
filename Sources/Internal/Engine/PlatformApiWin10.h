#pragma once

#if defined(__DAVAENGINE_WIN_UAP__)

/**
    \defgroup engine_win10 Engine facilities specific to Windows Universal Platform (Win10)
*/

namespace DAVA
{
class Window;
namespace PlatformApi
{
namespace Win10
{
/**
    \ingroup engine_win10
    Interface definition for a callbacks to be invoked when `Windows::UI::Xaml::Application` lifecycle event occurs (OnLaunched, etc).
    Only subset of `Windows::UI::Xaml::Application` methods are mapped to the interface definition, other methods are mapped as required.

    To receive callbacks application should declare class derived from `XamlApplicationListener`, implement necessary methods
    and register it through `RegisterXamlApplicationListener` function.

    Methods of `XamlApplicationListener` are always called in the context of primary UI thread (thread where first window is created).
*/
struct XamlApplicationListener
{
    // clang-format off
    virtual ~XamlApplicationListener() = default;
    virtual void OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args) {}
    /**
        Invoked when the application is activated by some means other than normal launching.
        You can cast `args` to specific type depending on `IActivatedEventArgs::Kind`, e.g.
        if `IActivatedEventArgs::Kind` is `ActivationKind::File` then you can safely cast `args` to
        pointer to `FileActivatedEventArgs`.
    */
    virtual void OnActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs^ args) {}
    virtual void OnSuspending() {}
    // clang-format on
};

void AddXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl);
void RemoveXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl);
void PositionXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl, float32 x, float32 y);
void UnfocusXamlControl(Window* targetWindow, ::Windows::UI::Xaml::UIElement ^ xamlControl);
::Windows::UI::Xaml::Input::Pointer ^ GetLastPressedPointer(Window* targetWindow);

/**
    \ingroup engine_win10
    Register a callback to be invoked in response of `Windows::UI::Xaml::Application` lifecycle events.

    Application can register a callback from any thread, but callbacks are invoked in the context of primary UI thread.
    The best place to call this function is before calling `Engine::Run` or in `Engine::gameLoopStarted` signal handler.

    \pre `listener` should not be null pointer
    \pre Function shall not be called before `Engine::Init` or after `Engine::cleanup` signal.
*/
void RegisterXamlApplicationListener(XamlApplicationListener* listener);

/**
    \ingroup engine_win10
    Unregister a callback previously registered by `RegisterXamlApplicationListener` function.

    Application can unregister a callback from any thread, even during callback invocation.

    \pre `listener` should be previously registered
    \pre Function shall not be called after `Engine::cleanup` signal
*/
void UnregisterXamlApplicationListener(XamlApplicationListener* listener);

} // namespace Win10
} // namespace PlatformApi
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
