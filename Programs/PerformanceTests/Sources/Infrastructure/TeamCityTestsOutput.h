#ifndef __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__
#define __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__

#include "Logger/TeamcityOutput.h"

namespace DAVA
{
class TeamcityPerformanceTestsOutput : public TeamcityOutput
{
public:
    virtual void Output(Logger::eLogLevel ll, const char8* text);

    static String FormatTestStarted(const String& testName);
    static String FormatTestFinished(const String& testName);
    static String FormatTestFailed(const String& testName, const String& condition, const String& errMsg);
    static String FormatBuildStatistic(const String& key, const String& value);

    static const String MIN_DELTA;
    static const String MAX_DELTA;
    static const String AVERAGE_DELTA;
    static const String TEST_TIME;
    static const String TIME_ELAPSED;

    static const String MIN_FPS;
    static const String MAX_FPS;
    static const String AVERAGE_FPS;

    static const String FRAME_DELTA;
    static const String MAX_MEM_USAGE;

    static const String MATERIAL_TEST_TIME;
    static const String MATERIAL_ELAPSED_TEST_TIME;
    static const String MATERIAL_FRAME_DELTA;

private:
    static const String START_TEST;
    static const String FINISH_TEST;
    static const String ERROR_TEST;
    static const String STATISTIC;
    static const String AT_FILE_TEST;

    void TestOutput(const String& data);
};
};

#endif // __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__
