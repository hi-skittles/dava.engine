#pragma once

#include <Base/BaseTypes.h>
#include <Math/Vector.h>

namespace DAVA
{
float32 PerlinNoise2d(const Vector2& position, float32 wrap);
float32 PerlinNoise3d(const Vector3& position, float32 wrap);
Vector3 Generate2OctavesPerlin(const Vector2& p);
}