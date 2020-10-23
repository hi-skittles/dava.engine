#include "TeamCityTestsOutput.h"
#include "Utils/Utils.h"

namespace DAVA
{
const String TeamcityPerformanceTestsOutput::START_TEST = "start test ";
const String TeamcityPerformanceTestsOutput::FINISH_TEST = "finish test ";
const String TeamcityPerformanceTestsOutput::ERROR_TEST = "test error ";
const String TeamcityPerformanceTestsOutput::STATISTIC = "statistic";
const String TeamcityPerformanceTestsOutput::AT_FILE_TEST = " at file: ";

const String TeamcityPerformanceTestsOutput::MIN_DELTA = "Min_frame_delta";
const String TeamcityPerformanceTestsOutput::MAX_DELTA = "Max_frame_delta";
const String TeamcityPerformanceTestsOutput::AVERAGE_DELTA = "Average_frame_delta";
const String TeamcityPerformanceTestsOutput::TEST_TIME = "Test_time";
const String TeamcityPerformanceTestsOutput::TIME_ELAPSED = "Time_elapsed";

const String TeamcityPerformanceTestsOutput::MIN_FPS = "Min_fps";
const String TeamcityPerformanceTestsOutput::MAX_FPS = "Max_fps";
const String TeamcityPerformanceTestsOutput::AVERAGE_FPS = "Average_fps";

const String TeamcityPerformanceTestsOutput::FRAME_DELTA = "Frame_delta";
const String TeamcityPerformanceTestsOutput::MAX_MEM_USAGE = "Max_memory_usage";

const String TeamcityPerformanceTestsOutput::MATERIAL_TEST_TIME = "Material_test_time";
const String TeamcityPerformanceTestsOutput::MATERIAL_ELAPSED_TEST_TIME = "Material__elapsed_test_time";
const String TeamcityPerformanceTestsOutput::MATERIAL_FRAME_DELTA = "Material_frame_delta";

void TeamcityPerformanceTestsOutput::Output(Logger::eLogLevel ll, const char8* text)
{
    String textStr = text;
    Vector<String> lines;
    Split(textStr, "\n", lines);

    String output;

    if (START_TEST == lines[0])
    {
        String testName = lines.at(1);
        output = "##teamcity[testStarted name=\'" + testName + "\']\n";
    }
    else if (FINISH_TEST == lines[0])
    {
        String testName = lines.at(1);
        output = "##teamcity[testFinished name=\'" + testName + "\']\n";
    }
    else if (ERROR_TEST == lines[0])
    {
        String testName = lines.at(1);
        String condition = NormalizeString(lines.at(2).c_str());
        String errorFileLine = NormalizeString(lines.at(3).c_str());
        output = "##teamcity[testFailed name=\'" + testName
        + "\' message=\'" + condition
        + "\' details=\'" + errorFileLine + "\']\n";
    }
    else if (STATISTIC == lines[0])
    {
        output = "##teamcity[buildStatisticValue key=\'" + lines[1] + "\' value=\'" + lines[2] + "\']\n";
    }
    else
    {
        TeamcityOutput::Output(ll, text);
        return;
    }

    TestOutput(output);
}

String TeamcityPerformanceTestsOutput::FormatTestStarted(const String& testName)
{
    return START_TEST + "\n" + testName;
}

String TeamcityPerformanceTestsOutput::FormatTestFinished(const String& testName)
{
    return FINISH_TEST + "\n" + testName;
}

String TeamcityPerformanceTestsOutput::FormatBuildStatistic(const String& key, const String& value)
{
    return STATISTIC + "\n" + key + "\n" + value + "\n";
}

String TeamcityPerformanceTestsOutput::FormatTestFailed(const String& testName, const String& condition, const String& errMsg)
{
    return ERROR_TEST + "\n" + testName + "\n" + condition + "\n" + errMsg;
}

void TeamcityPerformanceTestsOutput::TestOutput(const String& data)
{
    TeamcityOutput::PlatformOutput(data);
}

}; // end of namespace DAVA
