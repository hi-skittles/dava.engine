#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (GPUFamilyTest)
{
    DAVA_TEST (TestPrimaryGPU)
    {
        TEST_VERIFY(Texture::GetPrimaryGPUForLoading() == DeviceInfo::GetGPUFamily());
        
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WINDOWS__) || defined(__DAVAENGINE_WIN_UAP__)
        TEST_VERIFY(Texture::GetPrimaryGPUForLoading() == eGPUFamily::GPU_DX11);
#elif defined(__DAVAENGINE_LINUX__)
        // TODO: linux
        TEST_VERIFY(Texture::GetPrimaryGPUForLoading() == eGPUFamily::GPU_ORIGIN);
#elif defined(__DAVAENGINE_IPHONE__)
        TEST_VERIFY(Texture::GetPrimaryGPUForLoading() == eGPUFamily::GPU_POWERVR_IOS);
#elif defined(__DAVAENGINE_ANDROID__)
        eGPUFamily gpu = Texture::GetPrimaryGPUForLoading();

        TEST_VERIFY(
        gpu == eGPUFamily::GPU_POWERVR_ANDROID
        || gpu == eGPUFamily::GPU_TEGRA
        || gpu == eGPUFamily::GPU_MALI
        || gpu == eGPUFamily::GPU_ADRENO
        );
#else
        TEST_VERIFY(false);
#endif
    }

    DAVA_TEST (TestGPUForLoading)
    {
        const Vector<eGPUFamily>& gpuLoadingOrder = Texture::GetGPULoadingOrder();
        
#if defined(__DAVAENGINE_ANDROID__)
        if (DeviceInfo::GetGPUFamily() == eGPUFamily::GPU_MALI)
        {
            TEST_VERIFY(gpuLoadingOrder.size() == 1);
        }
        else
        {
            TEST_VERIFY(gpuLoadingOrder.size() == 2);
            if (gpuLoadingOrder.size() == 2)
            {
                TEST_VERIFY(gpuLoadingOrder[1] == eGPUFamily::GPU_MALI);
            }
        }
#else //Android
        TEST_VERIFY(gpuLoadingOrder.size() == 1);
#endif //Andoird
    }
};
