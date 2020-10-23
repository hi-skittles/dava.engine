#pragma once

#include <Base/BaseTypes.h>

#include <memory>

namespace DAVA
{
namespace Private
{
class AppInstanceMonitorImpl;
class EngineBackend;
}

/**
    \ingroup engine

    AppInstanceMonitor class is used to check whether another application instance is running and pass activation filename
    to that instance. Running instance knows about activation filename through `Engine::fileActivated` signal.

    On most platforms (android, ios, win10 and mac with some limitations) operating system limits application to a single instance.
    When user tries to run second instance or opens file associated with the application system brings running instance of
    application to front and can pass to it activation filename.

    In Windows world things are more difficult. Application should worry itself about single instancing.
    On Windows file associations work in the following way: when user double-clicks on file shell searches registry for record corresponding
    to that file extension (registry record is usually in the form `app.exe %1` where `%1` is substituted with real filename, there can be more
    command line switches, like `app.exe --file %1`), then if registry record is found shell starts new application instance passing filename
    to it through command line. Application, if it wants to be single-instanced, should do the following:
        - detect if another instance is running, if it is the first instance run and handle file, if any,
        - else application should somehow pass filename, if any, to the first instance and quit.
    All that boring work can be done by AppInstanceMonitor class.

    Instance of AppInstanceMonitor class is retrieved by `GetAppInstanceMonitor` function call. Application should check for another
    running instance as early as possible in DAVAMain function, even before instantiation of `Engine` class. If another application
    instance is detected this instance \b should quit else behavior is undefined. Additionally second instance can pass activation filename
    to the first instance.

    <b>Some words about macOS.</b> If user tries to start second instance of application, either by running `program.app` in Finder or double-clicking
    on file associated with the program, system does not start second instance but brings to front application and its window. Though it is
    possible to run another instance of `program.app` starting `program.app/Contents/MacOS/program` executable in terminal. But such case is ignored
    for now.
    \todo Make macOS application truly single-instanced.

    <b>Example of using AppInstanceMonitor to make application single-instance and pass activation filenames:</b>
    \code
    int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
    {
        // Get instance of AppInstanceMonitor object
        AppInstanceMonitor* appInstanceMonitor = GetAppInstanceMonitor("some unique string");
        // Check whether another instance is running
        if (appInstanceMonitor->IsAnotherInstanceRunning())
        {
            // This is second instance, so pass activation filename to the first instance and quit
            appInstanceMonitor->PassActivationFilename("file.txt");
            return 0;
        }

        // This is first instance, so init and run application
        Engine e;
        e.Init(...);
        return e.Run();
    }
    \endcode
*/
class AppInstanceMonitor final
{
public:
    /**
        Check whether another instance is running.

        \note On all platforms except win32 always returns \c false.
    */
    bool IsAnotherInstanceRunning() const;

    /**
        Pass activation filename to another or this instance, do nothing if `filename` is an empty string.

        Usually you call this method after parsing command line and detecting that it has filename to open.
        You can call this method on any platform though all platforms except win32 has theirs own means to
        run application activated with file.

        Activation filename later can be obtained either through `Engine::GetStartupActivationFilenames` in this instance
        or through `Engine::fileActivated` signal in another instance.
    */
    void PassActivationFilename(const String& filename);

private:
    // Constructor and destructor are intentionally private to disallow user instantiating this class
    AppInstanceMonitor(const char* uniqueAppId);
    ~AppInstanceMonitor();

    void ActivatedWithFilenameFromAnotherInstance(String filename);

    Private::EngineBackend* engineBackend = nullptr;
    std::unique_ptr<Private::AppInstanceMonitorImpl> impl;

    friend class Private::AppInstanceMonitorImpl;
    friend class Private::EngineBackend;
};

} // namespace DAVA
