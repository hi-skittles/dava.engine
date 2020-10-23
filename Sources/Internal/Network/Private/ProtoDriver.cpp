#include <Functional/Function.h>
#include <Debug/DVAssert.h>
#include <Concurrency/Atomic.h>
#include <Concurrency/LockGuard.h>

#include <Network/Base/IOLoop.h>
#include <Network/ServiceRegistrar.h>

#include <Network/Private/ProtoDriver.h>

namespace DAVA
{
namespace Net
{
ProtoDriver::Channel::~Channel() = default;

ProtoDriver::ProtoDriver(IOLoop* aLoop, eNetworkRole aRole, const ServiceRegistrar& aRegistrar, void* aServiceContext)
    : loop(aLoop)
    , role(aRole)
    , registrar(aRegistrar)
    , serviceContext(aServiceContext)
    , transport(NULL)
    , whatIsSending()
    , pendingPong(false)
{
    DVASSERT(loop != NULL);
    Memset(&curPacket, 0, sizeof(curPacket));
}

ProtoDriver::~ProtoDriver()
{
    for (std::shared_ptr<Channel>& ch : channels)
    {
        ch->driver = nullptr;
    }
}

void ProtoDriver::SetTransport(IClientTransport* aTransport, const uint32* sourceChannels, size_t channelCount)
{
    DVASSERT(aTransport != NULL && sourceChannels != NULL && channelCount > 0);

    transport = aTransport;
    channels.reserve(channelCount);
    for (size_t i = 0; i < channelCount; ++i)
    {
        channels.push_back(std::make_shared<Channel>(sourceChannels[i], this));
    }
}

void ProtoDriver::SendData(uint32 channelId, const void* buffer, size_t length, uint32* outPacketId)
{
    DVASSERT(transport != NULL && buffer != NULL && length > 0);

    Packet packet;
    PreparePacket(&packet, channelId, buffer, length);
    if (outPacketId != NULL)
        *outPacketId = packet.packetId;

    // This method may be invoked from different threads
    if (true == senderLock.TryLock())
    {
        // TODO: consider optimization when called from IOLoop's thread
        curPacket = packet;
        loop->Post(MakeFunction(this, &ProtoDriver::SendCurPacket));
    }
    else
    {
        EnqueuePacket(&packet);
    }
}

void ProtoDriver::SendControl(uint32 code, uint32 channelId, uint32 packetId)
{
    ProtoHeader header;
    proto.EncodeControlFrame(&header, code, channelId, packetId);
    if (true == senderLock.TryLock()) // Control frame can be sent directly without queueing
    {
        curControl = header;
        SendCurControl();
    }
    else
    {
        // No need for mutex locking as control frames are always sent from handlers
        controlQueue.push_back(header);
    }
}

void ProtoDriver::ReleaseServices()
{
    for (std::shared_ptr<Channel>& channel : channels)
    {
        if (channel->service != nullptr)
        {
            registrar.Delete(channel->channelId, channel->service, serviceContext);
            channel->service = nullptr;
        }
    }
}

void ProtoDriver::OnConnected(const Endpoint& endp)
{
    if (SERVER_ROLE == role)
    {
        // In SERVER_ROLE only setup remote endpoints
        for (std::shared_ptr<Channel>& channel : channels)
        {
            channel->remoteEndpoint = endp;
        }
    }
    else
    {
        // In CLIENT_ROLE ask server for services
        for (std::shared_ptr<Channel>& channel : channels)
        {
            channel->remoteEndpoint = endp;
            channel->service = registrar.Create(channel->channelId, serviceContext);
            if (channel->service != nullptr)
            {
                SendControl(TYPE_CHANNEL_QUERY, channel->channelId, 0);
            }
        }
    }
}

void ProtoDriver::OnDisconnected(const char* message)
{
    for (std::shared_ptr<Channel>& channel : channels)
    {
        if (channel->service != nullptr && true == channel->confirmed)
        {
            channel->confirmed = false;
            channel->service->OnChannelClosed(channel, message);
        }
    }
    ClearQueues();
}

bool ProtoDriver::OnDataReceived(const void* buffer, size_t length)
{
    bool canContinue = true;
    ProtoDecoder::DecodeResult result;
    ProtoDecoder::eDecodeStatus status = ProtoDecoder::DECODE_INVALID;
    pendingPong = false;
    do
    {
        status = proto.Decode(buffer, length, &result);
        if (ProtoDecoder::DECODE_OK == status)
        {
            switch (result.type)
            {
            case TYPE_DATA:
                canContinue = ProcessDataPacket(&result);
                break;
            case TYPE_CHANNEL_QUERY:
                canContinue = ProcessChannelQuery(&result);
                break;
            case TYPE_CHANNEL_ALLOW:
                canContinue = ProcessChannelAllow(&result);
                break;
            case TYPE_CHANNEL_DENY:
                canContinue = ProcessChannelDeny(&result);
                break;
            case TYPE_PING:
                SendControl(TYPE_PONG, 0, 0);
                canContinue = true;
                break;
            case TYPE_PONG:
                // Do nothing as some data have been already arrived
                canContinue = true;
                break;
            case TYPE_DELIVERY_ACK:
                canContinue = ProcessDeliveryAck(&result);
                break;
            }
        }
        DVASSERT(length >= result.decodedSize);
        length -= result.decodedSize;
        buffer = static_cast<const uint8*>(buffer) + result.decodedSize;
    } while (status != ProtoDecoder::DECODE_INVALID && true == canContinue && length > 0);
    canContinue = canContinue && (status != ProtoDecoder::DECODE_INVALID);
    return canContinue;
}

void ProtoDriver::OnSendComplete()
{
    if (SENDING_DATA_FRAME == whatIsSending)
    {
        curPacket.sentLength += curPacket.chunkLength;
        if (curPacket.sentLength == curPacket.dataLength)
        {
            std::shared_ptr<Channel> ch = GetChannel(curPacket.channelId);
            ch->service->OnPacketSent(ch, curPacket.data, curPacket.dataLength);
            curPacket.data = NULL;
        }
    }

    if (true == DequeueControl(&curControl)) // First send control packets if any
    {
        SendCurControl();
    }
    else if (curPacket.data != NULL || true == DequeuePacket(&curPacket)) // Send current packet further or send new packet
    {
        SendCurPacket();
    }
    else
    {
        senderLock.Unlock(); // Nothing to send, unlock sender
    }
}

bool ProtoDriver::OnTimeout()
{
    if (false == pendingPong)
    {
        pendingPong = true;
        SendControl(TYPE_PING, 0, 0);
        return true;
    }
    return false;
}

bool ProtoDriver::ProcessDataPacket(ProtoDecoder::DecodeResult* result)
{
    std::shared_ptr<Channel> ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        // Send back delivery confirmation
        SendControl(TYPE_DELIVERY_ACK, result->channelId, result->packetId);
        ch->service->OnPacketReceived(ch, result->data, result->dataSize);
        return true;
    }
    DVASSERT(0);
    return false;
}

bool ProtoDriver::ProcessChannelQuery(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(SERVER_ROLE == role);

    std::shared_ptr<Channel> ch = GetChannel(result->channelId);
    if (ch != NULL)
    {
        DVASSERT(NULL == ch->service);
        if (NULL == ch->service)
        {
            ch->service = registrar.Create(ch->channelId, serviceContext);
            uint32 code = ch->service != NULL ? TYPE_CHANNEL_ALLOW
                                                :
                                                TYPE_CHANNEL_DENY;
            SendControl(code, result->channelId, 0);
            if (ch->service != NULL)
            {
                ch->confirmed = true;
                ch->service->OnChannelOpen(ch);
            }
            return true;
        }
        return false;
    }
    return true; // Nothing strange that queried channel is not found
}

bool ProtoDriver::ProcessChannelAllow(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(CLIENT_ROLE == role);

    std::shared_ptr<Channel> ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        ch->confirmed = true;
        ch->service->OnChannelOpen(ch);
        return true;
    }
    DVASSERT(ch != NULL);
    DVASSERT(ch->service != NULL);
    return false;
}

