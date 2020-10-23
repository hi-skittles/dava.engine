#include "UnitTests/UnitTests.h"

#include "Concurrency/Thread.h"
#include "Time/SystemTimer.h"
#include "Utils/StringFormat.h"

#include <ctime>

DAVA_TESTCLASS (TimerTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("SystemTimer.cpp")
    END_FILES_COVERED_BY_TESTS()

    DAVA_TEST (TestPrecision)
    {
        using namespace DAVA;

        const int64 sleepTimeMs[] = {
            1, 50, 100, 500, 1000, 2000
        };

        for (int64 sleepTime : sleepTimeMs)
        {
            int64 beginMs = SystemTimer::GetMs();
            int64 beginUs = SystemTimer::GetUs();
            int64 beginNs = SystemTimer::GetNs();

            Thread::Sleep(static_cast<uint32>(sleepTime));

            int64 deltaMs = SystemTimer::GetMs() - beginMs;
            int64 deltaUs = SystemTimer::GetUs() - beginUs;
            int64 deltaNs = SystemTimer::GetNs() - beginNs;

            // Some platforms may sleep less than specified (e.g. Windows), so descrease sleep time by 1 ms
            TEST_VERIFY_WITH_MESSAGE(deltaMs >= (sleepTime - 1), Format("deltaMs=%lld, sleepTime=%lld", deltaMs, sleepTime));
            TEST_VERIFY_WITH_MESSAGE(deltaUs >= (sleepTime - 1) * 1000ll, Format("deltaUs=%lld, sleepTime=%lld", deltaUs, sleepTime * 1000ll));
            TEST_VERIFY_WITH_MESSAGE(deltaNs >= (sleepTime - 1) * 1000000ll, Format("deltaNs=%lld, sleepTime=%lld", deltaNs, sleepTime * 1000000ll));
        }

        {
            int64 begin = SystemTimer::GetSystemUptimeUs();
            Thread::Sleep(2000);
            int64 delta = SystemTimer::GetSystemUptimeUs() - begin;
            // I do not know period system updates its uptime so choose value less than sleep time by 500ms
            TEST_VERIFY_WITH_MESSAGE(delta >= 1500000ll, Format("delta=%lld", delta));
        }
    }

    DAVA_TEST (TestFrameDelta)
    {
        // All logic in Update method for this test
    }

    DAVA_TEST (TestDeprecatedForCoverage)
    {
        // TODO: remove this test after removing deprecated SystemTimer methods
        using namespace DAVA;
        float32 globalTime = SystemTimer::GetGlobalTime();
        SystemTimer::ResetGlobalTime();
        SystemTimer::PauseGlobalTime();
        SystemTimer::ResumeGlobalTime();
        TEST_VERIFY(globalTime >= 0.f);
    }

    void Update(DAVA::float32 timeElapsed, const DAVA::String& testName) override
    {
        using namespace DAVA;

        if (testName != "TestFrameDelta")
            return;

        if (testFrameDeltaStage == 0)
        {
            prevFrameTimestampMs = SystemTimer::GetFrameTimestampMs();

            // On first Update call make sleep to ensure real frame delta is greater than frame delta
            Thread::Sleep(1000);

            // Test that frame timestamp does not change during frame
            int64 checkTimestampMs = SystemTimer::GetFrameTimestampMs();
            TEST_VERIFY(checkTimestampMs == prevFrameTimestampMs);
        }
        else
        {
            // On next Update call check real frame delta and frame delta
            float32 frameDelta = SystemTimer::GetFrameDelta();
            float32 realFrameDelta = SystemTimer::GetRealFrameDelta();

            TEST_VERIFY(0.001f <= frameDelta && frameDelta <= 0.1f);
            TEST_VERIFY(frameDelta == timeElapsed);
            TEST_VERIFY(realFrameDelta >= 1.f);

            // Test that next frame timestamp is greater than previous
            int64 checkTimestampMs = SystemTimer::GetFrameTimestampMs();
            TEST_VERIFY(checkTimestampMs > prevFrameTimestampMs);
        }
        testFrameDeltaStage += 1;
    }

    bool TestComplete(const DAVA::String& testName) const override
    {
        if (testName == "TestFrameDelta")
        {
            return testFrameDeltaStage > 1;
        }
        return true;
    }

    int testFrameDeltaStage = 0;
    DAVA::int64 prevFrameTimestampMs = 0;
};
