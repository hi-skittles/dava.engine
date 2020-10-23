#pragma once

#include "Base/Platform.h"
#include "Platform/DeviceInfoPrivateBase.h"

DAVA_FORWARD_DECLARE_OBJC_CLASS(NSString);
DAVA_FORWARD_DECLARE_OBJC_CLASS(CTTelephonyNetworkInfo);

namespace DAVA
{
class DeviceInfoPrivate : public DeviceInfoPrivateBase
{
public:
    DeviceInfoPrivate();
    ~DeviceInfoPrivate();
    DeviceInfo::ePlatform GetPlatform();
    String GetPlatformString();
    String GetVersion();
    String GetManufacturer();
    String GetModel();
    String GetLocale();
    String GetRegion();
    String GetTimeZone();
    String GetUDID();
    WideString GetName();
    String GetHTTPProxyHost();
    String GetHTTPNonProxyHosts();
    int32 GetHTTPProxyPort();
    int32 GetZBufferSize();
    eGPUFamily GetGPUFamilyImpl() override;
    DeviceInfo::NetworkInfo GetNetworkInfo();
    List<DeviceInfo::StorageInfo> GetStoragesList();
    bool IsHIDConnected(DeviceInfo::eHIDType type);
    bool IsTouchPresented();
    String GetCarrierName();

private:
    CTTelephonyNetworkInfo* telephonyNetworkInfo = nullptr;
    NSString* lastCarrierName = nullptr;
};

}; // namespace DAVA
