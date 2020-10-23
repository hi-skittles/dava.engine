#ifndef __DAVAENGINE_TESTCLASS_H__
#define __DAVAENGINE_TESTCLASS_H__

#include "Base/BaseTypes.h"

#include <chrono>

namespace DAVA
{
namespace UnitTests
{
template <typename T>
struct TestClassTypeKeeper
{
    using TestClassType = T;
};

struct TestCoverageInfo
{
    Vector<String> testFiles;
    Map<String, String> targetFolders;
};

class TestClass
{
public:
    TestClass() = default;
    virtual ~TestClass() = default;

    virtual void InitTimeStampForTest(const String& testName);

    virtual void SetUp(const String& testName);
    virtual void TearDown(const String& testName);
    virtual void Update(float32 timeElapsed, const String& testName);
    virtual bool TestComplete(const String& testName) const;
    virtual TestCoverageInfo FilesCoveredByTests() const;

    virtual const String& TestName(size_t index) const;
    virtual size_t TestCount() const;
    virtual void RunTest(size_t index);

    void RegisterTest(const char* name, void (*testFunc)(TestClass*));

protected:
    String PrettifyTypeName(const String& name) const;
    String RemoveTestPostfix(const String& name) const;

protected:
    struct TestInfo
    {
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;

        TestInfo(const char* name_, void (*testFunction_)(TestClass*))
            : name(name_)
            , testFunction(testFunction_)
        {
        }

        TimePoint startTime;
        String name;
        void (*testFunction)(TestClass*);
    };

    Vector<TestInfo> tests;
};

//////////////////////////////////////////////////////////////////////////
inline const String& TestClass::TestName(size_t index) const
{
    return tests[index].name;
}

inline size_t TestClass::TestCount() const
{
    return tests.size();
}

inline void TestClass::RunTest(size_t index)
{
    tests[index].testFunction(this);
}

inline void TestClass::RegisterTest(const char* name, void (*testFunc)(TestClass*))
{
    tests.emplace_back(name, testFunc);
}

} // namespace UnitTests
} // namespace DAVA

#endif // __DAVAENGINE_TESTCLASS_H__
