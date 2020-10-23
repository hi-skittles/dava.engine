#include "Particles/ParticlesRandom.h"

// https://blog.demofox.org/2017/05/29/when-random-numbers-are-too-random-low-discrepancy-sequences/

namespace DAVA
{
namespace ParticlesRandom
{
float32 HammersleyRnd(uint32 n)
{
    float32 base = 0.5f;
    float32 res = 0.0f;
    while (n != 0)
    {
        if ((n & 1) != 0)
            res += base;
        n >>= 1;
        base *= 0.5f;
    }
    return res;
}

float32 HammersleyRnd(float32 min, float32 max, uint32 n)
{
    return (max - min) * HammersleyRnd(n) + min;
}

float32 VanDerCorputRnd(uint32 n, uint32 base)
{
    float32 res = 0.0f;
    float32 denom = static_cast<float32>(base);
    while (n > 0)
    {
        uint32 mult = n % base;
        res += static_cast<float32>(mult) / denom;
        n /= base;
        denom *= base;
    }
    return res;
}

float32 VanDerCorputRnd(float32 min, float32 max, uint32 n, uint32 base)
{
    return (max - min) * VanDerCorputRnd(n, base) + min;
}
}
}