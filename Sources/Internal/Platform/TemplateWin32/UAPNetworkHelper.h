#ifndef __DAVAENGINE_UAP_NETWORK_HELPER_H__
#define __DAVAENGINE_UAP_NETWORK_HELPER_H__

#include "Network/Base/Endpoint.h"
#include "Network/NetworkCommon.h"

namespace DAVA
{
class UAPNetworkHelper
{
public:
    static const uint16 UAP_DESKTOP_TCP_PORT = 7777;
    static const uint16 UAP_MOBILE_TCP_PORT = 1911;
    static const char* UAP_IP_ADDRESS;

    static Net::eNetworkRole GetCurrentNetworkRole();
    static Net::Endpoint GetCurrentEndPoint();
    static Net::Endpoint GetEndPoint(Net::eNetworkRole role);
};

} // namespace DAVA

#endif // __DAVAENGINE_UAP_NETWORK_HELPER_H__
