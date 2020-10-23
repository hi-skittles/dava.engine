#include "Platform/DeviceInfoPrivateBase.h"
#include <thread>

namespace DAVA
{
int32 DeviceInfoPrivateBase::GetCpuCount()
{
    size_t processors = std::thread::hardware_concurrency();
    return processors != 0 ? static_cast<int32>(processors) : 1;
}

DeviceInfo::HIDConnectionSignal& DeviceInfoPrivateBase::GetHIDConnectionSignal(
DeviceInfo::eHIDType type)
{
    return hidConnectionSignals[type];
}

eGPUFamily DeviceInfoPrivateBase::GetGPUFamily()
{
    return overridenGPU != GPU_INVALID ? overridenGPU : GetGPUFamilyImpl();
}

void DeviceInfoPrivateBase::SetOverridenGPU(eGPUFamily newGPU)
{
    overridenGPU = newGPU;
}

void DeviceInfoPrivateBase::ResetOverridenGPU()
{
    overridenGPU = GPU_INVALID;
}

} // namespace DAVA