#include "Base/BaseTypes.h"

#include "Base/Exception.h"
#include "Base/Platform.h"
#include "Debug/Backtrace.h"
#include "Engine/Private/CommandArgs.h"
#include "Engine/Private/EngineBackend.h"
#include "Logger/Logger.h"

/**
    \ingroup engine
    Entry point of program which uses dava.engine. An application shall implement this global function which designates
    start of program.

    This function is called after primary initialization of engine infrastructure. Thread which executes DAVAMain is considered DAVA main thread.
    DAVAMain takes parsed command line arguments as passed from operating system. Command line always contains at least one argument - program name 
    (on android program name is always app_process).

    Return value of DAVAMain is used as process exit code if underlying operating system supports such functionality.

    Minimalistic program that uses dava.engine:
    \code
    #include <Engine/Engine.h>

    int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
    {
        // Create Engine object
        DAVA::Engine engine; 

        // Initialize engine
        engine.Init(DAVA::eEngineRunMode::GUI_STANDALONE, DAVA::Vector<String>(), nullptr); 

        // Run game loop
        return engine.Run(); 
    }
    \endcode
*/
extern int DAVAMain(DAVA::Vector<DAVA::String> cmdline);

// clang-format off

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

// Defined in EntryApple.mm since it requies obj-c capabilities

#elif defined(__DAVAENGINE_LINUX__) || defined(__DAVAENGINE_QT__ ) || (defined(__DAVAENGINE_WIN32__) && defined(CONSOLE))

int main(int argc, char* argv[])
{
    using namespace DAVA;
    using DAVA::Private::EngineBackend;

    try {
        Vector<String> cmdargs = Private::GetCommandArgs(argc, argv);
        std::unique_ptr<EngineBackend> engineBackend(new EngineBackend(cmdargs));
        return DAVAMain(std::move(cmdargs));
    } catch (const Exception& e) {
        StringStream ss;
        ss << "!!! Unhandled DAVA::Exception \"" << e.what() << "\" at `" << e.file << "`:" << e.line << std::endl;
        ss << Debug::GetBacktraceString(e.callstack) << std::endl;
        Logger::PlatformLog(Logger::LEVEL_ERROR, ss.str().c_str());
        throw;
    }
}

#elif defined(__DAVAENGINE_WIN32__)

// Win32
// To use WinMain in static lib with unicode support set entry point to wWinMainCRTStartup:
//  1. through linker commandline option /ENTRY:wWinMainCRTStartup
//  2. property panel Linker -> Advanced -> Entry Point
//  3. cmake script - set_target_properties(target PROPERTIES LINK_FLAGS "/ENTRY:wWinMainCRTStartup")
// https://msdn.microsoft.com/en-us/library/dybsewaf.aspx
// https://support.microsoft.com/en-us/kb/125750
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    using namespace DAVA;
    using Private::EngineBackend;

    try {
        Vector<String> cmdargs = Private::GetCommandArgs();
        std::unique_ptr<EngineBackend> engineBackend(new EngineBackend(cmdargs));
        return DAVAMain(std::move(cmdargs));
    } catch (const Exception& e) {
        StringStream ss;
        ss << "!!! Unhandled DAVA::Exception \"" << e.what() << "\" at `" << e.file << "`: " << e.line << std::endl;
        ss << Debug::GetBacktraceString(e.callstack) << std::endl;
        Logger::PlatformLog(Logger::LEVEL_ERROR, ss.str().c_str());
        throw;
    }
}

#elif defined(__DAVAENGINE_WIN_UAP__)

namespace DAVA
{
namespace Private
{
extern int StartApplication(Vector<String> cmdargs);
} // namespace Private
} // namespace DAVA

// WinMain should have attribute which specifies threading model
[Platform::MTAThread]
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    using namespace DAVA;
    return Private::StartApplication(Private::GetCommandArgs());
}

#elif defined(__DAVAENGINE_ANDROID__)

#include <jni.h>
#include <android/log.h>

#include "Engine/Private/Android/AndroidBridge.h"

DAVA::Private::AndroidBridge* androidBridge = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        ANDROID_LOG_FATAL("JNI_OnLoad: failed to get environment");
        return -1;
    }

    if (androidBridge != nullptr)
    {
        ANDROID_LOG_FATAL("JNI_OnLoad: androidBridge is not null");
        return -1;
    }

    androidBridge = new DAVA::Private::AndroidBridge(vm);
    androidBridge->InitializeJNI(env);
    return JNI_VERSION_1_6;
}

#endif

// clang-format on
