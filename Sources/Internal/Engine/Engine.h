#pragma once

#include "Base/BaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/EngineContext.h"
#include "Engine/Window.h"
#include "Functional/Functional.h"
#include "Render/RHI/rhi_Type.h"

/**
    \defgroup engine Engine
*/

namespace DAVA
{
class AppInstanceMonitor;
class KeyedArchive;
class Window;
namespace Private
{
class EngineBackend;
}

/**
    \ingroup engine
    Utility function to get engine context.

    Behaviour is undefined when called before `Engine` instantiated or after 'Engine::cleanup' signal emitted.
    Another but longer way to get context is to call `Engine::Instance()->GetContext()`.
*/
const EngineContext* GetEngineContext();

/**
    \ingroup engine
    Utility function to get primary window.

    Behaviour is undefined when called before `Engine` instantiated or after 'Engine::cleanup' signal has emitted.
    Return `nullptr` if called before `Engine::Init` or if `Engine` has been initialized with `eEngineRunMode::CONSOLE_MODE` mode.
    Another but longer way to get primary window is to call `Engine::Instance()->PrimaryWindow()`.
*/
Window* GetPrimaryWindow();

/**
    \ingroup engine
    Utility function to run asynchronous task on DAVA main thread.

    Behaviour is undefined when called after `Engine::cleanup` signal has emitted.
*/
void RunOnMainThreadAsync(const Function<void()>& task);

/**
    \ingroup engine
    Utility function to run task on DAVA main thread and wait its completion blocking caller thread.

    This function should not be called before `Engine::Run` is executed since it can lead to deadlock.
    Behaviour is undefined when called after `Engine::cleanup` signal has emitted.
*/
void RunOnMainThread(const Function<void()>& task);

/**
    \ingroup engine
    Utility function to run asynchronous task on UI thread belonging to primary window.

    Behaviour is undefined:
        - if Engine is initialized with console run mode.
        - if called before `Engine::Init` method which create instance of primary window.
        - if called after `Engine::windowDestroyed` signal emitted for primary window.
*/
void RunOnUIThreadAsync(const Function<void()>& task);

/**
    \ingroup engine
    Utility function to run task on UI thread belonging to primary window and wait its completion blocking caller thread.

    Behaviour is undefined:
        - if Engine is initialized with console run mode.
        - if called before `Engine::windowCreated` signal emitted for primary window.
        - if called after `Engine::windowDestroyed` signal emitted for primary window.
*/
void RunOnUIThread(const Function<void()>& task);

/**
    \ingroup engine
    Utility function for obtaining AppInstanceMonitor instance which can check whether another application instance is
    running and pass activation filename to that running instance.

    Each application should pass some arbitrary string. This string should be as unique as possible, e.g. you can generate UUID.
    \note `uniqueAppId` is not null pointer or empty string.

    This function always returns the same object not regarding how many times you have called this function. Value of
    `uniqueAppId` is taken into account only on first call.
*/
AppInstanceMonitor* GetAppInstanceMonitor(const char* uniqueAppId);

/**
    \ingroup engine
    Core component of dava.engine which manages application's control flow.
    
    Client applications and other parts of dava.engine interact with Engine in one way or another.
    For any application there is precisely one Engine object.
    
    Engine's responsibilities are:
        - It initializes low level platform dependent objects and works directly with operating system.
        - It receives events from underlying system and dispatches them to client application through signals.
        - It initializes dava.engine's modules or subsystems and provides them to application through EngineContext object.
        - It manages window creation and destruction.
        - It controls game loop.
        - It performs some other important tasks:
            - application exit,
            - getting command line
            - running arbitrary client-supplied functor in context of thread where game loop is running, etc.
    
    The Engine object is intended to be the only singleton in dava.engine and is accessible through Instance() static method.
    
    The Engine object **must** be created and initialized before any use of dava.engine's modules or subsystems. Only few subsystems
    can be used immediately after Engine creation and before initialization:
        1. Logger
        2. FileSystem
    
    C/C++ main() function is buried deep inside dava.engine and client application shall provide its entry function DAVAMain:
    \code
    int DAVAMain(DAVA::Vector<DAVA::String> cmdline);
    \endcode
    
    Engine can run game loop in several modes (see eEngineRunMode):
        - as GUI application
        - as console application
        - as GUI application embedded into other framework, now only Qt is supported
    
    The following sample shows how to use Engine:
    \code
    #include <Engine/Engine.h>
    using namespace DAVA;
    int DAVAMain(Vector<String> cmdline)
    {
        Engine engine; // Create Engine object
        eEngineRunMode runmode = eEngineRunMode::CONSOLE_MODE;
        if (cmdline.size() > 1)
        {
            // Depending on command line choose run mode
            runmode = cmdline[1] == "--console" ? eEngineRunMode::CONSOLE_MODE : runmode;
            runmode = cmdline[1] == "--gui" ? eEngineRunMode::GUI_STANDALONE : runmode;
            // To run in embedded mode application must link with Qt
            runmode = cmdline[1] == "--embedded" ? eEngineRunMode::GUI_EMBEDDED : runmode;
        }
        engine.Init(runmode, Vector<String>(), nullptr); // Initialize engine
        return engine.Run(); // Run game loop
    }
    \endcode

    Application can be finished in two ways:
    1. user or system quits application or closes primary window which leads to application exit
    2. application decides to quit and calls `Engine::QuitAsync`

    User can finish application by using system shortcuts (Alt+F4, Cmd+Q, etc) or mouse click on system button which closes window,
    or by swiping application from task manager on mobile devices.

    Generally when application exits termination signals are emitted in the following sequence:
    1. `Engine::windowDestroyed`, not emitted when application runs in console mode
    2. `Engine::gameLoopStopped`
    3. `Engine::cleanup`

    Note that all those termination signals may **not** be emitted on some platforms when user decides to quit application.
    Such a behavior is applied for systems which do not notify application about termination (mobile platforms,
    also Universal Windows Platform for desktops). In such cases signal `Engine::suspended` can be used as notification
    that `application may terminate soon`.

    Termination signal sequence specified above is guaranteed when application exits through `Engine::QuitAsync` call.
*/
class Engine final
{
public:
    /**
        Return a pointer to Engine object or null if Engine is not created yet.
    */
    static Engine* Instance();

    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /**
        Get engine context which holds all DAVA subsystems.

        Subsystems become available after `Engine::Init` call, which subsystems are available depends on
        Engine's run mode (see `eEngineRunMode`), target platform (android, win32, etc).
        The following subsystems are available immediately after Engine class creation:
            - Logger
            - FileSystem
            - DeviceManager
    */
    const EngineContext* GetContext() const;

