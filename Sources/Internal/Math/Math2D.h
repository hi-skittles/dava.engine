#pragma once
#include <cmath>

//!
//! All 2D & 3D math represent vectors & points (2D eq to vector) as vector array
//! for example
//!	Math::Point2 < float32 > p; // [x y] (or [x y 1] for 3x3 matrix equations)
//! 
#include "Base/BaseTypes.h"
#include "Math/Math2DTemplateClasses.h"

// definition of basic 2D types
namespace DAVA
{
inline float32 FloatClamp(float32 min, float32 max, float32 val);
inline float32 Lerp(float32 v0, float32 v1, float32 t);

/*
    Helper classes designed mostly for internal framework usage
    in all general cases use Vector2, Rect and other classes instead
 */
//! int Point2
using Point2i = Point2Base<int32>;
//! float Point2
using Point2f = Point2Base<float32>;
//! int Size2
using Size2i = Size2Base<int32>;
//! float Size2
using Size2f = Size2Base<float32>;
//! int Rect2
using Rect2i = Rect2Base<int32>;
//! float Rect2
using Rect2f = Rect2Base<float32>;

// Implementations
inline float32 FloatClamp(float32 min, float32 max, float32 val)
{
    if (val > max)
        val = max;
    if (val < min)
        val = min;
    return val;
};

inline float32 Lerp(float32 v0, float32 v1, float32 t)
{
    return v0 * (1.0f - t) + v1 * t;
}

/**
    \brief Fast function to compute index of bit that is set in a value. Only one bit should be set to make it work correctly.
 */


#ifdef __GNUC__
//     #define CountLeadingZeros(x) __builtin_clz(x)
//     #define CountTrailingZeros(x) __builtin_ctz(x)
#define FastLog2(x) __builtin_ctz(x)
#else
//     inline uint32 popcnt( uint32 x )
//     {
//         x -= ((x >> 1) & 0x55555555);
//         x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
//         x = (((x >> 4) + x) & 0x0f0f0f0f);
//         x += (x >> 8);
//         x += (x >> 16);
//         return x & 0x0000003f;
//     }
//     inline uint32 CountLeadingZeros( uint32 x )
//     {
//         x |= (x >> 1);
//         x |= (x >> 2);
//         x |= (x >> 4);
//         x |= (x >> 8);
//         x |= (x >> 16);
//         return 32 - popcnt(x);
//     }
//     inline uint32 CountTrailingZeros( uint32 x )
//     {
//         return popcnt((x & -x) - 1);
//     }
extern const int MultiplyDeBruijnBitPosition2[32];
#define FastLog2(value) MultiplyDeBruijnBitPosition2[(uint32)(value * 0x077CB531U) >> 27]
#endif

template <pointer_size I>
struct StaticLog2
{
    static_assert(I > 0, "Invalid argument provided to StaticLog2");
    enum : pointer_size
    {
        value = 1 + StaticLog2<I / 2>::value
    };
};

template <>
struct StaticLog2<1>
{
    enum : pointer_size
    {
        value = 0
    };
};
};
