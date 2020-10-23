#include "Color.h"

namespace DAVA
{
const Color Color::White(1.f, 1.f, 1.f, 1.f);
const Color Color::Transparent(1.f, 1.f, 1.f, 0.f);
const Color Color::Clear(0.f, 0.f, 0.f, 0.f);
const Color Color::Black(0.f, 0.f, 0.f, 1.f);
const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);

Color ClampToUnityRange(Color color)
{
    color.r = DAVA::Clamp(0.f, 1.f, color.r);
    color.g = DAVA::Clamp(0.f, 1.f, color.g);
    color.b = DAVA::Clamp(0.f, 1.f, color.b);
    color.a = DAVA::Clamp(0.f, 1.f, color.a);

    return color;
}

Color MakeGrayScale(const Color& rgb)
{
    // http://www.tannerhelland.com/3643/grayscale-image-algorithm-vb6/

    float32 channel = rgb.r * 0.3f + rgb.g * 0.59f + rgb.b * 0.11f;
    return Color(channel, channel, channel, rgb.a);
}

template <>
bool AnyCompare<Color>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Color>() == v2.Get<Color>();
}
}