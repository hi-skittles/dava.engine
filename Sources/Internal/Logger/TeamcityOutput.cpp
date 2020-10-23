#include "Logger/TeamcityOutput.h"
#include "Engine/Engine.h"
#include "Utils/Utils.h"
#include "Engine/Engine.h"

#include <iostream>

#if defined(__DAVAENGINE_ANDROID__)
#include <android/log.h>
#define LOG_TAG "TeamcityOutput"
#elif defined(__DAVAENGINE_IPHONE__)
#import "UIKit/UIKit.h"
#endif

namespace DAVA
{
void TeamcityOutput::Output(Logger::eLogLevel ll, const char8* text)
{
    if (ll < GetEngineContext()->logger->GetLogLevel())
        return;

    String outStr = NormalizeString(text);
    String output;
    String status;

    switch (ll)
    {
    case Logger::LEVEL_ERROR:
        status = "WARNING";
        break;
    case Logger::LEVEL_WARNING:
        status = "WARNING";
        break;
    default:
        status = "NORMAL";
        break;
    }

    output = "##teamcity[message text=\'" + outStr + "\' errorDetails=\'\' status=\'" + status + "\']\n";
    PlatformOutput(output);
}

String TeamcityOutput::NormalizeString(const char8* text) const
{
    String str = text;

    StringReplace(str, "|", "||");

    StringReplace(str, "'", "|'");
    StringReplace(str, "\n", "|n");
    StringReplace(str, "\r", "|r");

    StringReplace(str, "[", "|[");
    StringReplace(str, "]", "|]");

    return str;
}

void TeamcityOutput::PlatformOutput(const String& text) const
{
#ifdef __DAVAENGINE_IPHONE__
    NSLog(@"%s", text.c_str());
#elif defined(__DAVAENGINE_ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", text.c_str());
#else
    std::cout << text << std::endl
              << std::flush;
#endif
}

}; // end of namespace DAVA
