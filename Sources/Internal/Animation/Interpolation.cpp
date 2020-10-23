#include "Base/BaseMath.h"
#include "Animation/Interpolation.h"
#include "Math/MathConstants.h"

namespace DAVA
{
Interpolation::Func Interpolation::GetFunction(FuncType type)
{
    static Interpolation::Func functions[] =
    {
      static_cast<float32 (*)(float32)>(&Interpolation::Linear),
      static_cast<float32 (*)(float32)>(&Interpolation::EaseIn),
      static_cast<float32 (*)(float32)>(&Interpolation::EaseOut),
      static_cast<float32 (*)(float32)>(&Interpolation::EaseInEaseOut),
      &Interpolation::SineIn,
      &Interpolation::SineOut,
      &Interpolation::SineInSineOut,
      &Interpolation::ElasticIn,
      &Interpolation::ElasticOut,
      &Interpolation::ElasticInElasticOut,
      &Interpolation::BounceIn,
      &Interpolation::BounceOut,
      &Interpolation::BounceInBounceOut,
      static_cast<float32 (*)(float32)>(&Interpolation::EaseIn),
      static_cast<float32 (*)(float32)>(&Interpolation::EaseOut),
      static_cast<float32 (*)(float32)>(&Interpolation::EaseInEaseOut)
    };

    DVASSERT(type < FUNC_TYPE_COUNT);
    return functions[type];
}

float32 Interpolation::Linear(float32 currentVal)
{
    return FloatClamp(0.0f, 1.0f, currentVal);
}

float32 Interpolation::EaseIn(float32 currentVal)
{
    float32 time = currentVal;
    float32 time2 = time * time;
    float32 time3 = time2 * time;

    float32 point1X = 0.04f * 3.0f;
    float32 point2X = 0.17f * 3.0f;

    float32 val = time3 * (1.0f + point1X - point2X) + time2 * (point2X - 2 * point1X) + point1X * time;
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::EaseOut(float32 currentVal)
{
    float32 time = currentVal;
    float32 time2 = time * time;
    float32 time3 = time2 * time;

    float32 point1X = 0.83f * 3.0f;
    float32 point2X = 0.96f * 3.0f;

    float32 val = time3 * (1.0f + point1X - point2X) + time2 * (point2X - 2 * point1X) + point1X * time;
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::EaseInEaseOut(float32 currentVal)
{
    float32 time = currentVal;
    float32 time2 = time * time;
    float32 time3 = time2 * time;

    float32 point1X = 0.08f * 3.0f;
    float32 point2X = 0.92f * 3.0f;

    float32 val = time3 * (1.0f + point1X - point2X) + time2 * (point2X - 2 * point1X) + point1X * time;
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::Linear(float32 moveFrom, float32 moveTo, float32 startVal, float32 currentVal, float32 endVal)
{
    float32 delta = FLOAT_EQUAL(endVal, startVal) ? (0.0f) : ((moveTo - moveFrom) * (currentVal - startVal) / (endVal - startVal));
    return moveFrom + delta;
}

float32 Interpolation::EaseIn(float32 moveFrom, float32 moveTo, float32 startVal, float32 currentVal, float32 endVal)
{
    float32 time = (currentVal - startVal) / (endVal - startVal);
    float32 point1X = moveFrom + (moveTo - moveFrom) * 0.04f;
    float32 point2X = moveFrom + (moveTo - moveFrom) * 0.17f;
    float32 xCoord = moveFrom * (1.0f - time) * (1.0f - time) * (1.0f - time) +
    point1X * 3.0f * time * (1.0f - time) * (1.0f - time) +
    point2X * 3.0f * time * time * (1.0f - time) +
    moveTo * time * time * time;

    return xCoord;
}

float32 Interpolation::EaseOut(float32 moveFrom, float32 moveTo, float32 startVal, float32 currentVal, float32 endVal)
{
    float32 time = (currentVal - startVal) / (endVal - startVal);
    float32 point1X = moveFrom + (moveTo - moveFrom) * 0.83f;
    float32 point2X = moveFrom + (moveTo - moveFrom) * 0.96f;
    float32 xCoord = moveFrom * (1.0f - time) * (1.0f - time) * (1.0f - time) +
    point1X * 3.0f * time * (1.0f - time) * (1.0f - time) +
    point2X * 3.0f * time * time * (1.0f - time) +
    moveTo * time * time * time;

    return xCoord;
}

float32 Interpolation::EaseInEaseOut(float32 moveFrom, float32 moveTo, float32 startVal, float32 currentVal, float32 endVal)
{
    float32 time = (currentVal - startVal) / (endVal - startVal);
    float32 point1X = moveFrom + (moveTo - moveFrom) * 0.02f;
    float32 point2X = moveFrom + (moveTo - moveFrom) * 0.98f;
    float32 xCoord = moveFrom * (1.0f - time) * (1.0f - time) * (1.0f - time) +
    point1X * 3.0f * time * (1.0f - time) * (1.0f - time) +
    point2X * 3.0f * time * time * (1.0f - time) +
    moveTo * time * time * time;

    return xCoord;
}

float32 Interpolation::SineIn(float32 t)
{
    float32 val = sin((t - 1.0f) * PI_05) + 1.0f;
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::SineOut(float32 t)
{
    float32 val = sin(t * PI_05);
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::SineInSineOut(float32 t)
{
    float32 val = -0.5f * (cos(PI * t) - 1.0f);
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::ElasticIn(float32 t)
{
    float32 val = sin(13.0f * t * PI_05) * pow(2.0f, 10.0f * (t - 1.0f));
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::ElasticOut(float32 t)
{
    float32 val = sin(-13.0f * (t + 1.0f) * PI_05) * pow(2.0f, -10.0f * t) + 1.0f;
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::ElasticInElasticOut(float32 t)
{
    float32 val = t < 0.5f
    ?
    0.5f * sin(+13.0f * PI_05 * 2.0f * t) * pow(2.0f, 10.0f * (2.0f * t - 1.0f))
    :
    0.5f * sin(-13.0f * PI_05 * ((2.0f * t - 1.0f) + 1.0f)) * pow(2.0f, -10.0f * (2.0f * t - 1.0f)) + 1.0f;

    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::BounceOut(float32 t)
{
    const float32 a = 4.0f / 11.0f;
    const float32 b = 8.0f / 11.0f;
    const float32 c = 9.0f / 10.0f;

    const float32 ca = 4356.0f / 361.0f;
    const float32 cb = 35442.0f / 1805.0f;
    const float32 cc = 16061.0f / 1805.0f;

    float32 t2 = t * t;

    float32 val = t < a
    ?
    7.5625f * t2
    :
    t < b
    ?
    9.075f * t2 - 9.9f * t + 3.4f
    :
    t < c
    ?
    ca * t2 - cb * t + cc
    :
    10.8f * t * t - 20.52f * t + 10.72f;

    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::BounceIn(float32 t)
{
    float32 val = 1.0f - BounceOut(1.0f - t);
    return FloatClamp(0.0f, 1.0f, val);
}

float32 Interpolation::BounceInBounceOut(float32 t)
{
    float32 val = t < 0.5f
    ?
    0.5f * (1.0f - BounceOut(1.0f - t * 2.0f))
    :
    0.5f * BounceOut(t * 2.0f - 1.0f) + 0.5f;

    return FloatClamp(0.0f, 1.0f, val);
}
};
