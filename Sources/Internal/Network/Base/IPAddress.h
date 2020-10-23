#ifndef __DAVAENGINE_IPADDRESS_H__
#define __DAVAENGINE_IPADDRESS_H__

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

namespace DAVA
{
namespace Net
{
/*
 Class IPAddress represents IPv4 address.
 IPAddress can be constructed from 32-bit integer or from string.
 Requirements on string IPAddress should be constructed from:
    - must be in the form XXX.XXX.XXX.XXX with no more than 3 decimal digits in a group
    - each group can contain only decimal numbers from 0 to 255 with no starting zeros
 Also address can be converted to string.

 IPAddress accepts and returns numeric parameters in host byte order
*/
class IPAddress
{
public:
    IPAddress(uint32 address = 0);
    IPAddress(const char8* address);

    uint32 ToUInt() const;

    bool IsUnspecified() const;
    bool IsMulticast() const;

    bool ToString(char8* buffer, size_t size) const;
    String ToString() const;

    static IPAddress FromString(const char8* addr);
    static IPAddress FromString(const String& addr);

    friend bool operator==(const IPAddress& left, const IPAddress& right);

private:
    uint32 addr;
};

//////////////////////////////////////////////////////////////////////////
inline bool IPAddress::IsUnspecified() const
{
    return 0 == addr;
}

inline bool IPAddress::IsMulticast() const
{
    return 0xE0000000 == (ToUInt() & 0xF0000000);
}

inline bool operator==(const IPAddress& left, const IPAddress& right)
{
    return left.addr == right.addr;
}

inline bool operator<(const IPAddress& left, const IPAddress& right)
{
    return left.ToUInt() < right.ToUInt();
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_IPADDRESS_H__
