#ifndef __DAVAENGINE_GHOSTPROTODECODER_H__
#define __DAVAENGINE_GHOSTPROTODECODER_H__

#include <Network/Private/ProtoTypes.h>

namespace DAVA
{
namespace Net
{
class ProtoDecoder
{
public:
    enum eDecodeStatus
    {
        DECODE_OK, // Frame or data packet decoded
        DECODE_INCOMPLETE, // Frame or data packet is incomplete, need more data
        DECODE_INVALID // Frame is invalid
    };

    struct DecodeResult
    {
        size_t decodedSize; // Number of bytes consumed from input buffer
        uint32 type;
        uint32 channelId;
        uint32 packetId;
        size_t dataSize;
        uint8* data; // Pointer to user data of data packet
    };

public:
    ProtoDecoder();

    eDecodeStatus Decode(const void* buffer, size_t length, DecodeResult* result);
    size_t EncodeDataFrame(ProtoHeader* header, uint32 channelId, uint32 packetId, size_t packetSize, size_t encodedSize) const;
    size_t EncodeControlFrame(ProtoHeader* header, uint32 type, uint32 channelId, uint32 packetId) const;

private:
    eDecodeStatus ProcessDataFrame(ProtoHeader* header, DecodeResult* result);
    eDecodeStatus ProcessControlFrame(ProtoHeader* header, DecodeResult* result);

    eDecodeStatus GatherHeader(const void* buffer, size_t length, DecodeResult* result);
    eDecodeStatus GatherFrame(const void* buffer, size_t length, DecodeResult* result);
    eDecodeStatus CheckHeader(const ProtoHeader* header) const;

private:
    size_t totalDataSize;
    size_t accumulatedSize;
    Vector<uint8> accum;

    uint8 curFrame[PROTO_MAX_FRAME_SIZE];
    size_t curFrameSize;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_ _H__
