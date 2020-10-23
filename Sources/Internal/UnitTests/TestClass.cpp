#include "TestClass.h"
#include "Debug/Backtrace.h"

namespace DAVA
{
namespace UnitTests
{
void TestClass::InitTimeStampForTest(const String& testName)
{
    auto iter = std::find_if(tests.begin(), tests.end(), [&testName](const TestInfo& testInfo)
                             {
                                 return testInfo.name == testName;
                             });

    iter->startTime = TestInfo::Clock::now();
}

void TestClass::SetUp(const String& testName)
{
}

void TestClass::TearDown(const String& testName)
{
}

void TestClass::Update(float32 timeElapsed, const String& testName)
{
}

bool TestClass::TestComplete(const String& testName) const
{
    return true;
}

TestCoverageInfo TestClass::FilesCoveredByTests() const
{
    return TestCoverageInfo();
}

String TestClass::PrettifyTypeName(const String& name) const
{
#if defined(__DAVAENGINE_APPLE__) || defined(__DAVAENGINE_ANDROID__)
    String result = Debug::DemangleFrameSymbol(name);
#else
    // On Windows names are already demangled
    String result = name;
#endif

    size_t spacePos = result.find_last_of(": ");
    if (spacePos != String::npos)
    {
        return result.substr(spacePos + 1);
    }
    return result;
}

String TestClass::RemoveTestPostfix(const String& name) const
{
    String lowcase = name;
    // Convert name to lower case
    std::transform(lowcase.begin(), lowcase.end(), lowcase.begin(), [](char ch) -> char { return 'A' <= ch && ch <= 'Z' ? ch - 'A' + 'a' : ch; });
    size_t pos = lowcase.rfind("test");
    // If name ends with 'test' discard it
    if (pos != String::npos && pos > 0 && lowcase.length() - pos == 4)
    {
        return name.substr(0, pos);
    }
    return name;
}

} // namespace UnitTests
} // namespace DAVA