    /**
        Return primary window if any.

        Primary window is non null except in the following cases:
            - before `Engine::Init` call,
            - Engine was initialized with `eEngineRunMode::CONSOLE_MODE` mode,
            - after receiving signal `Engine::windowDestroyed` for primary window.
    */
    Window* PrimaryWindow() const;

    /**
        Return currently existing windows, including the primary one.
    */
    const Vector<Window*>& GetWindows() const;

    eEngineRunMode GetRunMode() const;
    bool IsStandaloneGUIMode() const;
    bool IsEmbeddedGUIMode() const;
    bool IsConsoleMode() const;

    /**
        Initialize Engine object and create primary Window object (if not console mode).
        This method sets desired run mode, lists modules which should be created and assigns engine options.
        Engine object must be initialized before using dava.engine.
    
        Engine can run game loop in several modes (see eEngineRunMode enum):
            - as GUI application (eEngineRunMode::GUI_STANDALONE)
            - as console application (eEngineRunMode::CONSOLE_MODE)
            - as GUI application embedded into other framework (eEngineRunMode::GUI_EMBEDDED)
    
        Application may list dava.engine's modules (subsystems) which it wants to use during execution. The list may be empty.
        For now application may choose to create only several subsystems:
            - DownloadManager
            - JobManager
            - LocalizationSystem
            - NetCore
            - SoundSystem
            - PackManager
    
        Other modules are always created implicitly depending on specified run mode, e.g. UIScreenManager, InputSystem are not
        created in console mode.
    
        Additionally application can set options to tune dava.engine and its modules (can be null). If no options are given then dava.engine
        chooses some reasonable default value. List of options supported by dava.engine:

        | **Render options**              | Description                | Default        |
        | ------------------------------- | -------------------------- | -------------- |
        | renderer                        |                            | rhi::RHI_GLES2 |
        | rhi_threaded_frame_count        |                            | 0              |
        | max_index_buffer_count          |                            | 0              |
        | max_vertex_buffer_count         |                            | 0              |
        | max_const_buffer_count          |                            | 0              |
        | max_texture_count               |                            | 0              |
        | max_texture_set_count           |                            | 0              |
        | max_sampler_state_count         |                            | 0              |
        | max_pipeline_state_count        |                            | 0              |
        | max_depthstencil_state_count    |                            | 0              |
        | max_render_pass_count           |                            | 0              |
        | max_command_buffer_count        |                            | 0              |
        | max_packet_list_count           |                            | 0              |
        | shader_const_buffer_size        |                            | 0              |

        For more info on render options ask RHI guys.
    
        Other options can be found in description for corresponding module.
    */
    void Init(eEngineRunMode runMode, const Vector<String>& modules, KeyedArchive* options);

    /**
        Run game loop, here application life begins.
        After entering Run() application starts receiving life-cycle signals.
        Some platforms (iOS, macOS) may not return from Run(). Application should subscribe
        to cleanup signal as last chance to free up resources and get ready for termination.

        \return Exit code value passed to Quit() or 0 by default.
    */
    int Run();

    /**
        Request to quit application with given exit code.

        Application should use traditional exit code values: zero for success, positive values for failure.
        Performed asynchronously, dava.engine closes all windows then emits signals `gameLoopStopped` and `cleanup`.

        \note Not all platforms allow to specify exit code, but leave this ability for symmetry.
    */
    void QuitAsync(int exitCode);

