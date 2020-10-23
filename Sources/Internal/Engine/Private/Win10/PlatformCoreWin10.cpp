#include "Engine/Private/Win10/PlatformCoreWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Base/Exception.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiWin10.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"
#include "Engine/Private/Win10/DllImportWin10.h"
#include "Engine/Private/Win10/WindowImplWin10.h"

#include "Concurrency/LockGuard.h"
#include "Concurrency/Thread.h"
#include "Debug/Backtrace.h"
#include "Logger/Logger.h"
#include "Platform/DeviceInfo.h"
#include "Time/SystemTimer.h"
#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"

extern int DAVAMain(DAVA::Vector<DAVA::String> cmdline);

namespace DAVA
{
namespace Private
{
bool PlatformCore::isPhoneContractPresent = false;
::Windows::UI::Core::CoreDispatcher ^ PlatformCore::coreDispatcher = nullptr;

PlatformCore::PlatformCore(EngineBackend* engineBackend_)
    : engineBackend(engineBackend_)
    , dispatcher(engineBackend->GetDispatcher())
{
    using ::Windows::Foundation::Metadata::ApiInformation;
    isPhoneContractPresent = ApiInformation::IsApiContractPresent("Windows.Phone.PhoneContract", 1);

    DllImport::Initialize();
}

PlatformCore::~PlatformCore() = default;

void PlatformCore::Init()
{
}

void PlatformCore::Run()
{
    if (savedActivatedEventArgs != nullptr)
    {
        // Here notify listeners about OnLaunched or OnActivated occured before entering game loop.
        // Notification will come on first frame
        engineBackend->GetPrimaryWindow()->RunOnUIThreadAsync([ this, args = savedActivatedEventArgs ]() {
            using ::Windows::ApplicationModel::Activation::ActivationKind;
            NotifyListeners(args->Kind == ActivationKind::Launch ? ON_LAUNCHED : ON_ACTIVATED, args);
        });
        savedActivatedEventArgs = nullptr;
    }

    engineBackend->OnGameLoopStarted();

    if (appPrelaunched)
    {
        Logger::Info("Application is PrelaunchActivated");
    }

    while (!quitGameThread)
    {
        int64 frameBeginTime = SystemTimer::GetMs();

        int32 fps = engineBackend->OnFrame();

        int64 frameEndTime = SystemTimer::GetMs();
        int32 frameDuration = static_cast<int32>(frameEndTime - frameBeginTime);

        int32 sleep = 1;
        if (fps > 0)
        {
            sleep = 1000 / fps - frameDuration;
            if (sleep < 1)
                sleep = 1;
        }
        ::Sleep(sleep);
    }

    engineBackend->OnGameLoopStopped();
    engineBackend->OnEngineCleanup();
}

void PlatformCore::PrepareToQuit()
{
    engineBackend->PostAppTerminate(true);
}

void PlatformCore::Quit()
{
    quitGameThread = true;
}

void PlatformCore::SetScreenTimeoutEnabled(bool enabled)
{
    engineBackend->GetPrimaryWindow()->RunOnUIThreadAsync([this, enabled]() {
        if (displayRequestActive != enabled)
        {
            return;
        }

        if (enabled)
        {
            displayRequest->RequestRelease();
            displayRequestActive = false;
        }
        else
        {
            displayRequest->RequestActive();
            displayRequestActive = true;
        }
    });
}

void PlatformCore::OnLaunchedOrActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args)
{
    using ::Windows::UI::Core::CoreDispatcher;
    using ::Windows::UI::Core::CoreWindow;
    using namespace ::Windows::ApplicationModel::Activation;

    // Force DeviceInfo instantiation for early initialization (due to static nature of DeviceInfo)
    Logger::FrameworkDebug("%s", DeviceInfo::GetPlatformString().c_str());

    if (coreDispatcher == nullptr)
    {
        coreDispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
    }

    if (args->Kind == ActivationKind::Launch)
    {
        LaunchActivatedEventArgs ^ launchArgs = static_cast<LaunchActivatedEventArgs ^>(args);
        appPrelaunched = launchArgs->PrelaunchActivated;
    }

    ApplicationExecutionState prevExecState = args->PreviousExecutionState;
    if (prevExecState != ApplicationExecutionState::Running && prevExecState != ApplicationExecutionState::Suspended)
    {
        if (args->Kind == ActivationKind::File)
        {
            // Main thread is not running yet so we can safely collect filenames without any synchronization
            CollectActivationFilenames(static_cast<FileActivatedEventArgs ^>(args));
        }

        Thread* gameThread = Thread::Create(MakeFunction(this, &PlatformCore::GameThread));
        gameThread->Start();
        gameThread->BindToProcessor(0);
        gameThread->SetPriority(Thread::PRIORITY_HIGH);
        // TODO: make Thread detachable
        //gameThread->Detach();
        //gameThread->Release();

        // Save activated event arguments to notify listeners later just before entering game loop to ensure
        // that dava.engine and game have intialized and listeners have had chance to register.
        savedActivatedEventArgs = args;
    }
    else
    {
        if (args->Kind == ActivationKind::File)
        {
            RunOnMainThreadAsync([ this, args = static_cast<FileActivatedEventArgs ^>(args) ]() {
                CollectActivationFilenames(args);
                engineBackend->OnFileActivated();
            });
        }

        NotifyListeners(args->Kind == ActivationKind::Launch ? ON_LAUNCHED : ON_ACTIVATED, args);
    }
}

void PlatformCore::OnWindowCreated(::Windows::UI::Xaml::Window ^ xamlWindow)
{
    // TODO: think about binding XAML window to prior created Window instance
    Window* primaryWindow = engineBackend->GetPrimaryWindow();
    if (primaryWindow == nullptr)
    {
        primaryWindow = engineBackend->InitializePrimaryWindow();
    }
    WindowImpl* windowImpl = EngineBackend::GetWindowImpl(primaryWindow);
    windowImpl->BindXamlWindow(xamlWindow);
}

void PlatformCore::OnSuspending()
{
    NotifyListeners(ON_SUSPENDING, nullptr);
    dispatcher->SendEvent(MainDispatcherEvent(MainDispatcherEvent::APP_SUSPENDED)); // Blocking call !!!
}

void PlatformCore::OnResuming()
{
    dispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::APP_RESUMED));
}

