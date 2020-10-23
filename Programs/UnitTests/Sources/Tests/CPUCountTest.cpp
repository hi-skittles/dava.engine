#include "DAVAEngine.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

extern int32 GetCpuCount();

DAVA_TESTCLASS (CPUCountTest)
{
    DAVA_TEST (StandardVersusPlatformRealizationTest)
    {
        int32 cpuCountFromStandardRealization = DeviceInfo::GetCpuCount();
        int32 cpuCountFromPlatformRealization = GetCpuCount();

        TEST_VERIFY(cpuCountFromStandardRealization == cpuCountFromPlatformRealization);
    }
};