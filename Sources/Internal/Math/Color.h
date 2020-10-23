#pragma once

#include "Base/Any.h"
#include "Math/Vector.h"

namespace DAVA
{
/**
	\ingroup math
	\brief Class that represents 4-component RGBA float color
	This class is used in all SDK subsystems to work with colors.
 */
class Color
{
public:
    enum eChannel
    {
        CHANNEL_RED = 0,
        CHANNEL_GREEN = 1,
        CHANNEL_BLUE = 2,
        CHANNEL_ALPHA = 3,
        CHANNEL_COUNT = 4
    };

    static const Color White;
    static const Color Transparent;
    static const Color Clear;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Magenta;
    static const Color Cyan;

    union
    {
        float32 color[CHANNEL_COUNT];
        struct
        {
            float32 r, g, b, a;
        };
    };

    inline Color();
    inline Color(float32 r, float32 g, float32 b, float32 a);
    inline Color(uint32 rgba);
    inline explicit Color(const Vector4& vector);

    inline const Color& operator+=(const Color& _v);
    inline const Color& operator-=(const Color& _v);
    inline const Color& operator*=(const Color& _v);
    inline const Color& operator*=(float32 f);
    inline const Color& operator/=(float32 f);

    //! Comparison operators
    inline bool operator==(const Color& _v) const;
    inline bool operator!=(const Color& _v) const;

    inline uint32 GetRGBA() const;
};

static_assert(sizeof(Color) == 4 * sizeof(float32), "Wrong Sizeof(Color)");

//! with color
inline Color operator-(const Color& _v1, const Color& _v2);
inline Color operator+(const Color& _v1, const Color& _v2);
//! with scalar
inline Color operator+(const Color& _v, float32 _f);
inline Color operator+(float32 _f, const Color& _v);
inline Color operator-(const Color& _v, float32 _f);
inline Color operator-(float32 _f, const Color& _v);
inline Color operator*(const Color& _v, float32 _f);
inline Color operator*(float32 _f, const Color& _v);
inline Color operator*(const Color& _v1, const Color& _v2);
inline Color operator/(const Color& _v, float32 _f);
inline Color operator/(float32 _f, const Color& _v);

// Color inline implementation
inline Color::Color()
{
    r = g = b = a = 1.0f;
}
inline Color::Color(float32 _r, float32 _g, float32 _b, float32 _a)
{
    r = _r;
    g = _g;
    b = _b;
    a = _a;
}

inline Color::Color(const Vector4& vector)
{
    r = vector.x;
    g = vector.y;
    b = vector.z;
    a = vector.w;
}
inline Color::Color(uint32 rgba)
{
    for (int i = 0; i < 4; i++)
    {
        uint32 intVal = ((rgba >> (8 * i)) & 0xff);
        color[3 - i] = intVal / 255.0f;
    }
}

//! On operations
inline const Color& Color::operator+=(const Color& _v)
{
    r += _v.r;
    g += _v.g;
    b += _v.b;
    a += _v.a;
    return *this;
}

inline const Color& Color::operator-=(const Color& _v)
{
    r -= _v.r;
    g -= _v.g;
    b -= _v.b;
    a -= _v.a;
    return *this;
}

inline const Color& Color::operator*=(const Color& _v)
{
    r *= _v.r;
    g *= _v.g;
    b *= _v.b;
    a *= _v.a;
    return *this;
}

inline const Color& Color::operator*=(float32 f)
{
    r *= f;
    g *= f;
    b *= f;
    a *= f;
    return *this;
}
inline const Color& Color::operator/=(float32 f)
{
    r /= f;
    g /= f;
    b /= f;
    a /= f;
    return *this;
}

//! Comparison operators
inline bool Color::operator==(const Color& _v) const
{
    return (Memcmp(color, _v.color, sizeof(Color)) == 0);
}
inline bool Color::operator!=(const Color& _v) const
{
    return (Memcmp(color, _v.color, sizeof(Color)) != 0);
}

inline uint32 Color::GetRGBA() const
{
    return (static_cast<uint32>(a * 255.f) << 24)
    | (static_cast<uint32>(b * 255.f) << 16)
    | (static_cast<uint32>(g * 255.f) << 8)
    | (static_cast<uint32>(r * 255.f));
}

//! operators
inline Color operator-(const Color& _v1, const Color& _v2)
{
    return Color(_v1.r - _v2.r, _v1.g - _v2.g, _v1.b - _v2.b, _v1.a - _v2.a);
}
inline Color operator+(const Color& _v1, const Color& _v2)
{
    return Color(_v1.r + _v2.r, _v1.g + _v2.g, _v1.b + _v2.b, _v1.a + _v2.a);
}

//! with scalar
inline Color operator+(const Color& _v, float32 _f)
{
    return Color(_v.r + _f, _v.g + _f, _v.b + _f, _v.a + _f);
}

inline Color operator+(float32 _f, const Color& _v)
{
    return Color(_v.r + _f, _v.g + _f, _v.b + _f, _v.a + _f);
}

inline Color operator-(const Color& _v, float32 _f)
{
    return Color(_v.r - _f, _v.g - _f, _v.b - _f, _v.a - _f);
}
inline Color operator-(float32 _f, const Color& _v)
{
    return Color(_f - _v.r, _f - _v.g, _f - _v.b, _f - _v.a);
}

inline Color operator*(const Color& _v, float32 _f)
{
    return Color(_v.r * _f, _v.g * _f, _v.b * _f, _v.a * _f);
}

inline Color operator*(float32 _f, const Color& _v)
{
    return Color(_f * _v.r, _f * _v.g, _f * _v.b, _f * _v.a);
}

inline Color operator*(const Color& _v1, const Color& _v2)
{
    return Color(_v1.r * _v2.r, _v1.g * _v2.g, _v1.b * _v2.b, _v1.a * _v2.a);
}

inline Color operator/(const Color& _v, float32 _f)
{
    return Color(_v.r / _f, _v.g / _f, _v.b / _f, _v.a / _f);
}

inline Color operator/(float32 _f, const Color& _v)
{
    return Color(_f / _v.r, _f / _v.g, _f / _v.b, _f / _v.a);
}

Color ClampToUnityRange(Color color);

Color MakeGrayScale(const Color& colorRGB);

template <>
bool AnyCompare<Color>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<Color>;
};
