#include <Debug/DVAssert.h>

#include "Network/NetService.h"

namespace DAVA
{
namespace Net
{
IChannelListener::~IChannelListener() = default;
IChannel::~IChannel() = default;

void NetService::OnChannelOpen(const std::shared_ptr<IChannel>& channel_)
{
    DVASSERT(NULL == channel);
    channel = channel_;
    ChannelOpen();
}

void NetService::OnChannelClosed(const std::shared_ptr<IChannel>& channel_, const char8* message)
{
    // OnChannelClosed can be called without corresponding OnChannelOpen, e.g. when remote service is unavailable
    DVASSERT(NULL == channel || channel == channel_);
    channel = NULL;
    ChannelClosed(message);
}

void NetService::OnPacketReceived(const std::shared_ptr<IChannel>& channel_, const void* buffer, size_t length)
{
    DVASSERT(channel == channel_);
    PacketReceived(buffer, length);
}

void NetService::OnPacketSent(const std::shared_ptr<IChannel>& channel_, const void* buffer, size_t length)
{
    // If channel is NULL then OnChannelClosed has been called already
    DVASSERT(NULL == channel || channel == channel_);
    PacketSent();
}

void NetService::OnPacketDelivered(const std::shared_ptr<IChannel>& channel_, uint32 packetId)
{
    DVASSERT(channel == channel_);
    PacketDelivered();
}

bool NetService::Send(const void* data, size_t length, uint32* packetId)
{
    DVASSERT(data != NULL && length > 0 && true == IsChannelOpen());
    return IsChannelOpen() ? channel->Send(data, length, 0, packetId)
                             :
                             false;
}

} // namespace Net
} // namespace DAVA
