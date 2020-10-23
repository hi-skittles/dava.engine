#include "AnimationChannel.h"
#include "Base/BaseMath.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
uint32 AnimationChannel::Bind(const uint8* _data)
{
    keysData = nullptr;
    dimension = 0;
    keyStride = keysCount = 0;

    const uint8* dataptr = _data;
    if (_data != nullptr && *reinterpret_cast<const uint32*>(_data) == ANIMATION_CHANNEL_DATA_SIGNATURE)
    {
        dataptr += 4; //skip signature

        dimension = *dataptr;
        dataptr += 1;

        interpolation = eInterpolation(*dataptr);
        dataptr += 1;

        compression = *reinterpret_cast<const uint16*>(dataptr);
        dataptr += 2;

        keysCount = *reinterpret_cast<const uint32*>(dataptr);
        dataptr += 4;

        keysData = dataptr;

        keyStride = uint32(sizeof(float32)) * (dimension + 1);
        if (interpolation == INTERPOLATION_BEZIER)
            keyStride += uint32(sizeof(float32) * 4); //four float32 as tangents
    }

    return uint32(keysData - _data) + keysCount * keyStride;
}

#define KEY_DATA_SIZE (dimension * sizeof(float32))
#define KEY_TIME(keyIndex) (*reinterpret_cast<const float32*>(keysData + (keyIndex)*keyStride))
#define KEY_DATA(keyIndex) (reinterpret_cast<const float32*>(keysData + (keyIndex)*keyStride + sizeof(float32)))
#define KEY_META(keyIndex) (KEY_DATA(keyIndex) + KEY_DATA_SIZE) //tangents for bezier interpolation

void AnimationChannel::Evaluate(float32 time, float32* outData, uint32 dataSize) const
{
    DVASSERT(dataSize >= GetDimension());

    uint32 k = startKey;

    if (KEY_TIME(k) > time)
    {
        k = 0;
    }

    for (; k < keysCount; ++k)
    {
        if (KEY_TIME(k) > time)
            break;

        startKey = k;
    }

    if (k == 0)
    {
        Memcpy(outData, KEY_DATA(0), KEY_DATA_SIZE);
        return;
    }

    if (k == keysCount)
    {
        Memcpy(outData, KEY_DATA(keysCount - 1), KEY_DATA_SIZE);
        return;
    }

    uint32 k0 = k - 1;
    float32 time0 = KEY_TIME(k0);
    float32 time1 = KEY_TIME(k);
    float32 t = (time - time0) / (time1 - time0);

    switch (interpolation)
    {
    case INTERPOLATION_LINEAR:
    {
        for (uint32 d = 0; d < uint32(dimension); ++d)
        {
            float32 v0 = *(KEY_DATA(k0) + d);
            float32 v1 = *(KEY_DATA(k) + d);
            *(outData + d) = Lerp(v0, v1, t);
        }
    }
    break;

    case INTERPOLATION_SPHERICAL_LINEAR:
    {
        DVASSERT(dimension == 4); //should be quaternion

        Quaternion q0(KEY_DATA(k0));
        Quaternion q(KEY_DATA(k));
        q.Slerp(q0, q, t);
        q.Normalize();

        Memcpy(outData, q.data, KEY_DATA_SIZE);
    }
    break;

    case INTERPOLATION_BEZIER:
    {
        DVASSERT(false, "Bezier not supported yet");
    }
    break;

    default:
        break;
    }
}

#undef KEY_DATA_SIZE
#undef KEY_TIME
#undef KEY_DATA
#undef KEY_META
}
