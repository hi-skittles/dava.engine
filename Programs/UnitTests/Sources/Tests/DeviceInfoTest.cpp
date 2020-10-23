#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Platform/DeviceInfo.h"

using namespace DAVA;

DAVA_TESTCLASS (DeviceInfoTest)
{
    DAVA_TEST (TestFunction)
    {
        String osVersion = DeviceInfo::GetVersion();
        TEST_VERIFY("" != osVersion && "Not yet implemented" != osVersion);

        Logger::Debug(osVersion.c_str());

        String model = DeviceInfo::GetModel();
        Logger::Debug(model.c_str());

        eGPUFamily gpuModel = DeviceInfo::GetGPUFamily();
        DeviceInfo::NetworkInfo ninfo = DeviceInfo::GetNetworkInfo();

        String locale = DeviceInfo::GetLocale();
        TEST_VERIFY("" != locale && "Not yet implemented" != locale);

#if !defined(__DAVAENGINE_LINUX__)
        String region = DeviceInfo::GetRegion();
        TEST_VERIFY("" != region && "Not yet implemented" != region);

        String timeZone = DeviceInfo::GetTimeZone();
        TEST_VERIFY("" != timeZone); // not implemented for win32.
#endif

        String manufacturer = DeviceInfo::GetManufacturer();
        TEST_VERIFY("Not yet implemented" != manufacturer);
    }

    DAVA_TEST (TestGPUOverride)
    {
        eGPUFamily oldGPU = DeviceInfo::GetGPUFamily();
        eGPUFamily newGPU = oldGPU != GPU_TEGRA ? GPU_TEGRA : GPU_POWERVR_IOS;
        DeviceInfo::SetOverridenGPU(newGPU);
        TEST_VERIFY(DeviceInfo::GetGPUFamily() == newGPU);

        DeviceInfo::ResetOverridenGPU();
        TEST_VERIFY(oldGPU == DeviceInfo::GetGPUFamily());
    }
};
