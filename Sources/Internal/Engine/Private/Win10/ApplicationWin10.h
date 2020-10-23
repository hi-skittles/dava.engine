#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Private/EnginePrivateFwd.h"

namespace DAVA
{
namespace Private
{
// clang-format off

ref class Application sealed : public ::Windows::UI::Xaml::Application
{
internal:
    Application(Vector<String> cmdargs);

protected:
    // ::Windows::UI::Xaml::Application overriden methods
    void OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;
    void OnActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs^ args) override;
    void OnFileActivated(::Windows::ApplicationModel::Activation::FileActivatedEventArgs^ args) override;
    void OnWindowCreated(::Windows::UI::Xaml::WindowCreatedEventArgs^ args) override;

private:
    void OnLaunchedOrActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);

    void OnSuspending(::Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ arg);
    void OnResuming(::Platform::Object^ sender, ::Platform::Object^ arg);
    void OnUnhandledException(::Platform::Object^ sender, ::Windows::UI::Xaml::UnhandledExceptionEventArgs^ arg);

    void OnBackPressed(::Platform::Object^ sender, ::Windows::Phone::UI::Input::BackPressedEventArgs^ args);
    void OnBackRequested(::Platform::Object^ sender, ::Windows::UI::Core::BackRequestedEventArgs^ args);

    void OnGamepadAdded(::Platform::Object^ sender, ::Windows::Gaming::Input::Gamepad^ gamepad);
    void OnGamepadRemoved(::Platform::Object^ sender, ::Windows::Gaming::Input::Gamepad^ gamepad);

    void OnMemoryUsageLimitChanging(::Platform::Object^ sender, ::Windows::System::AppMemoryUsageLimitChangingEventArgs^ arg);
    void OnMemoryUsageIncreased(::Platform::Object^ sender, ::Platform::Object^ arg);

    void OnDpiChanged(::Windows::Graphics::Display::DisplayInformation^ sender, ::Platform::Object^ args);

    void InstallEventHandlers();

private:
    std::unique_ptr<EngineBackend> engineBackend;
    PlatformCore* core = nullptr;
    Vector<String> commandArgs;
};

// clang-format on

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
