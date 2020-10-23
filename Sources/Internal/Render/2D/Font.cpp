#include "Render/2D/Font.h"
#include "Engine/Engine.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "FontManager.h"
#include "UI/UIControlSystem.h"
#include "Utils/CRC32.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
int32 Font::globalFontDPI = 72;

void Font::SetDPI(int32 dpi)
{
    globalFontDPI = dpi;
}

int32 Font::GetDPI()
{
    return globalFontDPI;
}

Font::Font()
{
    GetEngineContext()->fontManager->RegisterFont(this);
}

Font::~Font()
{
    GetEngineContext()->fontManager->UnregisterFont(this);
}

bool Font::IsEqual(const Font* font) const
{
    if (!font)
    {
        return false;
    }

    if (GetFontType() != font->GetFontType())
    {
        return false;
    }

    if (GetVerticalSpacing() != font->GetVerticalSpacing())
    {
        return false;
    }

    if (!FLOAT_EQUAL(GetAscendScale(), font->GetAscendScale()))
    {
        return false;
    }

    if (!FLOAT_EQUAL(GetDescendScale(), font->GetDescendScale()))
    {
        return false;
    }

    return true;
}

bool Font::IsTextSupportsSoftwareRendering() const
{
    return false;
}

bool Font::IsTextSupportsHardwareRendering() const
{
    return false;
}

uint32 Font::GetHashCode()
{
    String rawHashString = GetRawHashString();
    return CRC32::ForBuffer(rawHashString.c_str(), static_cast<uint32>(rawHashString.length()));
}

String Font::GetRawHashString()
{
    return Format("%i_%i_%0.3f_%0.3f", GetFontType(), GetVerticalSpacing(), GetAscendScale(), GetDescendScale());
}

void Font::SetVerticalSpacing(int32 _verticalSpacing)
{
    verticalSpacing = _verticalSpacing;
}

int32 Font::GetVerticalSpacing() const
{
    return verticalSpacing;
}

Size2i Font::GetStringSize(float32 size, const WideString& str, Vector<float32>* charSizes)
{
    StringMetrics metrics = GetStringMetrics(size, str, charSizes);
    return Size2i(int32(std::ceil(metrics.width)), int32(std::ceil(metrics.height)));
}

Font::eFontType Font::GetFontType() const
{
    return fontType;
}

YamlNode* Font::SaveToYamlNode() const
{
    YamlNode* node = new YamlNode(YamlNode::TYPE_MAP);

    //Type
    node->Set("type", "Font");
    //Vertical Spacing
    node->Set("verticalSpacing", this->GetVerticalSpacing());
    //Ascend / descend
    node->Set("ascendScale", this->GetAscendScale());
    node->Set("descendScale", this->GetDescendScale());

    return node;
}

void Font::SetAscendScale(float32 ascendScale)
{
    // Not implemented
}

DAVA::float32 Font::GetAscendScale() const
{
    return 1.f;
}

void Font::SetDescendScale(float32 descendScale)
{
    // Not implemented
}

DAVA::float32 Font::GetDescendScale() const
{
    return 1.f;
}
};
