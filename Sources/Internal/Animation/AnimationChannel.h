#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class AnimationChannel
{
public:
    static const uint32 ANIMATION_CHANNEL_DATA_SIGNATURE = DAVA_MAKEFOURCC('D', 'V', 'A', 'C');

    enum eInterpolation : uint8
    {
        INTERPOLATION_LINEAR = 0,
        INTERPOLATION_SPHERICAL_LINEAR,
        INTERPOLATION_BEZIER,

        INTERPOLATION_COUNT
    };

    AnimationChannel() = default;

    uint32 Bind(const uint8* data);
    void Evaluate(float32 time, float32* outData, uint32 dataSize) const;

    uint32 GetDimension() const;

private:
    const DAVA::uint8* keysData = nullptr;
    mutable uint32 startKey = 0;
    uint32 keysCount = 0;
    uint32 keyStride = 0;
    uint16 compression = 0;
    uint8 dimension = 0;
    eInterpolation interpolation = INTERPOLATION_COUNT;
};

inline uint32 AnimationChannel::GetDimension() const
{
    return uint32(dimension);
}
}