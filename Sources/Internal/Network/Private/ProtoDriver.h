#ifndef __DAVAENGINE_PROTODRIVER_H__
#define __DAVAENGINE_PROTODRIVER_H__

#include <Base/BaseTypes.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/Spinlock.h>

#include <Network/Base/Endpoint.h>
#include <Network/NetworkCommon.h>
#include <Network/IChannel.h>

#include <Network/Private/ITransport.h>
#include <Network/Private/ProtoDecoder.h>

namespace DAVA
{
namespace Net
{
class IOLoop;
class ServiceRegistrar;

class ProtoDriver
{
private:
    struct Packet
    {
        uint32 channelId;
        uint32 packetId;
        uint8* data = nullptr; // Data
        size_t dataLength; //  and its length
        size_t sentLength; // Number of bytes that have been already transfered
        size_t chunkLength; // Number of bytes transfered during last operation
    };

    struct Channel : public IChannel
    {
        Channel(uint32 id, ProtoDriver* driver);
        ~Channel() override;

        bool Send(const void* data, size_t length, uint32 flags, uint32* packetId) override;
        const Endpoint& RemoteEndpoint() const override;

        bool confirmed; // Channel is confirmed by other side
        uint32 channelId;
        Endpoint remoteEndpoint;
        ProtoDriver* driver = nullptr;
        IChannelListener* service = nullptr;
    };

    enum eSendingFrameType
    {
        SENDING_DATA_FRAME = false,
        SENDING_CONTROL_FRAME = true
    };

public:
    ProtoDriver(IOLoop* aLoop, eNetworkRole aRole, const ServiceRegistrar& aRegistrar, void* aServiceContext);
    ~ProtoDriver();

    void SetTransport(IClientTransport* aTransport, const uint32* sourceChannels, size_t channelCount);
    void SendData(uint32 channelId, const void* buffer, size_t length, uint32* outPacketId);

    void ReleaseServices();

    void OnConnected(const Endpoint& endp);
    void OnDisconnected(const char* message);
    bool OnDataReceived(const void* buffer, size_t length);
    void OnSendComplete();
    bool OnTimeout();

private:
    std::shared_ptr<Channel>& GetChannel(uint32 channelId);
    void SendControl(uint32 code, uint32 channelId, uint32 packetId);

    bool ProcessDataPacket(ProtoDecoder::DecodeResult* result);
    bool ProcessChannelQuery(ProtoDecoder::DecodeResult* result);
    bool ProcessChannelAllow(ProtoDecoder::DecodeResult* result);
    bool ProcessChannelDeny(ProtoDecoder::DecodeResult* result);
    bool ProcessDeliveryAck(ProtoDecoder::DecodeResult* result);

    void ClearQueues();

    void SendCurPacket();
    void SendCurControl();

    void PreparePacket(Packet* packet, uint32 channelId, const void* buffer, size_t length);
    bool EnqueuePacket(Packet* packet);
    bool DequeuePacket(Packet* dest);
    bool DequeueControl(ProtoHeader* dest);

private:
    IOLoop* loop = nullptr;
    eNetworkRole role;
    const ServiceRegistrar& registrar;
    void* serviceContext = nullptr;
    IClientTransport* transport = nullptr;
    Vector<std::shared_ptr<Channel>> channels;

    Spinlock senderLock;
    Mutex queueMutex;
    eSendingFrameType whatIsSending;
    bool pendingPong;

    Packet curPacket;
    Deque<Packet> dataQueue;
    Deque<uint32> pendingAckQueue;

    ProtoHeader curControl;
    Deque<ProtoHeader> controlQueue;

    ProtoDecoder proto;
    ProtoHeader header;
};

//////////////////////////////////////////////////////////////////////////
inline ProtoDriver::Channel::Channel(uint32 id, ProtoDriver* aDriver)
    : confirmed(false)
    , channelId(id)
    , driver(aDriver)
    , service(NULL)
{
}

inline bool ProtoDriver::Channel::Send(const void* data, size_t length, uint32 flags, uint32* outPacketId)
{
    if (driver != nullptr)
    {
        driver->SendData(channelId, data, length, outPacketId);
    }
    return true;
}

inline const Endpoint& ProtoDriver::Channel::RemoteEndpoint() const
{
    return remoteEndpoint;
}

inline std::shared_ptr<ProtoDriver::Channel>& ProtoDriver::GetChannel(uint32 channelId)
{
    for (std::shared_ptr<ProtoDriver::Channel>& channel : channels)
    {
        if (channel->channelId == channelId)
        {
            return channel;
        }
    }

    static std::shared_ptr<ProtoDriver::Channel> empty;
    return empty;
}

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_PROTODRIVER_H__