void PlatformCore::OnUnhandledException(::Windows::UI::Xaml::UnhandledExceptionEventArgs ^ arg)
{
    Logger::Error("Unhandled exception: hresult=0x%08X, message=%s", arg->Exception, UTF8Utils::EncodeToUTF8(arg->Message->Data()).c_str());
}

void PlatformCore::OnBackPressed()
{
    dispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::BACK_NAVIGATION));
}

void PlatformCore::OnGamepadAdded(::Windows::Gaming::Input::Gamepad ^ /*gamepad*/)
{
    dispatcher->PostEvent(MainDispatcherEvent::CreateGamepadAddedEvent(0));
}

void PlatformCore::OnGamepadRemoved(::Windows::Gaming::Input::Gamepad ^ /*gamepad*/)
{
    dispatcher->PostEvent(MainDispatcherEvent::CreateGamepadRemovedEvent(0));
}

void PlatformCore::OnMemoryUsageLimitChanging(::Windows::System::AppMemoryUsageLimitChangingEventArgs ^ arg)
{
    using namespace ::Windows::System;

    // https://github.com/Microsoft/Windows-universal-samples/blob/master/Samples/BackgroundMediaPlayback/README.md
    if (MemoryManager::AppMemoryUsage > arg->NewLimit)
    {
        Logger::PlatformLog(Logger::LEVEL_INFO, Format("Memory usage [%ull] is higher than limit [%ull].", MemoryManager::AppMemoryUsage, arg->NewLimit).c_str());
        dispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::LOW_MEMORY));
    }
}

void PlatformCore::OnMemoryUsageIncreased()
{
    using namespace ::Windows::System;

    if (MemoryManager::AppMemoryUsageLevel >= AppMemoryUsageLevel::High)
    {
        Logger::PlatformLog(Logger::LEVEL_INFO, "Memory usage level is high.");
        dispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::LOW_MEMORY));
    }
}

void PlatformCore::OnDpiChanged()
{
    engineBackend->UpdateDisplayConfig();
}

