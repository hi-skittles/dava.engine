#include "Logger/Logger.h"

#if defined(__DAVAENGINE_LINUX__)

#include <cstdio>

namespace DAVA
{
void Logger::PlatformLog(eLogLevel ll, const char8* text)
{
    std::fprintf(stdout, "[%s] %s", GetLogLevelString(ll), text);
}
} // namespace DAVA

#endif
