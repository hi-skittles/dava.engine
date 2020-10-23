#include <cstdio>

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

#include "Debug/DVAssert.h"
#include "Network/Base/Endpoint.h"

namespace DAVA
{
namespace Net
{
Endpoint::Endpoint(const char8* address, uint16 port)
    : data()
{
    InitSockaddrIn(IPAddress::FromString(address).ToUInt(), port);
}

Endpoint::Endpoint(const sockaddr* sa)
{
    DVASSERT(sa);
    Memcpy(&data, sa, sizeof(data));
}

Endpoint::Endpoint(const sockaddr_in* sin)
{
    DVASSERT(sin);
    Memcpy(&data, sin, sizeof(data));
}

uint16 Endpoint::Port() const
{
    return ntohs(data.sin_port);
}

bool Endpoint::ToString(char8* buffer, size_t size) const
{
    DVASSERT(buffer != NULL && size > 0);
    Array<char8, 20> addr;
    if (Address().ToString(addr.data(), addr.size()))
    {
        // TODO: Snprintf on Win32 do not conform standard
        Snprintf(buffer, size, "%s:%hu", addr.data(), Port());
        return true;
    }
    return false;
}

String Endpoint::ToString() const
{
    Array<char8, 50> buf;
    if (ToString(buf.data(), buf.size()))
        return String(buf.data());
    return String();
}

void Endpoint::InitSockaddrIn(uint32 addr, uint16 port)
{
    data.sin_family = AF_INET;
    data.sin_port = htons(port);
#ifdef __DAVAENGINE_WINDOWS__
    data.sin_addr.S_un.S_addr = htonl(addr);
#else // __DAVAENGINE_WINDOWS__
    data.sin_addr.s_addr = htonl(addr);
#endif // __DAVAENGINE_WINDOWS__
}

uint32 Endpoint::GetSockaddrAddr() const
{
#ifdef __DAVAENGINE_WINDOWS__
    return ntohl(data.sin_addr.S_un.S_addr);
#else // __DAVAENGINE_WINDOWS__
    return ntohl(data.sin_addr.s_addr);
#endif // __DAVAENGINE_WINDOWS__
}

} // namespace Net
} // namespace DAVA
