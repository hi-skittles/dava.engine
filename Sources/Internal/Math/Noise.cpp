#include "Math/Noise.h"

#include <math.h>

#include "Math/MathHelpers.h"

// Noise implementation based on https://github.com/g-truc/glm/blob/master/glm/gtc/noise.inl

namespace DAVA
{
namespace NoiseDetails
{
Vector4 Permute(const Vector4& v)
{
    Vector4 vs = v * 34.0f;
    return Vector4
    (
    std::fmod((vs.x + 1.0f) * v.x, 289.0f),
    std::fmod((vs.y + 1.0f) * v.y, 289.0f),
    std::fmod((vs.z + 1.0f) * v.z, 289.0f),
    std::fmod((vs.w + 1.0f) * v.w, 289.0f)
    );
}

Vector4 TaylorInvSqrt(const Vector4& r)
{
    Vector4 rs = 0.85373472095314f * r;
    static const float32 num = 1.79284291400159f;
    return Vector4
    (
    num - rs.x,
    num - rs.y,
    num - rs.z,
    num - rs.w
    );
}

#define NOISE_FAST_FADE 1
float32 Fade(float32 val)
{
#if NOISE_FAST_FADE
    return val * val * (3.0f - 2.0f * val);
#else
    return val * val * val * (val * (val * 6.0f - 15.0f) + 10.0f);
#endif
}

Vector3 Fade(const Vector3& t)
{
    return Vector3(Fade(t.x), Fade(t.y), Fade(t.z));
}

Vector2 Fade(const Vector2& t)
{
    return Vector2(Fade(t.x), Fade(t.y));
}

int32 WrapBlock(const int32 block, const int32 numBlocks, const float32 scale)
{
    if (static_cast<float32>(block) >= numBlocks * scale)
        return 0;
    if (block < 0)
        return static_cast<int32>(static_cast<float32>(numBlocks) * scale) - 1;
    return block;
}

void GetPoint(int32 blockX, int32 blockY, int32 blockZ, int32 numBlocks, int32 blockSize, const int32 numPointsInBlock, const int32 i, const float32 scale, const float32* array, float32* x, float32* y, float32* z)
{
    int32 wbx = WrapBlock(blockX, numBlocks, scale);
    int32 wby = WrapBlock(blockY, numBlocks, scale);
    int32 wbz = WrapBlock(blockZ, numBlocks, scale);

    int32 offset = numPointsInBlock * (wbz * numBlocks * numBlocks + wby * numBlocks + wbx) + i;
    float32 px = *(array + 3 * offset + 0);
    float32 py = *(array + 3 * offset + 1);
    float32 pz = *(array + 3 * offset + 2);

    *x = (static_cast<float32>(blockX) + px) * static_cast<float32>(blockSize);
    *y = (static_cast<float32>(blockY) + py) * static_cast<float32>(blockSize);
    *z = (static_cast<float32>(blockZ) + pz) * static_cast<float32>(blockSize);
}
}

float32 PerlinNoise2d(const Vector2& p, float32 wrap)
{
    Vector4 pi = Floor(Vector4(p.x, p.y, p.x, p.y)) + Vector4(0.0f, 0.0f, 1.0f, 1.0f);
    Vector4 pf = Abs(Frac(Vector4(p.x, p.y, p.x, p.y))) - Vector4(0.0f, 0.0f, 1.0f, 1.0f);
    pi = Fmod(pi, Vector4(wrap, wrap, wrap, wrap));
    pi = Fmod(pi, Vector4(289.0f, 289.0f, 289.0f, 289.0f));

    Vector4 ix = Vector4(pi.x, pi.z, pi.x, pi.z);
    Vector4 iy = Vector4(pi.y, pi.y, pi.w, pi.w);
    Vector4 fx = Vector4(pf.x, pf.z, pf.x, pf.z);
    Vector4 fy = Vector4(pf.y, pf.y, pf.w, pf.w);

    Vector4 i = NoiseDetails::Permute(NoiseDetails::Permute((ix) + iy));

    Vector4 gx = 2.0f * Abs(Frac(i / 41.0f)) - 1.0f;
    Vector4 gy = Abs(gx) - 0.5f;
    Vector4 tx = Floor(gx + 0.5f);
    gx = gx - tx;

    Vector2 g00(gx.x, gy.x);
    Vector2 g10(gx.y, gy.y);
    Vector2 g01(gx.z, gy.z);
    Vector2 g11(gx.w, gy.w);

    Vector4 norm = NoiseDetails::TaylorInvSqrt(Vector4(g00.DotProduct(g00), g01.DotProduct(g01), g10.DotProduct(g10), g11.DotProduct(g11)));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;

    float32 n00 = g00.DotProduct(Vector2(fx.x, fy.x));
    float32 n10 = g10.DotProduct(Vector2(fx.y, fy.y));
    float32 n01 = g01.DotProduct(Vector2(fx.z, fy.z));
    float32 n11 = g11.DotProduct(Vector2(fx.w, fy.w));

    Vector2 fade_xy = NoiseDetails::Fade(Vector2(pf.x, pf.y));
    Vector2 n_x = Lerp(Vector2(n00, n01), Vector2(n10, n11), fade_xy.x);
    float32 n_xy = Lerp(n_x.x, n_x.y, fade_xy.y);
    return n_xy * 2.3f;
}

float32 PerlinNoise3d(const Vector3& p, float32 wrap)
{
    Vector3 pi0 = Floor(p);
    Vector3 pi1 = pi0 + Vector3(1.0f, 1.0f, 1.0f);

    pi0 = Fmod(pi0, Vector3(wrap, wrap, wrap));
    pi1 = Fmod(pi1, Vector3(wrap, wrap, wrap));

    Vector3 pf0 = Frac(p);
    Vector3 pf1 = pf0 - Vector3(1.0f, 1.0f, 1.0f);
    Vector4 ix = Vector4(pi0.x, pi1.x, pi0.x, pi1.x);
    Vector4 iy = Vector4(pi0.y, pi0.y, pi1.y, pi1.y);
    Vector4 iz0 = Vector4(pi0.z, pi0.z, pi0.z, pi0.z);
    Vector4 iz1 = Vector4(pi1.z, pi1.z, pi1.z, pi1.z);

    Vector4 ixy = NoiseDetails::Permute(NoiseDetails::Permute(ix) + iy);
    Vector4 ixy0 = NoiseDetails::Permute(ixy + iz0);
    Vector4 ixy1 = NoiseDetails::Permute(ixy + iz1);

    Vector4 gx0 = ixy0 / 7.0f;
    Vector4 gy0 = Abs(Frac(Floor(gx0) / 7.0f)) - 0.5f;
    gx0 = Abs(Frac(gx0));
    Vector4 gz0 = Vector4(0.5f, 0.5f, 0.5f, 0.5f) - Abs(gx0) - Abs(gy0);
    Vector4 sz0 = Step(gz0, Vector4(0.0f, 0.0f, 0.0f, 0.0f));

    gx0 -= sz0 * (Step(Vector4::Zero, gx0) - 0.5f);
    gy0 -= sz0 * (Step(Vector4::Zero, gy0) - 0.5f);

    Vector4 gx1 = ixy1 / 7.0f;
    Vector4 gy1 = Abs(Frac(Floor(gx1) / 7.0f)) - 0.5f;
    gx1 = Abs(Frac(gx1));
    Vector4 gz1 = Vector4(0.5f, 0.5f, 0.5f, 0.5f) - Abs(gx1) - Abs(gy1);
    Vector4 sz1 = Step(gz1, Vector4(0.0f, 0.0f, 0.0f, 0.0f));
    gx1 -= sz1 * (Step(Vector4::Zero, gx1) - 0.5f);
    gy1 -= sz1 * (Step(Vector4::Zero, gy1) - 0.5f);

    Vector3 g000 = Vector3(gx0.x, gy0.x, gz0.x);
    Vector3 g100 = Vector3(gx0.y, gy0.y, gz0.y);
    Vector3 g010 = Vector3(gx0.z, gy0.z, gz0.z);
    Vector3 g110 = Vector3(gx0.w, gy0.w, gz0.w);
    Vector3 g001 = Vector3(gx1.x, gy1.x, gz1.x);
    Vector3 g101 = Vector3(gx1.y, gy1.y, gz1.y);
    Vector3 g011 = Vector3(gx1.z, gy1.z, gz1.z);
    Vector3 g111 = Vector3(gx1.w, gy1.w, gz1.w);

    Vector4 norm0 = NoiseDetails::TaylorInvSqrt(Vector4(g000.DotProduct(g000), g010.DotProduct(g010), g100.DotProduct(g100), g110.DotProduct(g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    Vector4 norm1 = NoiseDetails::TaylorInvSqrt(Vector4(g001.DotProduct(g001), g011.DotProduct(g011), g101.DotProduct(g101), g111.DotProduct(g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;

    float32 n000 = g000.DotProduct(pf0);
    float32 n100 = g100.DotProduct(Vector3(pf1.x, pf0.y, pf0.z));
    float32 n010 = g010.DotProduct(Vector3(pf0.x, pf1.y, pf0.z));
    float32 n110 = g110.DotProduct(Vector3(pf1.x, pf1.y, pf0.z));
    float32 n001 = g001.DotProduct(Vector3(pf0.x, pf0.y, pf1.z));
    float32 n101 = g101.DotProduct(Vector3(pf1.x, pf0.y, pf1.z));
    float32 n011 = g011.DotProduct(Vector3(pf0.x, pf1.y, pf1.z));
    float32 n111 = g111.DotProduct(pf1);

    Vector3 fade_xyz = NoiseDetails::Fade(pf0);
    Vector4 n_z = Lerp<Vector4>({ n000, n100, n010, n110 }, { n001, n101, n011, n111 }, { fade_xyz.z, fade_xyz.z, fade_xyz.z, fade_xyz.z });
    Vector2 n_yz = Lerp<Vector2>({ n_z.x, n_z.y }, { n_z.z, n_z.w }, { fade_xyz.y, fade_xyz.y });
    float32 n_xyz = Lerp(n_yz.x, n_yz.y, fade_xyz.x);
    return n_xyz;
}

Vector3 Generate2OctavesPerlin(const Vector2& p)
{
    Vector3 total(0.0f, 0.0f, 0.0f);
    float frequency = 8.f;
    float amplitude = 1.0f;
    float persistence = 0.5f;
    uint32 octaves = 2;
    for (uint32 i = 0; i < octaves; ++i)
    {
        float n0 = PerlinNoise2d(p, frequency);
        float n1 = PerlinNoise2d(p + Vector2(123.4f, 129845.6f), frequency);
        float n2 = PerlinNoise2d(p + Vector2(-9519.0f, 9051.0f), frequency);
        total += Vector3(n0, n1, n2) * amplitude;
        frequency *= 2.0f;
        amplitude *= persistence;
    }
    return total;
}
}