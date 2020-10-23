#include <algorithm>

#include <Debug/DVAssert.h>

#include <Network/NetConfig.h>

namespace DAVA
{
namespace Net
{
bool operator==(const NetConfig::TransportConfig& left, const NetConfig::TransportConfig& right)
{
    return left.type == right.type && left.endpoint == right.endpoint;
}

NetConfig::NetConfig()
    : role()
{
}

NetConfig::NetConfig(eNetworkRole aRole)
    : role(aRole)
{
}

bool NetConfig::Validate() const
{
    return false == transports.empty() && false == services.empty();
}

NetConfig NetConfig::Mirror(const IPAddress& addr) const
{
    DVASSERT(true == Validate());
    NetConfig result(SERVER_ROLE == role ? CLIENT_ROLE : SERVER_ROLE);
    result.transports = transports;
    result.services = services;
    for (Vector<TransportConfig>::iterator i = result.transports.begin(), e = result.transports.end(); i != e; ++i)
    {
        uint16 port = (*i).endpoint.Port();
        (*i).endpoint = Endpoint(addr, port);
    }
    return result;
}

void NetConfig::SetRole(eNetworkRole aRole)
{
    DVASSERT(true == transports.empty() && true == services.empty());
    role = aRole;
}

bool NetConfig::AddTransport(eTransportType type, const Endpoint& endpoint)
{
    TransportConfig config(type, endpoint);
    DVASSERT(std::find(transports.begin(), transports.end(), config) == transports.end());
    if (std::find(transports.begin(), transports.end(), config) == transports.end())
    {
        transports.push_back(config);
        return true;
    }
    return false;
}

bool NetConfig::AddService(uint32 serviceId)
{
    DVASSERT(std::find(services.begin(), services.end(), serviceId) == services.end());
    if (std::find(services.begin(), services.end(), serviceId) == services.end())
    {
        services.push_back(serviceId);
        return true;
    }
    return false;
}

} // namespace Net
} // namespace DAVA
