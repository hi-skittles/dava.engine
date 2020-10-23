#include "Render/2D/FontPreset.h"
#include "Math/MathDefines.h"
#include "Render/2D/Font.h"

namespace DAVA
{
const FontPreset FontPreset::EMPTY = {};

FontPreset::FontPreset() = default;

FontPreset::FontPreset(const RefPtr<Font>& font, float32 size)
    : font(font)
    , size(size)
{
}

FontPreset::FontPreset(const FontPreset& src)
    : font(src.font)
    , size(src.size)
{
}

FontPreset::~FontPreset() = default;

bool FontPreset::Valid() const
{
    return font.Valid();
}

bool FontPreset::operator==(const FontPreset& second) const
{
    return font == second.font && FLOAT_EQUAL(size, second.size);
}

void FontPreset::SetFont(const RefPtr<Font>& font_)
{
    font = font_;
}

void FontPreset::SetSize(float32 size_)
{
    size = size_;
}
}