bool ProtoDriver::ProcessChannelDeny(ProtoDecoder::DecodeResult* result)
{
    DVASSERT(CLIENT_ROLE == role);

    std::shared_ptr<Channel> ch = GetChannel(result->channelId);
    if (ch != NULL && ch->service != NULL)
    {
        ch->service->OnChannelClosed(ch, "Remote service is unavailable");
        return true;
    }
    DVASSERT(ch != NULL);
    DVASSERT(ch->service != NULL);
    return false;
}

bool ProtoDriver::ProcessDeliveryAck(ProtoDecoder::DecodeResult* result)
{
    std::shared_ptr<Channel> ch = GetChannel(result->channelId);
    DVASSERT(ch != NULL && ch->service != NULL);
    DVASSERT(false == pendingAckQueue.empty());
    if (ch != NULL && ch->service != NULL && false == pendingAckQueue.empty())
    {
        uint32 pendingId = pendingAckQueue.front();
        pendingAckQueue.pop_front();
        DVASSERT(pendingId == result->packetId);
        if (pendingId == result->packetId)
        {
            ch->service->OnPacketDelivered(ch, pendingId);
            return true;
        }
    }
    return false;
}

void ProtoDriver::ClearQueues()
{
    if (curPacket.data != NULL)
    {
        std::shared_ptr<Channel> ch = GetChannel(curPacket.channelId);
        ch->service->OnPacketSent(ch, curPacket.data, curPacket.dataLength);

        curPacket.data = NULL;
    }
    for (Deque<Packet>::iterator i = dataQueue.begin(), e = dataQueue.end(); i != e; ++i)
    {
        Packet& packet = *i;
        std::shared_ptr<Channel> ch = GetChannel(packet.channelId);
        ch->service->OnPacketSent(ch, packet.data, packet.dataLength);
    }
    dataQueue.clear();
    pendingAckQueue.clear();
    controlQueue.clear();
    senderLock.Unlock();
}

