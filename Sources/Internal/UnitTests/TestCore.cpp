#include "TestCore.h"

#include "Debug/DVAssert.h"
#include "Utils/Utils.h"

#include "UnitTests/TestClassFactory.h"
#include "UnitTests/TestClass.h"

namespace DAVA
{
namespace UnitTests
{
// Move dtor to source file to prevent clang warning: 'class' has no out-of-line virtual method definitions
TestClassFactoryBase::~TestClassFactoryBase() = default;

TestCore::TestClassInfo::TestClassInfo(const char* name_, TestClassFactoryBase* factory_)
    : name(name_)
    , factory(factory_)
{
}

TestCore::TestClassInfo::TestClassInfo(TestClassInfo&& other)
    : name(std::move(other.name))
    , runTest(other.runTest)
    , factory(std::move(other.factory))
    , testedFiles(std::move(other.testedFiles))
{
}

TestCore::TestClassInfo::~TestClassInfo()
{
}

//////////////////////////////////////////////////////////////////////////
TestCore* TestCore::Instance()
{
    static TestCore core;
    return &core;
}

void TestCore::Init(TestClassStartedCallback testClassStartedCallback_, TestClassFinishedCallback testClassFinishedCallback_,
                    TestStartedCallback testStartedCallback_, TestFinishedCallback testFinishedCallback_,
                    TestFailedCallback testFailedCallback_, TestClassDisabledCallback testClassDisabledCallback_)
{
    DVASSERT(testClassStartedCallback_ != nullptr && testClassFinishedCallback_ != nullptr && testClassDisabledCallback_ != nullptr);
    DVASSERT(testStartedCallback_ != nullptr && testFinishedCallback_ != nullptr && testFailedCallback_ != nullptr);
    testClassStartedCallback = testClassStartedCallback_;
    testClassFinishedCallback = testClassFinishedCallback_;
    testStartedCallback = testStartedCallback_;
    testFinishedCallback = testFinishedCallback_;
    testFailedCallback = testFailedCallback_;
    testClassDisabledCallback = testClassDisabledCallback_;
}

void TestCore::RunOnlyTheseTestClasses(const String& testClassNames)
{
    DVASSERT(!runLoopInProgress);
    if (!testClassNames.empty())
    {
        Vector<String> testNames;
        Split(testClassNames, ";, ", testNames);

        // First, disable all tests
        for (TestClassInfo& x : testClasses)
        {
            x.runTest = false;
        }

        // Enable only specified tests
        for (const String& testName : testNames)
        {
            auto iter = std::find_if(testClasses.begin(), testClasses.end(), [&testName](const TestClassInfo& testClassInfo) -> bool {
                return testClassInfo.name == testName;
            });
            DVASSERT(iter != testClasses.end() && "Test classname is not found among registered tests");
            if (iter != testClasses.end())
            {
                iter->runTest = true;
            }
        }
    }
}

void TestCore::DisableTheseTestClasses(const String& testClassNames)
{
    DVASSERT(!runLoopInProgress);
    if (!testClassNames.empty())
    {
        Vector<String> testNames;
        Split(testClassNames, ";, ", testNames);

        for (const String& testName : testNames)
        {
            auto iter = std::find_if(testClasses.begin(), testClasses.end(), [&testName](const TestClassInfo& testClassInfo) -> bool {
                return testClassInfo.name == testName;
            });
            DVASSERT(iter != testClasses.end() && "Test classname is not found among registered tests");
            if (iter != testClasses.end())
            {
                iter->runTest = false;
            }
        }
    }
}

bool TestCore::HasTestClasses() const
{
    ptrdiff_t n = std::count_if(testClasses.begin(), testClasses.end(), [](const TestClassInfo& info) -> bool { return info.runTest; });
    return n > 0;
}

bool TestCore::ProcessTests(float32 timeElapsed)
{
    runLoopInProgress = true;
    const size_t testClassCount = testClasses.size();
    if (curTestClassIndex < testClassCount)
    {
        if (nullptr == curTestClass)
        {
            TestClassInfo& testClassInfo = testClasses[curTestClassIndex];
            curTestClassName = testClassInfo.name;
            if (testClassInfo.runTest)
            {
                curTestClass = testClassInfo.factory->CreateTestClass();
                testClassStartedCallback(curTestClassName);
            }
            else
            {
                testClassStartedCallback(curTestClassName);
                testClassDisabledCallback(curTestClassName);
                testClassFinishedCallback(curTestClassName);
                curTestClassIndex += 1;
            }
        }

        if (curTestClass != nullptr)
        {
            if (curTestIndex < curTestClass->TestCount())
            {
                if (!testSetUpInvoked)
                {
                    curTestName = curTestClass->TestName(curTestIndex);
                    testStartedCallback(curTestClassName, curTestName);
                    curTestClass->InitTimeStampForTest(curTestName);
                    curTestClass->SetUp(curTestName);
                    testSetUpInvoked = true;
                    curTestClass->RunTest(curTestIndex);
                }

                if (curTestClass->TestComplete(curTestName))
                {
                    testSetUpInvoked = false;
                    curTestClass->TearDown(curTestName);
                    testFinishedCallback(curTestClassName, curTestName);
                    curTestIndex += 1;
                }
                else
                {
                    curTestClass->Update(timeElapsed, curTestName);
                }
            }
            else
            {
                testClassFinishedCallback(curTestClassName);

                if (curTestClass->TestCount() > 0)
                { // Get and save files names which are covered by test only if test files has tests
                    testClasses[curTestClassIndex].testedFiles = curTestClass->FilesCoveredByTests();
                }

                SafeDelete(curTestClass);
                curTestIndex = 0;
                curTestClassIndex += 1;
            }
        }
        return true;
    }
    else
    {
        runLoopInProgress = false;
        return false; // No more tests, finish
    }
}

Map<String, TestCoverageInfo> TestCore::GetTestCoverage()
{
    Map<String, TestCoverageInfo> result;
    for (TestClassInfo& x : testClasses)
    {
        if (!x.testedFiles.testFiles.empty())
        {
            result.emplace(x.name, std::move(x.testedFiles));
        }
    }
    return result;
}

void TestCore::TestFailed(const String& condition, const char* filename, int lineno, const String& userMessage)
{
    testFailedCallback(curTestClassName, curTestName, condition, filename, lineno, userMessage);
}

void TestCore::RegisterTestClass(const char* name, TestClassFactoryBase* factory)
{
    testClasses.emplace_back(name, factory);
}

} // namespace UnitTests
} // namespace DAVA