void PlatformCore::RegisterXamlApplicationListener(PlatformApi::Win10::XamlApplicationListener* listener)
{
    DVASSERT(listener != nullptr);

    using std::begin;
    using std::end;

    LockGuard<Mutex> lock(listenersMutex);
    auto it = std::find(begin(xamlApplicationListeners), end(xamlApplicationListeners), listener);
    if (it == end(xamlApplicationListeners))
    {
        xamlApplicationListeners.push_back(listener);
    }
}

void PlatformCore::UnregisterXamlApplicationListener(PlatformApi::Win10::XamlApplicationListener* listener)
{
    using std::begin;
    using std::end;

    LockGuard<Mutex> lock(listenersMutex);
    auto it = std::find(begin(xamlApplicationListeners), end(xamlApplicationListeners), listener);
    if (it != end(xamlApplicationListeners))
    {
        xamlApplicationListeners.erase(it);
    }
}

void PlatformCore::GameThread()
{
    try
    {
        DAVAMain(engineBackend->GetCommandLine());
    }
    catch (const Exception& e)
    {
        StringStream ss;
        ss << "!!! Unhandled DAVA::Exception \"" << e.what() << "\" at `" << e.file << "`: " << e.line << std::endl;
        ss << Debug::GetBacktraceString(e.callstack) << std::endl;
        Logger::PlatformLog(Logger::LEVEL_ERROR, ss.str().c_str());
        throw;
    }

    using namespace ::Windows::UI::Xaml;
    Application::Current->Exit();
}

void PlatformCore::NotifyListeners(eNotificationType type, ::Platform::Object ^ arg1)
{
    using ::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs;
    using ::Windows::ApplicationModel::Activation::IActivatedEventArgs;

    Vector<PlatformApi::Win10::XamlApplicationListener*> listenersCopy;
    {
        // Make copy to allow listeners unregistering inside a callback
        LockGuard<Mutex> lock(listenersMutex);
        listenersCopy.resize(xamlApplicationListeners.size());
        std::copy(xamlApplicationListeners.begin(), xamlApplicationListeners.end(), listenersCopy.begin());
    }
    for (PlatformApi::Win10::XamlApplicationListener* l : listenersCopy)
    {
        switch (type)
        {
        case ON_LAUNCHED:
            l->OnLaunched(static_cast<LaunchActivatedEventArgs ^>(arg1));
            break;
        case ON_ACTIVATED:
            l->OnActivated(static_cast<IActivatedEventArgs ^>(arg1));
            break;
        case ON_SUSPENDING:
            l->OnSuspending();
            break;
        default:
            break;
        }
    }
}

void PlatformCore::CollectActivationFilenames(::Windows::ApplicationModel::Activation::FileActivatedEventArgs ^ args)
{
    using ::Windows::ApplicationModel::Activation::FileActivatedEventArgs;
    using ::Windows::Storage::StorageFile;

    auto files = args->Files;
    const unsigned int nfiles = files->Size;
    for (unsigned int i = 0; i < nfiles; ++i)
    {
        StorageFile ^ file = static_cast<StorageFile ^>(files->GetAt(i));
        engineBackend->AddActivationFilename(UTF8Utils::EncodeToUTF8(file->Path->Data()));
    }
}

void PlatformCore::EnableHighResolutionTimer(bool enable)
{
    if (DllImport::fnTimeGetDevCaps != nullptr)
    {
        static UINT minTimerPeriod = 0;
        static bool highResolutionEnabled = false;

        if (minTimerPeriod == 0)
        {
            // On first call obtain timer capabilities
            TIMECAPS timeCaps;
            if (DllImport::fnTimeGetDevCaps(&timeCaps, sizeof(TIMECAPS)) == 0)
            {
                minTimerPeriod = timeCaps.wPeriodMin;
            }
        }

        // Application must match each call to timeBeginPeriod with a call to timeEndPeriod
        // https://msdn.microsoft.com/en-us/library/dd757633(v=vs.85).aspx
        if (minTimerPeriod != 0 && highResolutionEnabled != enable)
        {
            if (enable)
            {
                DllImport::fnTimeBeginPeriod(minTimerPeriod);
            }
            else
            {
                DllImport::fnTimeEndPeriod(minTimerPeriod);
            }
            highResolutionEnabled = enable;
        }
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN_UAP__
