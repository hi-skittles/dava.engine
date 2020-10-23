#include "Platform/DeviceInfo.h"

#if defined(__DAVAENGINE_IPHONE__)
    #include "Platform/TemplateiOS/DeviceInfo.Ios.h"
#elif defined(__DAVAENGINE_MACOS__)
    #include "Platform/TemplateMacOS/DeviceInfo.Macos.h"
#elif defined(__DAVAENGINE_ANDROID__)
    #include "Engine/Private/Android/DeviceInfoAndroid.h"
#elif defined(__DAVAENGINE_WIN32__)
    #include "Platform/TemplateWin32/DeviceInfo.Win.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
    #include "Platform/TemplateWin32/DeviceInfo.Win10.h"
#elif defined(__DAVAENGINE_LINUX__)
    #include "Platform/Linux/DeviceInfo.Linux.h"
#endif

namespace DAVA
{
DeviceInfoPrivate* DeviceInfo::GetPrivateImpl()
{
    static DeviceInfoPrivate privateImpl;
    return &privateImpl;
}

DeviceInfo::ePlatform DeviceInfo::GetPlatform()
{
    return GetPrivateImpl()->GetPlatform();
}

String DeviceInfo::GetPlatformString()
{
    return GetPrivateImpl()->GetPlatformString();
}

String DeviceInfo::GetVersion()
{
    return GetPrivateImpl()->GetVersion();
}

String DeviceInfo::GetManufacturer()
{
    return GetPrivateImpl()->GetManufacturer();
}

String DeviceInfo::GetModel()
{
    return GetPrivateImpl()->GetModel();
}

String DeviceInfo::GetLocale()
{
    return GetPrivateImpl()->GetLocale();
}

String DeviceInfo::GetRegion()
{
    return GetPrivateImpl()->GetRegion();
}

String DeviceInfo::GetTimeZone()
{
    return GetPrivateImpl()->GetTimeZone();
}

String DeviceInfo::GetUDID()
{
    return GetPrivateImpl()->GetUDID();
}

WideString DeviceInfo::GetName()
{
    return GetPrivateImpl()->GetName();
}

String DeviceInfo::GetHTTPProxyHost()
{
    return GetPrivateImpl()->GetHTTPProxyHost();
}

String DeviceInfo::GetHTTPNonProxyHosts()
{
    return GetPrivateImpl()->GetHTTPNonProxyHosts();
}

int32 DeviceInfo::GetHTTPProxyPort()
{
    return GetPrivateImpl()->GetHTTPProxyPort();
}

int32 DeviceInfo::GetZBufferSize()
{
    return GetPrivateImpl()->GetZBufferSize();
}

eGPUFamily DeviceInfo::GetGPUFamily()
{
    return GetPrivateImpl()->GetGPUFamily();
}

void DeviceInfo::SetOverridenGPU(eGPUFamily newGPU)
{
    GetPrivateImpl()->SetOverridenGPU(newGPU);
}

void DeviceInfo::ResetOverridenGPU()
{
    GetPrivateImpl()->ResetOverridenGPU();
}

DeviceInfo::NetworkInfo DeviceInfo::GetNetworkInfo()
{
    return GetPrivateImpl()->GetNetworkInfo();
}

List<DeviceInfo::StorageInfo> DeviceInfo::GetStoragesList()
{
    return GetPrivateImpl()->GetStoragesList();
}

int32 DeviceInfo::GetCpuCount()
{
    return GetPrivateImpl()->GetCpuCount();
}

bool DeviceInfo::IsTouchPresented()
{
    return GetPrivateImpl()->IsTouchPresented();
}

String DeviceInfo::GetCarrierName()
{
    return GetPrivateImpl()->GetCarrierName();
}

bool DeviceInfo::IsHIDConnected(eHIDType type)
{
    return GetPrivateImpl()->IsHIDConnected(type);
}

DeviceInfo::HIDConnectionSignal& DeviceInfo::GetHIDConnectionSignal(DeviceInfo::eHIDType type)
{
    return GetPrivateImpl()->GetHIDConnectionSignal(type);
}

Signal<const String&> DeviceInfo::carrierNameChanged;

} // namespace DAVA
