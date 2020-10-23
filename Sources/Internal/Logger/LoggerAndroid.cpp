#include "Logger/Logger.h"

#if defined(__DAVAENGINE_ANDROID__)

#include <stdarg.h>
#include <android/log.h>
#include "Utils/StringFormat.h"

namespace DAVA
{
static DAVA::String androidLogTag = "DAVA";

int32 LogLevelToAndtoid(Logger::eLogLevel ll)
{
    int32 androidLL = ANDROID_LOG_DEFAULT;
    switch (ll)
    {
    case Logger::LEVEL_FRAMEWORK:
    case Logger::LEVEL_DEBUG:
        androidLL = ANDROID_LOG_DEBUG;
        break;

    case Logger::LEVEL_INFO:
        androidLL = ANDROID_LOG_INFO;
        break;

    case Logger::LEVEL_WARNING:
        androidLL = ANDROID_LOG_WARN;
        break;

    case Logger::LEVEL_ERROR:
        androidLL = ANDROID_LOG_ERROR;
        break;
    default:
        break;
    }

    return androidLL;
}

void Logger::PlatformLog(eLogLevel ll, const char8* text)
{
    size_t len = strlen(text);
    // about limit on android: http://stackoverflow.com/questions/8888654/android-set-max-length-of-logcat-messages
    const size_t limit{ 4000 };

    char8* str = const_cast<char*>(text);

    while (len > limit)
    {
        char8 lastChar = str[limit];
        str[limit] = '\0';
        __android_log_print(LogLevelToAndtoid(ll), androidLogTag.c_str(), str, "");
        str[limit] = lastChar;
        str += limit;
        len -= limit;
    }

    __android_log_print(LogLevelToAndtoid(ll), androidLogTag.c_str(), str, "");
}

void Logger::SetTag(const char8* logTag)
{
    androidLogTag = Format("%s", logTag);
}

} // end namespace DAVA

#endif //#if defined(__DAVAENGINE_ANDROID__)
