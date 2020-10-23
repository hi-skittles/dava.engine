#include "Debug/DVAssert.h"

#include "Network/Base/Endpoint.h"
#include "Network/Base/IPAddress.h"

namespace DAVA
{
namespace Net
{
IPAddress::IPAddress(uint32 address)
    : addr(htonl(address))
{
}

IPAddress::IPAddress(const char8* address)
    : addr(0)
{
    DVASSERT(address != NULL);
    *this = FromString(address);
}

uint32 IPAddress::ToUInt() const
{
    return ntohl(addr);
}

bool IPAddress::ToString(char8* buffer, size_t size) const
{
    DVASSERT(buffer != NULL && size > 0);
    return 0 == uv_ip4_name(Endpoint(*this, 0).CastToSockaddrIn(), buffer, size);
}

String IPAddress::ToString() const
{
    char8 buf[20]; // This should be enough for IPv4 address
    bool ret = ToString(buf, 20);
    DVASSERT(ret);
    return String(ret ? buf : "");
}

IPAddress IPAddress::FromString(const char8* addr)
{
    DVASSERT(addr != NULL);

    Endpoint endp;
    
#if !defined(DAVA_NETWORK_DISABLE)
    if (0 == uv_ip4_addr(addr, 0, endp.CastToSockaddrIn()))
        return endp.Address();
#endif
    return IPAddress();
}

IPAddress IPAddress::FromString(const String& addr)
{
    return FromString(addr.c_str());
}

} // namespace Net
} // namespace DAVA
