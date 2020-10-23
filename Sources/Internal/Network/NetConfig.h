#ifndef __DAVAENGINE_NETCONFIG_H__
#define __DAVAENGINE_NETCONFIG_H__

#include <Base/BaseTypes.h>

#include <Network/Base/Endpoint.h>
#include <Network/NetworkCommon.h>

namespace DAVA
{
namespace Net
{
class NetConfig
{
public:
    struct TransportConfig
    {
        TransportConfig();
        TransportConfig(eTransportType aType, const Endpoint& aEndpoint);

        friend bool operator==(const TransportConfig& left, const TransportConfig& right);

        eTransportType type;
        Endpoint endpoint;
    };

public:
    NetConfig();
    NetConfig(eNetworkRole aRole);

    bool Validate() const;
    NetConfig Mirror(const IPAddress& addr) const;

    void SetRole(eNetworkRole aRole);
    bool AddTransport(eTransportType type, const Endpoint& endpoint);
    bool AddService(uint32 serviceId);

    eNetworkRole Role() const
    {
        return role;
    }

    const Vector<TransportConfig>& Transports() const
    {
        return transports;
    }
    const Vector<uint32>& Services() const
    {
        return services;
    }

private:
    eNetworkRole role;
    Vector<TransportConfig> transports;
    Vector<uint32> services;
};

//////////////////////////////////////////////////////////////////////////
inline NetConfig::TransportConfig::TransportConfig()
    : type()
    , endpoint()
{
}

inline NetConfig::TransportConfig::TransportConfig(eTransportType aType, const Endpoint& aEndpoint)
    : type(aType)
    , endpoint(aEndpoint)
{
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_NETCONFIG_H__
