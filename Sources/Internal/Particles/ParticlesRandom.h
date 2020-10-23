#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
namespace ParticlesRandom
{
float32 HammersleyRnd(uint32 n);
float32 HammersleyRnd(float32 min, float32 max, uint32 n);
float32 VanDerCorputRnd(uint32 n, uint32 base);
float32 VanDerCorputRnd(float32 min, float32 max, uint32 n, uint32 base);
}
}
