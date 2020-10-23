#ifndef __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__
#define __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__

#include "Logger/TeamcityOutput.h"

namespace DAVA
{
class TeamcityTestsOutput : public TeamcityOutput
{
public:
    virtual void Output(Logger::eLogLevel ll, const char8* text);

    bool CaptureStdoutFlag() const;
    void SetCaptureStdoutFlag(bool value);

    static String FormatTestStarted(const String& testClassName, const String& testName);
    static String FormatTestFinished(const String& testClassName, const String& testName);
    static String FormatTestFailed(const String& testClassName, const String& testName, const String& condition, const String& errMsg);

    static String FormatTestClassStarted(const String& testClassName);
    static String FormatTestClassFinished(const String& testClassName);
    static String FormatTestClassDisabled(const String& testClassName);

private:
    void TestOutput(const String& data);

private:
    bool captureStdoutFlag = false; // Flag controls whether TeamCity attribute 'captureStandardOutput=true' is set on test start
    int64 testStartTime = 0;
    int64 suiteStartTime = 0;
};

//////////////////////////////////////////////////////////////////////////
inline bool TeamcityTestsOutput::CaptureStdoutFlag() const
{
    return captureStdoutFlag;
}

inline void TeamcityTestsOutput::SetCaptureStdoutFlag(bool value)
{
    captureStdoutFlag = value;
}

} // namespace DAVA

#endif // __DAVAENGINE_TEAMCITY_TEST_OUTPUT_H__
