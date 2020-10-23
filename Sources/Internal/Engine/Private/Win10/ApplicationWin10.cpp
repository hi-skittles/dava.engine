#include "Engine/Private/Win10/ApplicationWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"

// clang-format off

namespace DAVA
{
namespace Private
{

int StartApplication(Vector<String> cmdargs)
{
    using namespace ::Windows::UI::Xaml;
    auto appStartCallback = ref new ApplicationInitializationCallback([cmdargs=std::move(cmdargs)](ApplicationInitializationCallbackParams^) {
        ref new DAVA::Private::Application(std::move(cmdargs));
    });
    Application::Start(appStartCallback);
    return 0;
}

Application::Application(Vector<String> cmdargs)
    : commandArgs(std::move(cmdargs))
{
}

void Application::OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args)
{
    OnLaunchedOrActivated(args);
}

void Application::OnActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
    OnLaunchedOrActivated(args);
}

void Application::OnFileActivated(::Windows::ApplicationModel::Activation::FileActivatedEventArgs^ args)
{
    OnLaunchedOrActivated(args);
}

void Application::OnLaunchedOrActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
    using ::Windows::ApplicationModel::Activation::ApplicationExecutionState;

    ApplicationExecutionState prevExecState = args->PreviousExecutionState;
    if (prevExecState != ApplicationExecutionState::Running && prevExecState != ApplicationExecutionState::Suspended)
    {
        // Install event handlers only if application is not running
        InstallEventHandlers();
    }

    core->OnLaunchedOrActivated(args);
}

void Application::OnWindowCreated(::Windows::UI::Xaml::WindowCreatedEventArgs^ args)
{
    if (engineBackend == nullptr)
    {
        // Create EngineBackend when application has entered thread where Universal Applications live (so called UI-thread).
        // Reason: Win10 platform implementation can access WinRT API when initializing EngineBackend (DeviceManager, etc).
        engineBackend.reset(new EngineBackend(commandArgs));
        core = engineBackend->GetPlatformCore();
        commandArgs.clear();
    }
    core->OnWindowCreated(args->Window);
}

void Application::OnSuspending(::Platform::Object^ /*sender*/, ::Windows::ApplicationModel::SuspendingEventArgs^ /*arg*/)
{
    core->OnSuspending();
}

void Application::OnResuming(::Platform::Object^ /*sender*/, ::Platform::Object^ /*arg*/)
{
    core->OnResuming();
}

void Application::OnUnhandledException(::Platform::Object^ /*sender*/, ::Windows::UI::Xaml::UnhandledExceptionEventArgs^ arg)
{
    core->OnUnhandledException(arg);
}

void Application::OnBackPressed(::Platform::Object^ /*sender*/, ::Windows::Phone::UI::Input::BackPressedEventArgs^ args)
{
    core->OnBackPressed();
    args->Handled = true;
}

void Application::OnBackRequested(::Platform::Object^ /*sender*/, ::Windows::UI::Core::BackRequestedEventArgs^ args)
{
    core->OnBackPressed();
    args->Handled = true;
}

void Application::OnGamepadAdded(::Platform::Object^ sender, ::Windows::Gaming::Input::Gamepad^ gamepad)
{
    core->OnGamepadAdded(gamepad);
}

void Application::OnGamepadRemoved(::Platform::Object^ sender, ::Windows::Gaming::Input::Gamepad^ gamepad)
{
    core->OnGamepadRemoved(gamepad);
}

void Application::OnMemoryUsageLimitChanging(::Platform::Object^/*sender*/, ::Windows::System::AppMemoryUsageLimitChangingEventArgs^ arg)
{
    core->OnMemoryUsageLimitChanging(arg);
}

void Application::OnMemoryUsageIncreased(::Platform::Object ^/*sender*/, ::Platform::Object ^/*arg*/)
{
    core->OnMemoryUsageIncreased();
}

void Application::OnDpiChanged(::Windows::Graphics::Display::DisplayInformation^ sender, ::Platform::Object^ args)
{
    core->OnDpiChanged();
}

void Application::InstallEventHandlers()
{
    using namespace ::Platform;
    using namespace ::Windows::System;
    using namespace ::Windows::Foundation;
    using namespace ::Windows::UI::Xaml;
    using namespace ::Windows::UI::Core;
    using namespace ::Windows::Phone::UI::Input;
    using namespace ::Windows::Graphics::Display;

    Suspending += ref new SuspendingEventHandler(this, &Application::OnSuspending);
    Resuming += ref new EventHandler<Object^>(this, &Application::OnResuming);
    UnhandledException += ref new UnhandledExceptionEventHandler(this, &Application::OnUnhandledException);

    SystemNavigationManager::GetForCurrentView()->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(this, &Application::OnBackRequested);
    if (PlatformCore::IsPhoneContractPresent())
    {
        HardwareButtons::BackPressed += ref new EventHandler<BackPressedEventArgs^>(this, &Application::OnBackPressed);
    }

    ::Windows::Gaming::Input::Gamepad::GamepadAdded += ref new EventHandler<::Windows::Gaming::Input::Gamepad^>(this, &Application::OnGamepadAdded);
    ::Windows::Gaming::Input::Gamepad::GamepadRemoved += ref new EventHandler<::Windows::Gaming::Input::Gamepad^>(this, &Application::OnGamepadRemoved);

    DisplayInformation^ displayInformation = DisplayInformation::GetForCurrentView();
    displayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &Application::OnDpiChanged);

    MemoryManager::AppMemoryUsageLimitChanging += ref new EventHandler<AppMemoryUsageLimitChangingEventArgs^>(this, &Application::OnMemoryUsageLimitChanging);
    MemoryManager::AppMemoryUsageIncreased += ref new EventHandler<Object^>(this, &Application::OnMemoryUsageIncreased);
}

} // namespace Private
} // namespace DAVA

// clang-format on

#endif // __DAVAENGINE_WIN_UAP__
