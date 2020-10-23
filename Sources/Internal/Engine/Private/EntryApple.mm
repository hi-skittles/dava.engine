#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSAutoreleasePool.h>

#include "Base/Exception.h"
#include "Base/Platform.h"
#include "Debug/Backtrace.h"
#include "Engine/Private/CommandArgs.h"
#include "Engine/Private/EngineBackend.h"
#include "Logger/Logger.h"

extern int DAVAMain(DAVA::Vector<DAVA::String> cmdline);

NSAutoreleasePool* preMainLoopReleasePool = nullptr;

int main(int argc, char* argv[])
{
    using namespace DAVA;
    using DAVA::Private::EngineBackend;

    // NSApplicationMain/UIApplicationMain will be called only inside of Engine::Run
    // Before that happens we should manage another autorelease pool to use during initialization
    // (in case [... autorelease] message is used)
    // It will be drained right before NSApplicationMain/UIApplicationMain is called in PlatformCore
    preMainLoopReleasePool = [[NSAutoreleasePool alloc] init];

    try
    {
        Vector<String> cmdargs = Private::GetCommandArgs(argc, argv);
        std::unique_ptr<EngineBackend> engineBackend(new EngineBackend(cmdargs));
        return DAVAMain(std::move(cmdargs));
    }
    catch (const Exception& e)
    {
        StringStream ss;
        ss << "!!! Unhandled DAVA::Exception \"" << e.what() << "\" at `" << e.file << "`:" << e.line << std::endl;
        ss << Debug::GetBacktraceString(e.callstack) << std::endl;
        Logger::PlatformLog(Logger::LEVEL_ERROR, ss.str().c_str());
        throw;
    }
}

#endif // __DAVAENGINE_MACOS__ || __DAVAENGINE_IPHONE__