    /**
        Immediately terminate application with given exit code.

        Application is immediately terminated bypassing normal exiting sequence:
            - memory and resources are not freed,
            - destructors of global objects are not called,
            - destructors of automatic objects that exist at the time of the call are not called,
            - signals such as `gameLoopStopped`, `cleanup` and others are not emitted.

        \note Termination is an unusual way to exit application.
    */
    void Terminate(int exitCode = EXIT_FAILURE);

    /**
        Set handler which is invoked when user is trying to close window or application.
        
        Handler can prevent window/application closing by returning false. This ability is
        supported only on desktop platforms: win32 and macOS.
        Typical usage is to return false in handler to prevent immediate window/app closing
        and show dialog asking user whether he wants to close window/app. If he chooses to
        close window/app then application should call Window::Close or Engine::Quit.
        Handler is only invoked if window/app is closing by user request: by pressing Alt+F4 or
        by clicking mouse on window close button or by pressing Cmd+Q on macOS.
        
        \param handler Function object with prototype
                       \code
                            bool handler(Window* window);
                       \endcode
                       If window parameter is null then user is trying to close whole application,
                       otherwise user is trying to close specified window.
    */
    void SetCloseRequestHandler(const Function<bool(Window*)>& handler);

    const KeyedArchive* GetOptions() const;

    /**
        Get current frame index (you can consider this index as number of updates invoked).
    */
    uint32 GetGlobalFrameIndex() const;

    /**
        Get parsed command line args, now it is the same as command line passed to DAVAMain function.
        First command line argument is always application name (on android it is app_process).
    */
    const Vector<String>& GetCommandLine() const;

    /**
        Get command line args in form suitable to use as argc and argv.

        Some platforms want command line in form argc and argv, e.g. iOS's UIApplicationMain, Qt's QApplication, etc.
        Engine grabs command line at program start and stores it as DAVA::Vector of DAVA::String, so GetCommandLineAsArgv allows
        passing command line arguments to those functions.

        How to use:
        \code{.cpp}
            Vector<char*> argv = engineBackend->GetCommandLineAsArgv();
            int n = static_cast<int>(argv.size());
            ::UIApplicationMain(n, &*argv.begin(), @"principal", @"delegate");
        \endcode
    */
    Vector<char*> GetCommandLineAsArgv() const;

    /**
        Return value indicating if an app is in suspended state.
        You can also use `suspended` and `resumed` signals to keep track of this value.

        \note Only these platforms support suspending: Win10, iOS, Android.
    */
    bool IsSuspended() const;

    /**
        Set value indicating if screen timeout is enabled.
        If value is `true`, screens will dim some time after last user interaction (if it's enabled in system settings),
        otherwise screens will stay active even if there's no input.
        By default, timeout is enabled.
    */
    void SetScreenTimeoutEnabled(bool enabled);

    /**
        Get list of filenames application was started with.
        Application can retrieve that list only in `Engine::gameLoopStarted` signal handler. If you want to get filenames
        later then save the list.
    */
    Vector<String> GetStartupActivationFilenames() const;

public:
    Signal<> registerUserTypes; //!< Emitted just before initializing subsystems, place here user types/reflections/components/systems registration. Note: should be connected before Engine::Init(...) call.
    Signal<> gameLoopStarted; //!< Emitted just before entering game loop. Note: native windows are not created yet and renderer is not initialized.
    Signal<> gameLoopStopped; //!< Emitted after exiting game loop, application should prepare to terminate.
    Signal<> cleanup; //!< Last signal emitted by Engine, after this signal dava.engine is dead.
    Signal<Window*> windowCreated; //!< Emitted when native window is created and renderer is initialized.
    Signal<Window*> windowDestroyed; //!< Emitted just before native window is destroyed. After this signal no one should use window.
    Signal<> beginFrame; //!< Emitted at the beginning of frame when application is in foreground.
    Signal<float32> update; //!< Emitted on each frame when application is in foreground (not suspended). Note: rendering should be performed on `Window::update`, `Window::draw` signals.
    Signal<> endFrame; //!< Emitted at the end of frame when application is in foreground.
    Signal<float32> backgroundUpdate; //!< Emitted on each frame when application is suspended.
    Signal<> suspended; //!< Emitted when application has entered suspended state. This signal is fired only on platforms
    //!< that support suspending: Win10, iOS, Android. Rendering is stopped but `backgroundUpdate` signal is emitted if system permits.
    Signal<> resumed; //!< Emitted when application exits suspended state.

    Signal<rhi::RenderingError> renderingError; //!< Emitted when rendering is not possible anymore, can be invoked from any thread.
    //!< Application should be gracefully closed (with optional message to user depending on error value)
    //!< Ignoring this signal or continuing work after it may lead to undefined behaviour

    Signal<> lowMemory; //!< Emitted when OS running low on memory and corresponding event received by engine. (works on Android, iOS, Win64 and UWP on some devices).

    Signal<Vector<String>> fileActivated; //!< Emitted when already running application was activated with given filenames.

private:
    Private::EngineBackend* engineBackend = nullptr;

    // Friends
    friend class Private::EngineBackend;
};

} // namespace DAVA