void ProtoDriver::SendCurPacket()
{
    DVASSERT(curPacket.sentLength < curPacket.dataLength);

    whatIsSending = SENDING_DATA_FRAME;
    curPacket.chunkLength = proto.EncodeDataFrame(&header, curPacket.channelId, curPacket.packetId, curPacket.dataLength, curPacket.sentLength);

    Buffer buffers[2];
    buffers[0] = CreateBuffer(&header);
    buffers[1] = CreateBuffer(curPacket.data + curPacket.sentLength, curPacket.chunkLength);
    if (0 == transport->Send(buffers, 2) && 0 == curPacket.sentLength)
    {
        pendingAckQueue.push_back(curPacket.packetId);
    }
}

void ProtoDriver::SendCurControl()
{
    whatIsSending = SENDING_CONTROL_FRAME;

    Buffer buffer = CreateBuffer(&curControl);
    transport->Send(&buffer, 1);
}

void ProtoDriver::PreparePacket(Packet* packet, uint32 channelId, const void* buffer, size_t length)
{
    static Atomic<uint32> nextPacketId{ 0 };

    DVASSERT(buffer != NULL && length > 0);

    packet->channelId = channelId;
    packet->packetId = ++nextPacketId;
    packet->dataLength = length;
    packet->sentLength = 0;
    packet->chunkLength = 0;
    packet->data = static_cast<uint8*>(const_cast<void*>(buffer));
}

bool ProtoDriver::EnqueuePacket(Packet* packet)
{
    bool queueWasEmpty = false;

    LockGuard<Mutex> lock(queueMutex);
    queueWasEmpty = dataQueue.empty();
    dataQueue.push_back(*packet);
    return queueWasEmpty;
}

bool ProtoDriver::DequeuePacket(Packet* dest)
{
    LockGuard<Mutex> lock(queueMutex);
    if (false == dataQueue.empty())
    {
        *dest = dataQueue.front();
        dataQueue.pop_front();
        return true;
    }
    return false;
}

bool ProtoDriver::DequeueControl(ProtoHeader* dest)
{
    // No need for mutex locking as control packets are always dequeued from handler
    if (false == controlQueue.empty())
    {
        *dest = controlQueue.front();
        controlQueue.pop_front();
        return true;
    }
    return false;
}

} // namespace Net
} // namespace DAVA
