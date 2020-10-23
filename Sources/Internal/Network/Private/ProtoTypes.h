#ifndef __DAVAENGINE_PROTOTYPES_H__
#define __DAVAENGINE_PROTOTYPES_H__

#include <Base/BaseTypes.h>

namespace DAVA
{
namespace Net
{
struct ProtoHeader
{
    ProtoHeader() = default;

    uint16 frameSize = 0; // Frame length: header + data
    uint16 frameType = 0; // Frame type
    uint32 channelId = 0; // Channel identifier
    uint32 packetId = 0; // Packet Id for acknoledgements
    uint32 totalSize = 0; // Total size of user data
};

const size_t PROTO_MAX_FRAME_SIZE = 1024 * 64 - 1;
const size_t PROTO_MAX_FRAME_DATA_SIZE = PROTO_MAX_FRAME_SIZE - sizeof(ProtoHeader);

enum eProtoFrameType
{
    TYPE_DATA, // Frame carries user data
    TYPE_CHANNEL_QUERY, // Control frame: check whether channel is available
    TYPE_CHANNEL_ALLOW, // Control frame: answer to CHANNEL_QUERY frame: channel is available
    TYPE_CHANNEL_DENY, // Control frame: answer to CHANNEL_QUERY frame: channel is not available
    TYPE_PING, // Control frame: keep-alive request
    TYPE_PONG, // Control frame: answer to PING frame
    TYPE_DELIVERY_ACK, // Control frame: user data packet delivered

    TYPE_FIRST = TYPE_DATA,
    TYPE_CONTROL_FIRST = TYPE_CHANNEL_QUERY,
    TYPE_LAST = TYPE_DELIVERY_ACK
};

enum eProtoFrameFlags
{
    FRAME_NO_DELIVERY_ACK = 0x01
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_PROTOTYPES_H__
