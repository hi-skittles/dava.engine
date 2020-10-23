#pragma once

#include <Base/BaseTypes.h>
#include <Platform/DeviceInfo.h>

#include <Network/Base/IfAddress.h>
#include <Network/NetConfig.h>

namespace DAVA
{
namespace Net
{
class PeerDescription
{
public:
    PeerDescription();
    PeerDescription(const String& appName, const NetConfig& config);

    DeviceInfo::ePlatform GetPlatform() const;
    const String& GetPlatformString() const;
    const String& GetVersion() const;
    const String& GetManufacturer() const;
    const String& GetModel() const;
    const String& GetUDID() const;
    const String& GetDeviceName() const;
    const String& GetAppName() const;
    eGPUFamily GetGPUFamily() const;
    const NetConfig& NetworkConfig() const;
    const Vector<IfAddress>& NetworkInterfaces() const;
    void SetNetworkInterfaces(const Vector<IfAddress>& availIfAddr);

    size_t SerializedSize() const;
    size_t Serialize(void* dstBuffer, size_t buflen) const;
    size_t Deserialize(const void* srcBuffer, size_t buflen);
    static bool ExtractAppName(const void* srcBuffer, size_t buflen, String& appName);

#ifdef __DAVAENGINE_DEBUG__
    void DumpToStdout() const;
#endif

private:
    DeviceInfo::ePlatform platformType;
    String platform;
    String version;
    String manufacturer;
    String model;
    String udid;
    String deviceName;
    String appName;
    eGPUFamily gpuFamily;

    NetConfig netConfig;
    Vector<IfAddress> ifaddr;
};

//////////////////////////////////////////////////////////////////////////
inline DeviceInfo::ePlatform PeerDescription::GetPlatform() const
{
    return platformType;
}

inline const String& PeerDescription::GetPlatformString() const
{
    return platform;
}

inline const String& PeerDescription::GetVersion() const
{
    return version;
}

inline const String& PeerDescription::GetManufacturer() const
{
    return manufacturer;
}

inline const String& PeerDescription::GetModel() const
{
    return model;
}

inline const String& PeerDescription::GetUDID() const
{
    return udid;
}

inline const String& PeerDescription::GetDeviceName() const
{
    return deviceName;
}

inline const String& PeerDescription::GetAppName() const
{
    return appName;
}

inline eGPUFamily PeerDescription::GetGPUFamily() const
{
    return gpuFamily;
}

inline const NetConfig& PeerDescription::NetworkConfig() const
{
    return netConfig;
}

inline const Vector<IfAddress>& PeerDescription::NetworkInterfaces() const
{
    return ifaddr;
}

} // namespace Net
} // namespace DAVA
