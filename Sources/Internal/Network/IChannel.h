#pragma once

#include "Base/BaseTypes.h"
#include "Network/NetworkCommon.h"

#include <memory>

namespace DAVA
{
namespace Net
{
/*
 Interface should be implemented by objects which want to know what is going on in channel.
*/
struct IChannel;
struct IChannelListener
{
    // There should be a virtual destructor defined as objects may be deleted through this interface
    virtual ~IChannelListener();

    // Channel is open (underlying transport has connection) and can receive and send data through IChannel interface
    virtual void OnChannelOpen(const std::shared_ptr<IChannel>& channel) = 0;
    // Channel is closed (underlying transport has disconnected) with reason
    virtual void OnChannelClosed(const std::shared_ptr<IChannel>& channel, const char8* message) = 0;
    // Some data arrived into channel
    virtual void OnPacketReceived(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) = 0;
    // Buffer has been sent and can be reused or freed
    virtual void OnPacketSent(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) = 0;
    // Data packet with given ID has been delivered to other side
    virtual void OnPacketDelivered(const std::shared_ptr<IChannel>& channel, uint32 packetId) = 0;
};

/*
 This interface is passed to IChannelListener methods to allow clients to send data to channels.
*/
class Endpoint;
struct IChannel
{
    // There should be a virtual destructor defined as objects may be deleted through this interface
    virtual ~IChannel();

    virtual bool Send(const void* data, size_t length, uint32 flags, uint32* packetId) = 0;
    virtual const Endpoint& RemoteEndpoint() const = 0;
};

} // namespace Net
} // namespace DAVA
