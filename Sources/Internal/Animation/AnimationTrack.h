#pragma once

#include "AnimationChannel.h"

namespace DAVA
{
class AnimationTrack
{
public:
    static const uint32 ANIMATION_TRACK_DATA_SIGNATURE = DAVA_MAKEFOURCC('D', 'V', 'A', 'T');

    enum eChannelTarget : uint8
    {
        CHANNEL_TARGET_POSITION = 0,
        CHANNEL_TARGET_ORIENTATION,
        CHANNEL_TARGET_SCALE,

        CHANNEL_TARGET_COUNT
    };

    uint32 Bind(const uint8* data);
    void Evaluate(float32 time, uint32 channel, float32* outData, uint32 dataSize) const;

    uint32 GetChannelsCount() const;
    eChannelTarget GetChannelTarget(uint32 channel) const;

    uint32 GetChannelValueSize(uint32 channel) const;
    uint32 GetMaxChannelValueSize() const;

private:
    struct Channel
    {
        AnimationChannel channel;
        eChannelTarget target;
    };
    Vector<Channel> channels;
};
}
