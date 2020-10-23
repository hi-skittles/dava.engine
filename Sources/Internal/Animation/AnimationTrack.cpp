#include "AnimationTrack.h"
#include "AnimationChannel.h"

#include "Base/FastName.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
uint32 AnimationTrack::Bind(const uint8* _data)
{
    channels.clear();

    const uint8* dataptr = _data;
    if (dataptr && *reinterpret_cast<const uint32*>(dataptr) == ANIMATION_TRACK_DATA_SIGNATURE)
    {
        dataptr += 4; //skip signature

        uint32 channelsCount = *reinterpret_cast<const uint32*>(dataptr);
        dataptr += 4;

        channels.resize(channelsCount);

        for (uint32 c = 0; c < channelsCount; ++c)
        {
            channels[c].target = eChannelTarget(*dataptr);
            dataptr += 1;

            dataptr += 3; //pad

            uint32 boundData = channels[c].channel.Bind(dataptr);
            if (boundData == 0)
            {
                channels.clear();
                return 0;
            }

            dataptr += boundData;
        }
    }

    return uint32(dataptr - _data);
}

void AnimationTrack::Evaluate(float32 time, uint32 channel, float32* outData, uint32 dataSize) const
{
    DVASSERT(channel < GetChannelsCount());
    channels[channel].channel.Evaluate(time, outData, dataSize);
}

uint32 AnimationTrack::GetChannelsCount() const
{
    return uint32(channels.size());
}

AnimationTrack::eChannelTarget AnimationTrack::GetChannelTarget(uint32 channel) const
{
    DVASSERT(channel < GetChannelsCount());
    return channels[channel].target;
}

uint32 AnimationTrack::GetChannelValueSize(uint32 channel) const
{
    DVASSERT(channel < GetChannelsCount());
    return channels[channel].channel.GetDimension();
}

uint32 AnimationTrack::GetMaxChannelValueSize() const
{
    uint32 maxChannelSize = 0;
    for (const Channel& channel : channels)
        maxChannelSize = Max(maxChannelSize, channel.channel.GetDimension());

    return maxChannelSize;
}
}