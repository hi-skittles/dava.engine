#ifndef __DAVAENGINE_NETWORKCOMMON_H__
#define __DAVAENGINE_NETWORKCOMMON_H__

namespace DAVA
{
namespace Net
{
// Transport types
enum eTransportType
{
    TRANSPORT_TCP // Transport based on TCP
};

// Role of network objects
enum eNetworkRole
{
    SERVER_ROLE,
    CLIENT_ROLE
};

enum eMiscConst
{
#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    DEFAULT_READ_TIMEOUT = 5 * 1000, // Timeout in ms
#else
    // Increase read timeout when memory profiling enabled to reduce connection breaks on timeout
    DEFAULT_READ_TIMEOUT = 120 * 1000,
#endif
    DEFAULT_ANNOUNCE_TIME_PERIOD_SEC = 5
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_NETWORKCOMMON_H__
