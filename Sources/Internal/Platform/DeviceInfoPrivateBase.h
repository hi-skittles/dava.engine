#pragma once
#include "Platform/DeviceInfo.h"

namespace DAVA
{
//Common implementation of device info
class DeviceInfoPrivateBase
{
public:
    virtual ~DeviceInfoPrivateBase() = default;
    int32 GetCpuCount();
    DeviceInfo::HIDConnectionSignal& GetHIDConnectionSignal(DeviceInfo::eHIDType type);

    eGPUFamily GetGPUFamily();
    virtual eGPUFamily GetGPUFamilyImpl() = 0;

    void SetOverridenGPU(eGPUFamily newGPU);
    void ResetOverridenGPU();

private:
    Map<DeviceInfo::eHIDType, DeviceInfo::HIDConnectionSignal> hidConnectionSignals;
    eGPUFamily overridenGPU = GPU_INVALID;
};

} // namespace DAVA
