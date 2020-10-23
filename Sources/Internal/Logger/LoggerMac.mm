#include "Logger/Logger.h"

#if defined(__DAVAENGINE_MACOS__)

#include <cstdio>
namespace DAVA
{
void Logger::PlatformLog(eLogLevel ll, const char8* text)
{
    // Use printf instead of std::cout as std::cout can produce mess when
    // logging from several threads
    std::printf("[%s] %s", GetLogLevelString(ll), text);
}
} // namespace DAVA

#elif defined(__DAVAENGINE_IPHONE__)

#import <Foundation/Foundation.h>
namespace DAVA
{
void Logger::PlatformLog(eLogLevel ll, const char8* text)
{
    NSLog(@"[%s] %s", GetLogLevelString(ll), text);
}

} // namespace DAVA

#endif
