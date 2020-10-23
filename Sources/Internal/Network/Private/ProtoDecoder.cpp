#include <Debug/DVAssert.h>

#include <Network/Private/ProtoDecoder.h>

namespace DAVA
{
namespace Net
{
ProtoDecoder::ProtoDecoder()
    : totalDataSize(0)
    , accumulatedSize(0)
    , curFrameSize(0)
{
}

ProtoDecoder::eDecodeStatus ProtoDecoder::Decode(const void* buffer, size_t length, DecodeResult* result)
{
    DVASSERT(buffer != NULL && result != NULL);

    Memset(result, 0, sizeof(DecodeResult));
    eDecodeStatus status = GatherHeader(buffer, length, result);
    if (DECODE_OK == status)
    {
        status = GatherFrame(static_cast<const uint8*>(buffer) + result->decodedSize, length - result->decodedSize, result);
        if (DECODE_OK == status)
        {
            ProtoHeader* header = reinterpret_cast<ProtoHeader*>(curFrame);
            status = TYPE_DATA == header->frameType ? ProcessDataFrame(header, result)
                                                      :
                                                      ProcessControlFrame(header, result);
            curFrameSize = 0;
        }
    }
    return status;
}

size_t ProtoDecoder::EncodeDataFrame(ProtoHeader* header, uint32 channelId, uint32 packetId, size_t packetSize, size_t encodedSize) const
{
    DVASSERT(header != NULL && packetSize > 0 && encodedSize < packetSize);

    // Compute size of user data that can fit in one frame
    size_t sizeToEncode = Min(packetSize - encodedSize, PROTO_MAX_FRAME_DATA_SIZE);

    header->frameSize = static_cast<uint16>(sizeof(ProtoHeader) + sizeToEncode);
    header->frameType = TYPE_DATA;
    header->channelId = channelId;
    header->packetId = packetId;
    header->totalSize = static_cast<uint32>(packetSize);
    return sizeToEncode;
}

size_t ProtoDecoder::EncodeControlFrame(ProtoHeader* header, uint32 type, uint32 channelId, uint32 packetId) const
{
    DVASSERT(header != NULL && TYPE_CONTROL_FIRST <= type && type <= TYPE_LAST);
    header->frameSize = sizeof(ProtoHeader);
    header->frameType = type;
    header->channelId = 0;
    header->packetId = 0;
    header->totalSize = 0;
    switch (type)
    {
    case TYPE_CHANNEL_QUERY:
    case TYPE_CHANNEL_ALLOW:
    case TYPE_CHANNEL_DENY:
        header->channelId = channelId;
        break;
    case TYPE_DELIVERY_ACK:
        header->channelId = channelId;
        header->packetId = packetId;
        break;
    }
    return sizeof(ProtoHeader);
}

ProtoDecoder::eDecodeStatus ProtoDecoder::ProcessDataFrame(ProtoHeader* header, DecodeResult* result)
{
    if (0 == totalDataSize)
    {
        accumulatedSize = 0;
        totalDataSize = static_cast<size_t>(header->totalSize);
        if (accum.size() < totalDataSize)
            accum.resize(totalDataSize);
    }
    // TODO: maybe I should compare channel ID and packet ID with initial values
    DVASSERT(curFrameSize >= sizeof(ProtoHeader));
    size_type packetSize = curFrameSize - sizeof(ProtoHeader);
    DVASSERT(accum.size() >= accumulatedSize + packetSize);
    Memcpy(&*accum.begin() + accumulatedSize, curFrame + sizeof(ProtoHeader), packetSize);
    accumulatedSize += packetSize;
    if (accumulatedSize == totalDataSize)
    {
        result->type = TYPE_DATA;
        result->channelId = header->channelId;
        result->packetId = header->packetId;
        result->dataSize = totalDataSize;
        result->data = &*accum.begin();

        totalDataSize = 0;
        return DECODE_OK;
    }
    return DECODE_INCOMPLETE;
}

ProtoDecoder::eDecodeStatus ProtoDecoder::ProcessControlFrame(ProtoHeader* header, DecodeResult* result)
{
    result->type = header->frameType;
    switch (header->frameType)
    {
    case TYPE_CHANNEL_QUERY:
        result->channelId = header->channelId;
        break;
    case TYPE_CHANNEL_ALLOW:
        result->channelId = header->channelId;
        break;
    case TYPE_CHANNEL_DENY:
        result->channelId = header->channelId;
        break;
    case TYPE_PING:
        break;
    case TYPE_PONG:
        break;
    case TYPE_DELIVERY_ACK:
        result->channelId = header->channelId;
        result->packetId = header->packetId;
        break;
    }
    // Always return DECODE_OK as frame type has been checked while gathering header
    return DECODE_OK;
}

ProtoDecoder::eDecodeStatus ProtoDecoder::GatherHeader(const void* buffer, size_t length, DecodeResult* result)
{
    if (curFrameSize < sizeof(ProtoHeader))
    {
        size_t n = Min(sizeof(ProtoHeader) - curFrameSize, length);
        Memcpy(curFrame + curFrameSize, buffer, n);
        curFrameSize += n;
        result->decodedSize += n;

        return curFrameSize == sizeof(ProtoHeader) ? CheckHeader(reinterpret_cast<const ProtoHeader*>(curFrame))
                                                     :
                                                     DECODE_INCOMPLETE;
    }
    return DECODE_OK;
}

ProtoDecoder::eDecodeStatus ProtoDecoder::GatherFrame(const void* buffer, size_t length, DecodeResult* result)
{
    ProtoHeader* header = reinterpret_cast<ProtoHeader*>(curFrame);
    size_t frameSize = header->frameSize;
    if (curFrameSize < frameSize)
    {
        size_t n = Min(frameSize - curFrameSize, length);
        Memcpy(curFrame + curFrameSize, buffer, n);
        curFrameSize += n;
        result->decodedSize += n;

        return curFrameSize == frameSize ? DECODE_OK
                                           :
                                           DECODE_INCOMPLETE;
    }
    return DECODE_OK;
}

ProtoDecoder::eDecodeStatus ProtoDecoder::CheckHeader(const ProtoHeader* header) const
{
    // TODO: do more sophisticated check
    if (header->frameSize >= sizeof(ProtoHeader) && TYPE_FIRST <= header->frameType && header->frameType <= TYPE_LAST)
    {
        return DECODE_OK;
    }
    return DECODE_INVALID;
}

} // namespace Net
} // namespace DAVA
