#ifndef __DAVAENGINE_IFADDRESS_H__
#define __DAVAENGINE_IFADDRESS_H__

#include <Base/BaseTypes.h>

#include <Network/Base/IPAddress.h>

namespace DAVA
{
namespace Net
{
/*
 Class IfAddress represents network interface address.
 Also it provides static member for retrieving addresses of all installed interfaces
*/
class IfAddress
{
public:
    struct PhysAddress
    {
        uint8 data[6];
    };

public:
    IfAddress();
    IfAddress(bool isInternal_, const PhysAddress& physAddr_, const IPAddress& addr_, const IPAddress& mask_);

    bool IsInternal() const;
    const PhysAddress& PhysicalAddress() const;
    const IPAddress& Address() const;
    const IPAddress& Mask() const;

    static Vector<IfAddress> GetInstalledInterfaces(bool withInternal = false);

    friend bool operator<(const IfAddress& left, const IfAddress& right);

private:
    bool isInternal;
    PhysAddress physAddr;
    IPAddress addr;
    IPAddress mask;
};

//////////////////////////////////////////////////////////////////////////
inline IfAddress::IfAddress()
    : isInternal(true)
{
    Memset(&physAddr, 0, sizeof(PhysAddress));
}

inline IfAddress::IfAddress(bool isInternal_, const PhysAddress& physAddr_, const IPAddress& addr_, const IPAddress& mask_)
    : isInternal(isInternal_)
    , physAddr(physAddr_)
    , addr(addr_)
    , mask(mask_)
{
}

inline bool IfAddress::IsInternal() const
{
    return isInternal;
}

inline const IfAddress::PhysAddress& IfAddress::PhysicalAddress() const
{
    return physAddr;
}

inline const IPAddress& IfAddress::Address() const
{
    return addr;
}

inline const IPAddress& IfAddress::Mask() const
{
    return mask;
}

inline bool operator<(const IfAddress& left, const IfAddress& right)
{
    // Order by address, internal interfaces moves to back
    return left.isInternal == right.isInternal ? left.addr < right.addr
                                                 :
                                                 left.isInternal == right.isInternal;
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_IFADDRESS_H__
