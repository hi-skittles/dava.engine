#include "Platform/TemplateWin32/UAPNetworkHelper.h"
#include "Platform/DeviceInfo.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)

namespace DAVA
{
const char* UAPNetworkHelper::UAP_IP_ADDRESS = "127.0.0.1";

DeviceInfo::ePlatform GetPlatformChecked()
{
    DeviceInfo::ePlatform platform = DeviceInfo::GetPlatform();
    bool uapPlatform = platform == DeviceInfo::PLATFORM_DESKTOP_WIN_UAP ||
    platform == DeviceInfo::PLATFORM_PHONE_WIN_UAP;

    DVASSERT(uapPlatform, "Not UAP platform");
    return platform;
}

Net::eNetworkRole UAPNetworkHelper::GetCurrentNetworkRole()
{
    DeviceInfo::ePlatform platform = GetPlatformChecked();
    return platform == DeviceInfo::PLATFORM_DESKTOP_WIN_UAP ? Net::CLIENT_ROLE : Net::SERVER_ROLE;
}

Net::Endpoint UAPNetworkHelper::GetCurrentEndPoint()
{
    return GetEndPoint(GetCurrentNetworkRole());
}

Net::Endpoint UAPNetworkHelper::GetEndPoint(Net::eNetworkRole role)
{
    DeviceInfo::ePlatform platform = GetPlatformChecked();
    uint16 port;
    if (platform == DeviceInfo::PLATFORM_DESKTOP_WIN_UAP)
    {
        port = UAP_DESKTOP_TCP_PORT;
    }
    else
    {
        port = UAP_MOBILE_TCP_PORT;
    }

    switch (role)
    {
    case Net::SERVER_ROLE:
        return Net::Endpoint(port);
    case Net::CLIENT_ROLE:
        return Net::Endpoint(UAP_IP_ADDRESS, port);
    default:
        DVASSERT(false, "Something wrong");
        return Net::Endpoint();
    }
}

} // namespace DAVA

#endif
