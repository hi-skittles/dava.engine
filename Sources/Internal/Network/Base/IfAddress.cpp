#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

#include "Network/Base/Endpoint.h"
#include "Network/Base/IfAddress.h"

namespace DAVA
{
namespace Net
{
Vector<IfAddress> IfAddress::GetInstalledInterfaces(bool withInternal)
{
    Vector<IfAddress> result;

#if !defined(DAVA_NETWORK_DISABLE)
    int n = 0;
    uv_interface_address_t* ifaddr = NULL;
    int error = uv_interface_addresses(&ifaddr, &n);
    if (0 == error)
    {
        result.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            if (ifaddr[i].address.address4.sin_family != AF_INET)
                continue; // For now list only IPv4 addresses

            if (true == Endpoint(&ifaddr[i].address.address4).Address().IsUnspecified())
                continue; // List only interfaces with specified IP-address

            if (false == withInternal && ifaddr[i].is_internal != 0)
                continue; // Do not list internal interfaces

            PhysAddress physAddr;
            Endpoint addr(&ifaddr[i].address.address4);
            Endpoint mask(&ifaddr[i].netmask.netmask4);
            bool isInternal = ifaddr[i].is_internal != 0;
            Memcpy(physAddr.data, ifaddr[i].phys_addr, 6);

            result.push_back(IfAddress(isInternal, physAddr, addr.Address(), mask.Address()));
        }
        uv_free_interface_addresses(ifaddr, n);
    }
#endif
    return result;
}

} // namespace Net
} // namespace DAVA
