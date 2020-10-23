#include <Debug/DVAssert.h>

#include <Network/PeerDesription.h>

namespace DAVA
{
namespace Net
{
namespace PeerDescriptionDetails
{
struct SerializedHeader
{
    uint32 magicZero; // Magic zero
    uint32 totalSize; // Total length of serialized data including this header
    uint32 checkSum; // Simple checksum of serialized data not including this header
    uint32 ifadrCount; // Number of network interfaces
    uint32 networkRole; // Network role in network configuration
    uint32 transpCount; // Number of transports in network configuration
    uint32 serviceCount; // Number of services in network configuration
};

struct SerializedGeneralInfo
{
    uint32 platfromType;
    uint32 gpuFamily;
    int32 screenWidth;
    int32 screenHeight;
    int32 screenScale;
    Array<char8, 16> platform;
    Array<char8, 16> version;
    Array<char8, 16> manufacturer;
    Array<char8, 32> model;
    Array<char8, 64> udid;
    Array<char8, 32> deviceName;
    Array<char8, 32> appName;
};
// make sure that sizeof SerializedGeneralInfo with arrays like char8 arr[] is same as witn std::array
static_assert(228 == sizeof(SerializedGeneralInfo), "invalid SerializedGeneralInfo size.");

struct SerializedIfAddress
{
    uint32 addr;
    uint32 mask;
    uint8 physAddr[6];
    uint16 isInternal;
};

struct SerializedTransport
{
    uint32 type;
    uint32 addr;
    uint32 port;
};

DeviceInfo::ePlatform IntToPlatform(uint32 n)
{
    return DeviceInfo::PLATFORM_MACOS <= n && n < DeviceInfo::PLATFORMS_COUNT ? static_cast<DeviceInfo::ePlatform>(n) : DeviceInfo::PLATFORM_UNKNOWN_VALUE;
}

eGPUFamily IntToGPUFamily(uint32 n)
{
    return GPU_POWERVR_IOS <= n && n < GPU_FAMILY_COUNT ? static_cast<eGPUFamily>(n)
                                                          :
                                                          GPU_INVALID;
}

eTransportType IntToTransportType(uint32 n)
{
    switch (n)
    {
    case TRANSPORT_TCP:
        return TRANSPORT_TCP;
    }
    DVASSERT(0);
    return TRANSPORT_TCP;
}

eNetworkRole IntToNetworkRole(uint32 n)
{
    switch (n)
    {
    case SERVER_ROLE:
        return SERVER_ROLE;
    case CLIENT_ROLE:
        return CLIENT_ROLE;
    }
    DVASSERT(0);
    return SERVER_ROLE;
}

size_t EstimateBufferSize(const SerializedHeader* header)
{
    return
    sizeof(SerializedHeader) +
    sizeof(SerializedGeneralInfo) +
    sizeof(SerializedIfAddress) * header->ifadrCount +
    sizeof(SerializedTransport) * header->transpCount +
    sizeof(uint32) * header->serviceCount;
}

bool ValidateHeader(const SerializedHeader* header, size_t buflen)
{
    size_t estimatedSize = EstimateBufferSize(header);
    // TODO: verify checksum
    return (0 == header->magicZero
            && header->totalSize <= buflen
            && header->totalSize == estimatedSize
            && (SERVER_ROLE == header->networkRole || CLIENT_ROLE == header->networkRole));
}
} // namespace PeerDescriptionDetails

size_t PeerDescription::SerializedSize() const
{
    using namespace PeerDescriptionDetails;

    return
    sizeof(SerializedHeader) +
    sizeof(SerializedGeneralInfo) +
    sizeof(SerializedIfAddress) * ifaddr.size() +
    sizeof(SerializedTransport) * netConfig.Transports().size() +
    sizeof(uint32) * netConfig.Services().size();
}

size_t PeerDescription::Serialize(void* dstBuffer, size_t buflen) const
{
    using namespace PeerDescriptionDetails;

    size_t totalSize = SerializedSize();
    DVASSERT(dstBuffer != NULL && totalSize <= buflen);
    if (false == (dstBuffer != NULL && totalSize <= buflen))
        return 0;

    SerializedHeader* header = static_cast<SerializedHeader*>(dstBuffer);
    header->magicZero = 0;
    header->totalSize = static_cast<uint32>(totalSize);
    header->checkSum = 0; // Compute later after serializing into buffer
    header->ifadrCount = static_cast<uint32>(ifaddr.size());
    header->networkRole = static_cast<uint32>(netConfig.Role());
    header->transpCount = static_cast<uint32>(netConfig.Transports().size());
    header->serviceCount = static_cast<uint32>(netConfig.Services().size());

    SerializedGeneralInfo* general = reinterpret_cast<SerializedGeneralInfo*>(header + 1);
    general->platfromType = static_cast<uint32>(platformType);
    general->gpuFamily = static_cast<uint32>(gpuFamily);

    strncpy(general->platform.data(), platform.c_str(), general->platform.size());
    general->platform[general->platform.size() - 1] = '\0';
    strncpy(general->version.data(), version.c_str(), general->version.size());
    general->version[general->version.size() - 1] = '\0';
    strncpy(general->manufacturer.data(), manufacturer.c_str(), general->manufacturer.size());
    general->manufacturer[general->manufacturer.size() - 1] = '\0';
    strncpy(general->model.data(), model.c_str(), general->model.size());
    general->model[general->model.size() - 1] = '\0';
    strncpy(general->udid.data(), udid.c_str(), general->udid.size());
    general->udid[general->udid.size() - 1] = '\0';
    strncpy(general->deviceName.data(), deviceName.c_str(), general->deviceName.size());
    general->deviceName[general->deviceName.size() - 1] = '\0';
    strncpy(general->appName.data(), appName.c_str(), general->appName.size());
    general->appName[general->appName.size() - 1] = '\0';

    SerializedIfAddress* ifa = reinterpret_cast<SerializedIfAddress*>(general + 1);
    for (size_t i = 0, n = ifaddr.size(); i < n; ++i)
    {
        ifa[i].addr = ifaddr[i].Address().ToUInt();
        ifa[i].mask = ifaddr[i].Mask().ToUInt();
        ifa[i].isInternal = ifaddr[i].IsInternal();
        Memcpy(ifa[i].physAddr, ifaddr[i].PhysicalAddress().data, 6);
    }

    SerializedTransport* tr = reinterpret_cast<SerializedTransport*>(ifa + ifaddr.size());
    for (size_t i = 0, n = netConfig.Transports().size(); i < n; ++i)
    {
        tr[i].type = static_cast<uint32>(netConfig.Transports()[i].type);
        tr[i].addr = netConfig.Transports()[i].endpoint.Address().ToUInt();
        tr[i].port = netConfig.Transports()[i].endpoint.Port();
    }

    uint32* serv = reinterpret_cast<uint32*>(tr + netConfig.Transports().size());
    for (size_t i = 0, n = netConfig.Services().size(); i < n; ++i)
    {
        serv[i] = netConfig.Services()[i];
    }
    // TODO: compute checksum
    return totalSize;
}

size_t PeerDescription::Deserialize(const void* srcBuffer, size_t buflen)
{
    using namespace PeerDescriptionDetails;

    DVASSERT(srcBuffer != NULL && buflen > sizeof(SerializedHeader));
    if (false == (srcBuffer != NULL && buflen > sizeof(SerializedHeader)))
        return 0;

    const SerializedHeader* header = static_cast<const SerializedHeader*>(srcBuffer);

    if (false == ValidateHeader(header, buflen))
    {
        return 0;
    }

    PeerDescription temp;
    temp.ifaddr.reserve(header->ifadrCount);
    temp.netConfig.SetRole(IntToNetworkRole(header->networkRole));

    const SerializedGeneralInfo* general = reinterpret_cast<const SerializedGeneralInfo*>(header + 1);
    temp.platformType = IntToPlatform(general->platfromType);
    temp.gpuFamily = IntToGPUFamily(general->gpuFamily);
    temp.platform = general->platform.data();
    temp.version = general->version.data();
    temp.manufacturer = general->manufacturer.data();
    temp.model = general->model.data();
    temp.udid = general->udid.data();
    temp.deviceName = general->deviceName.data();
    temp.appName = general->appName.data();

    const SerializedIfAddress* ifa = reinterpret_cast<const SerializedIfAddress*>(general + 1);
    for (uint32 i = 0; i < header->ifadrCount; ++i)
    {
        IfAddress::PhysAddress phys;
        Memcpy(phys.data, ifa[i].physAddr, 6);
        temp.ifaddr.push_back(IfAddress(ifa[i].isInternal != 0, phys, IPAddress(ifa[i].addr), IPAddress(ifa[i].mask)));
    }

    const SerializedTransport* tr = reinterpret_cast<const SerializedTransport*>(ifa + header->ifadrCount);
    for (uint32 i = 0; i < header->transpCount; ++i)
    {
        eTransportType type = IntToTransportType(tr[i].type);
        temp.netConfig.AddTransport(type, Endpoint(IPAddress(tr[i].addr), static_cast<uint16>(tr[i].port)));
    }

    const uint32* serv = reinterpret_cast<const uint32*>(tr + header->transpCount);
    for (uint32 i = 0; i < header->serviceCount; ++i)
    {
        temp.netConfig.AddService(serv[i]);
    }

    *this = std::move(temp);
    return header->totalSize;
}

bool PeerDescription::ExtractAppName(const void* srcBuffer, size_t buflen, String& appName)
{
    using namespace PeerDescriptionDetails;

    const SerializedHeader* header = static_cast<const SerializedHeader*>(srcBuffer);
    if (srcBuffer == nullptr
        || buflen < sizeof(SerializedHeader) + sizeof(SerializedGeneralInfo)
        || false == ValidateHeader(header, buflen))
    {
        return false;
    }

    const SerializedGeneralInfo* general = reinterpret_cast<const SerializedGeneralInfo*>(header + 1);
    appName = general->appName.data();

    return true;
}

} // namespace Net
} // namespace DAVA
