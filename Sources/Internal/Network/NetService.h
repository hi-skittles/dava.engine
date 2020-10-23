#pragma once

#include "Base/BaseTypes.h"
#include "Network/IChannel.h"

namespace DAVA
{
namespace Net
{
class NetService : public IChannelListener
{
public:
    NetService();
    virtual ~NetService()
    {
    }

    // IChannelListener
    void OnChannelOpen(const std::shared_ptr<IChannel>& channel) override;
    void OnChannelClosed(const std::shared_ptr<IChannel>& channel, const char8* message) override;
    void OnPacketReceived(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override;
    void OnPacketSent(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override;
    void OnPacketDelivered(const std::shared_ptr<IChannel>& channel, uint32 packetId) override;

    virtual void ChannelOpen()
    {
    }
    virtual void ChannelClosed(const char8* message)
    {
    }
    virtual void PacketReceived(const void* packet, size_t length)
    {
    }
    virtual void PacketSent()
    {
    }
    virtual void PacketDelivered()
    {
    }

    bool IsChannelOpen() const;

protected:
    bool Send(const void* data, size_t length, uint32* packetId = NULL);
    template <typename T>
    bool Send(const T* value, uint32* packetId = NULL);

protected:
    std::shared_ptr<IChannel> channel;
};

//////////////////////////////////////////////////////////////////////////
inline NetService::NetService()
    : channel(NULL)
{
}

inline bool NetService::IsChannelOpen() const
{
    return channel != NULL;
}

template <typename T>
inline bool NetService::Send(const T* value, uint32* packetId)
{
    return Send(value, sizeof(T), packetId);
}

} // namespace Net
} // namespace DAVA
