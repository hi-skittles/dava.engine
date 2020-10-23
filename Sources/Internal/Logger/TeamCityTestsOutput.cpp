#include "Logger/TeamCityTestsOutput.h"
#include "Time/SystemTimer.h"
#include "Utils/StringFormat.h"
#include "Utils/Utils.h"

namespace DAVA
{
namespace
{
const String startSuiteMarker = "start suite";
const String finishSuiteMarker = "finish suite";
const String disabledSuiteMarker = "disable suite";
const String startTestMarker = "start test ";
const String finishTestMarker = "finish test ";
const String errorTestMarker = "test error ";

} // unnamed namespace

void TeamcityTestsOutput::Output(Logger::eLogLevel ll, const char8* text)
{
    String textStr = text;
    Vector<String> lines;
    Split(textStr, "\n", lines);

    String output;
    if (startSuiteMarker == lines[0])
    {
        const String& testName = lines.at(1);
        output = "##teamcity[testSuiteStarted name='" + testName + "']\n";
        suiteStartTime = SystemTimer::GetMs();
    }
    else if (finishSuiteMarker == lines[0])
    {
        int64 deltaTime = SystemTimer::GetMs() - suiteStartTime;

        const String& testName = lines.at(1);
        output = "##teamcity[testSuiteFinished name='" + testName + Format("' time='%d.%03d sec']\n", deltaTime / 1000, deltaTime % 1000);
    }
    else if (disabledSuiteMarker == lines[0])
    {
        const String& testName = lines.at(1);
        output = "##teamcity[testIgnored name='" + testName + "' message='test is disabled']\n";
    }
    else if (startTestMarker == lines[0])
    {
        const String& testName = lines.at(1);
        output = "##teamcity[testStarted name='" + testName + "'";
        if (captureStdoutFlag)
        {
            output += " captureStandardOutput='true'";
        }
        output += "]\n";
        testStartTime = SystemTimer::GetMs();
    }
    else if (finishTestMarker == lines[0])
    {
        int64 deltaTime = SystemTimer::GetMs() - testStartTime;

        const String& testName = lines.at(1);
        output = "##teamcity[testFinished name='" + testName + Format("' time='%d.%03d sec']\n", deltaTime / 1000, deltaTime % 1000);
    }
    else if (errorTestMarker == lines[0])
    {
        const String& testName = lines.at(1);
        String condition = NormalizeString(lines.at(2).c_str());
        String errorFileLine = NormalizeString(lines.at(3).c_str());
        output = "##teamcity[testFailed name='" + testName
        + "' message='" + condition
        + "' details='" + errorFileLine + "']\n";
    }
    else
    {
        TeamcityOutput::Output(ll, text);
        return;
    }
    TestOutput(output);
}

String TeamcityTestsOutput::FormatTestStarted(const String& testClassName, const String& testName)
{
    return startTestMarker + "\n" + testClassName + "." + testName;
}

String TeamcityTestsOutput::FormatTestFinished(const String& testClassName, const String& testName)
{
    return finishTestMarker + "\n" + testClassName + "." + testName;
}

String TeamcityTestsOutput::FormatTestClassStarted(const String& testClassName)
{
    return startSuiteMarker + "\n" + testClassName;
}

String TeamcityTestsOutput::FormatTestClassFinished(const String& testClassName)
{
    return finishSuiteMarker + "\n" + testClassName;
}

String TeamcityTestsOutput::FormatTestClassDisabled(const String& testClassName)
{
    return disabledSuiteMarker + "\n" + testClassName;
}

String TeamcityTestsOutput::FormatTestFailed(const String& testClassName, const String& testName, const String& condition, const String& errMsg)
{
    return errorTestMarker + "\n" + testName + "\n" + condition + "\n" + errMsg;
}

void TeamcityTestsOutput::TestOutput(const String& data)
{
    TeamcityOutput::PlatformOutput(data);
}

} // end of namespace DAVA
