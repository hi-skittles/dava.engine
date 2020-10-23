#ifndef __DAVAENGINE_TEAMCITY_OUTPUT_H__
#define __DAVAENGINE_TEAMCITY_OUTPUT_H__

/**
    \defgroup utils Utilities
 */

#include "Logger/Logger.h"

namespace DAVA
{
class TeamcityOutput : public LoggerOutput
{
public:
    void Output(Logger::eLogLevel ll, const char8* text) override;

protected:
    void PlatformOutput(const String& text) const;

    String NormalizeString(const char8* text) const;
};

} // namespace DAVA 

#endif // __DAVAENGINE_TEAMCITY_OUTPUT_H__
