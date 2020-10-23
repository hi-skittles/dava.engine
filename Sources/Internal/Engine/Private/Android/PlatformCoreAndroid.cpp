#include "Engine/Private/Android/PlatformCoreAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Base/Exception.h"
#include "Engine/Window.h"
#include "Engine/PlatformApiAndroid.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"
#include "Engine/Private/Android/AndroidBridge.h"
#include "Engine/Private/Android/WindowImplAndroid.h"

#include "Debug/Backtrace.h"
#include "Input/InputSystem.h"
#include "Logger/Logger.h"
#include "Time/SystemTimer.h"

extern int DAVAMain(DAVA::Vector<DAVA::String> cmdline);
extern DAVA::Private::AndroidBridge* androidBridge;

extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_DavaGamepadManager_nativeOnGamepadAdded(JNIEnv* env, jclass jclazz, jint deviceId, jstring name, jboolean hasTriggerButtons)
{
    using namespace DAVA;
    String deviceName = JNI::JavaStringToString(name, env);
    androidBridge->core->OnGamepadAdded(deviceId, deviceName, hasTriggerButtons == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaGamepadManager_nativeOnGamepadRemoved(JNIEnv* env, jclass jclazz, jint deviceId)
{
    androidBridge->core->OnGamepadRemoved(deviceId);
}

} // extern "C"

namespace DAVA
{
namespace Private
{
PlatformCore::PlatformCore(EngineBackend* engineBackend)
    : engineBackend(engineBackend)
    , mainDispatcher(engineBackend->GetDispatcher())
{
    AndroidBridge::AttachPlatformCore(this);
}

PlatformCore::~PlatformCore() = default;

void PlatformCore::Init()
{
}

void PlatformCore::Run()
{
    // Minimum JNI local references count that can be created during one frame.
    // From docs: VM automatically ensures that at least 16 local references can be created
    static const jint JniLocalRefsMinCount = 16;

    engineBackend->OnGameLoopStarted();
    // OnGameLoopStarted can take some amount of time so hide spash view after game has done
    // its work to not frighten user with black screen.
    AndroidBridge::HideSplashView();
    AndroidBridge::NotifyEngineRunning();

    JNIEnv* env = AndroidBridge::GetEnv();
    while (!quitGameThread)
    {
        int64 frameBeginTime = SystemTimer::GetMs();

        // We want to automatically clear all JNI local references created by user
        // on current frame. This should be done to protect our main-loop from
        // potential IndirectReferenceTable overflow when the user forgot to delete
        // its local reference.
        //
        // Note, engine user is still responsible for freeing local references created by him.
        env->PushLocalFrame(JniLocalRefsMinCount);

        // Now engine frame can be executed
        int32 fps = engineBackend->OnFrame();

        // Pop off the current local reference frame and free references.
        env->PopLocalFrame(nullptr);

        int64 frameEndTime = SystemTimer::GetMs();
        int32 frameDuration = static_cast<int32>(frameEndTime - frameBeginTime);
        int32 sleep = 1;
        if (fps > 0)
        {
            sleep = 1000 / fps - frameDuration;
            if (sleep < 1)
                sleep = 1;
        }
        Thread::Sleep(sleep);
    }

    engineBackend->OnGameLoopStopped();
    engineBackend->OnEngineCleanup();
}

void PlatformCore::PrepareToQuit()
{
    AndroidBridge::PostQuitToActivity();
}

void PlatformCore::Quit()
{
    quitGameThread = true;
}

void PlatformCore::SetScreenTimeoutEnabled(bool enabled)
{
    androidBridge->SetScreenTimeoutEnabled(enabled);
}

WindowImpl* PlatformCore::ActivityOnCreate()
{
    Window* primaryWindow = engineBackend->InitializePrimaryWindow();
    WindowImpl* primaryWindowImpl = EngineBackend::GetWindowImpl(primaryWindow);
    return primaryWindowImpl;
}

void PlatformCore::ActivityOnFileIntent(String filename, bool onStartup)
{
    if (onStartup)
    {
        // Main thread is not running yet so we can safely add filenames without any synchronization
        engineBackend->AddActivationFilename(std::move(filename));
    }
    else
    {
        RunOnMainThreadAsync([ this, filename = std::move(filename) ]() mutable {
            engineBackend->AddActivationFilename(std::move(filename));
            engineBackend->OnFileActivated();
        });
    }
}

void PlatformCore::ActivityOnResume()
{
    if (goBackgroundTimeRelativeToBoot > 0)
    {
        int64 timeSpentInBackground1 = SystemTimer::GetSystemUptimeUs() - goBackgroundTimeRelativeToBoot;
        int64 timeSpentInBackground2 = SystemTimer::GetUs() - goBackgroundTime;

        Logger::Debug("Time spent in background %lld us (reported by SystemTimer %lld us)", timeSpentInBackground1, timeSpentInBackground2);
        // Do adjustment only if SystemTimer has stopped ticking
        if (timeSpentInBackground1 - timeSpentInBackground2 > 500000l)
        {
            EngineBackend::AdjustSystemTimer(timeSpentInBackground1 - timeSpentInBackground2);
        }
    }

    mainDispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::APP_RESUMED));
}

void PlatformCore::ActivityOnPause()
{
    // Blocking call !!!
    mainDispatcher->SendEvent(MainDispatcherEvent(MainDispatcherEvent::APP_SUSPENDED));

    goBackgroundTimeRelativeToBoot = SystemTimer::GetSystemUptimeUs();
    goBackgroundTime = SystemTimer::GetUs();
}

void PlatformCore::ActivityOnDestroy()
{
    // Dispatch application termination request initiated by system, i.e. android activity is finishing
    // Do nonblocking call as Java part will wait until native thread is finished
    engineBackend->PostAppTerminate(true);
}

void PlatformCore::ActivityOnTrimMemory(int32 level)
{
    Logger::PlatformLog(Logger::LEVEL_INFO, Format("Trim memory lvl [%i].", level).c_str());

    // https://developer.android.com/reference/android/content/ComponentCallbacks2.html
    enum TrimLevel
    {
        TRIM_MEMORY_BACKGROUND = 40, // the process has gone on to the LRU list.
        TRIM_MEMORY_COMPLETE = 80, // the process is nearing the end of the background LRU list, and if more memory isn't found soon it will be killed.
        TRIM_MEMORY_MODERATE = 60, // the process is around the middle of the background LRU list; freeing memory can help the system keep other processes running later in the list for better overall performance.
        TRIM_MEMORY_RUNNING_CRITICAL = 15, // the process is not an expendable background process, but the device is running extremely low on memory and is about to not be able to keep any background processes running.
        TRIM_MEMORY_RUNNING_LOW = 10, // the process is not an expendable background process, but the device is running low on memory.
        TRIM_MEMORY_RUNNING_MODERATE = 5, // the process is not an expendable background process, but the device is running moderately low on memory.
        TRIM_MEMORY_UI_HIDDEN = 20, // the process had been showing a user interface, and is no longer doing so.
    };

    if ((level >= TRIM_MEMORY_RUNNING_MODERATE && level <= TRIM_MEMORY_RUNNING_CRITICAL) || level == TRIM_MEMORY_COMPLETE)
    {
        mainDispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::LOW_MEMORY));
    }
}

void PlatformCore::GameThread()
{
    try
    {
        DAVAMain(std::move(androidBridge->cmdargs));
    }
    catch (const Exception& e)
    {
        StringStream ss;
        ss << "!!! Unhandled DAVA::Exception \"" << e.what() << "\" at `" << e.file << "`: " << e.line << std::endl;
        ss << Debug::GetBacktraceString(e.callstack) << std::endl;
        Logger::PlatformLog(Logger::LEVEL_ERROR, ss.str().c_str());
        throw;
    }
}

void PlatformCore::OnGamepadAdded(int32 deviceId, const String& name, bool hasTriggerButtons)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadAddedEvent(deviceId));
}

void PlatformCore::OnGamepadRemoved(int32 deviceId)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadRemovedEvent(deviceId));
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
