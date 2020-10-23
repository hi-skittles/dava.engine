#include "GraphicFont.h"
#include "Concurrency/LockGuard.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Shader.h"
#include "Render/Texture.h"
#include "UI/UIControlSystem.h"

#define NOT_DEF_CHAR 0xffff

namespace DAVA
{
class GraphicInternalFont : public BaseObject
{
public:
    static GraphicInternalFont* Create(const FilePath& descriptorPath);

protected:
    static Mutex dataMapMutex;

    GraphicInternalFont();
    virtual ~GraphicInternalFont();
    bool InitFromConfig(const FilePath& path);

    struct CharDescription
    {
        float32 height;
        float32 width;
        Map<int32, float32> kerning;
        float32 xOffset;
        float32 yOffset;
        float32 xAdvance;
        float32 u;
        float32 u2;
        float32 v;
        float32 v2;
    };
    using CharsMap = Map<char16, CharDescription>;

    CharsMap chars;
    float32 baseSize;
    float32 paddingLeft;
    float32 paddingRight;
    float32 paddingTop;
    float32 paddingBottom;
    float32 lineHeight;
    float32 baselineHeight;
    float32 spread;
    bool isDistanceFieldFont;

    FilePath configPath;

    friend class GraphicFont;
};

using FontGraphicMap = Map<FilePath, GraphicInternalFont*>;
FontGraphicMap dataMap;
Mutex GraphicInternalFont::dataMapMutex;

GraphicInternalFont::GraphicInternalFont()
{
    baseSize = 0;
    paddingLeft = paddingRight = paddingTop = paddingBottom = 0;
    lineHeight = 0;
    baselineHeight = 0;
    spread = 1.f;
    isDistanceFieldFont = false;
}

GraphicInternalFont::~GraphicInternalFont()
{
    LockGuard<Mutex> guard(dataMapMutex);
    dataMap.erase(configPath);
}

GraphicInternalFont* GraphicInternalFont::Create(const FilePath& descriptorPath)
{
    {
        LockGuard<Mutex> guard(dataMapMutex);
        FontGraphicMap::iterator iter = dataMap.find(descriptorPath);
        if (iter != dataMap.end())
        {
            return SafeRetain(iter->second);
        }
    }

    GraphicInternalFont* fontData = new GraphicInternalFont();
    if (!fontData->InitFromConfig(descriptorPath))
    {
        fontData->Release(); // Used mutex lock
        return nullptr;
    }

    {
        LockGuard<Mutex> guard(dataMapMutex);
        dataMap[descriptorPath] = fontData;
    }
    return fontData;
}

bool GraphicInternalFont::InitFromConfig(const DAVA::FilePath& path)
{
    RefPtr<YamlParser> parser = YamlParser::Create(path.GetAbsolutePathname());

    if (!parser)
        return false;

    configPath = path;

    YamlNode* rootNode = parser->GetRootNode();
    const YamlNode* configNode = rootNode->Get("font");
    if (!configNode)
    {
        return false;
    }
    const YamlNode* charsNode = configNode->Get("chars");
    if (!charsNode)
    {
        return false;
    }

    baseSize = configNode->Get("size")->AsFloat();
    const YamlNode* paddingTop = configNode->Get("padding_top");
    if (paddingTop)
        this->paddingTop = paddingTop->AsFloat();
    const YamlNode* paddingLeft = configNode->Get("padding_left");
    if (paddingLeft)
        this->paddingLeft = paddingLeft->AsFloat();
    const YamlNode* paddingBottom = configNode->Get("padding_bottop");
    if (paddingBottom)
        this->paddingBottom = paddingBottom->AsFloat();
    const YamlNode* paddingRight = configNode->Get("padding_right");
    if (paddingRight)
        this->paddingRight = paddingRight->AsFloat();
    const YamlNode* lineHeight = configNode->Get("lineHeight");
    if (lineHeight)
        this->lineHeight = lineHeight->AsFloat();
    const YamlNode* baselineHeight = configNode->Get("baselineHeight");
    if (baselineHeight)
        this->baselineHeight = baselineHeight->AsFloat();
    const YamlNode* spread = configNode->Get("spread");
    if (spread)
        this->spread = spread->AsFloat();
    const YamlNode* distanceFieldFont = configNode->Get("distanceFieldFont");
    if (distanceFieldFont)
        this->isDistanceFieldFont = distanceFieldFont->AsBool();

    const auto& charsMap = charsNode->AsMap();
    auto charsMapEnd = charsMap.end();
    for (auto iter = charsMap.begin(); iter != charsMapEnd; ++iter)
    {
        char16 charId = atoi(iter->first.c_str());
        CharDescription charDescription;
        charDescription.height = iter->second->Get("height")->AsFloat();
        charDescription.width = iter->second->Get("width")->AsFloat();
        charDescription.xOffset = iter->second->Get("xoffset")->AsFloat();
        charDescription.yOffset = iter->second->Get("yoffset")->AsFloat();
        charDescription.xAdvance = iter->second->Get("xadvance")->AsFloat();
        charDescription.u = iter->second->Get("u")->AsFloat();
        charDescription.u2 = iter->second->Get("u2")->AsFloat();
        charDescription.v = iter->second->Get("v")->AsFloat();
        charDescription.v2 = iter->second->Get("v2")->AsFloat();

        chars[charId] = charDescription;
    }

    const YamlNode* kerningNode = configNode->Get("kerning");
    if (kerningNode)
    {
        const auto& kerningMap = kerningNode->AsMap();
        for (auto iter = kerningMap.begin(); iter != kerningMap.end(); ++iter)
        {
            int32 charId = atoi(iter->first.c_str());
            CharsMap::iterator charIter = chars.find(charId);
            if (charIter == chars.end())
                continue;

            const auto& charKerningMap = iter->second->AsMap();
            for (auto i = charKerningMap.begin(); i != charKerningMap.end(); ++i)
            {
                int32 secondCharId = atoi(i->first.c_str());
                float32 kerning = i->second->AsFloat();
                charIter->second.kerning[secondCharId] = kerning;
            }
        }
    }

    return true;
}

GraphicFont::GraphicFont()
    : fontInternal(nullptr)
    , texture(nullptr)
{
    fontType = Font::TYPE_GRAPHIC;
}

GraphicFont::~GraphicFont()
{
    SafeRelease(fontInternal);
    SafeRelease(texture);
}

GraphicFont* GraphicFont::Create(const FilePath& descriptorPath, const FilePath& texturePath)
{
    GraphicFont* font = new GraphicFont();

    font->fontInternal = GraphicInternalFont::Create(descriptorPath);
    if (font->fontInternal == nullptr || !font->LoadTexture(texturePath))
    {
        SafeRelease(font);
        return nullptr;
    }

    if (font->fontInternal->isDistanceFieldFont)
    {
        font->fontType = Font::TYPE_DISTANCE;
    }

    return font;
}

Font::StringMetrics GraphicFont::GetStringMetrics(float32 size, const WideString& str, Vector<float32>* charSizes /* = 0*/) const
{
    if (charSizes != nullptr)
    {
        charSizes->clear();
    }
    int32 charDrawed = 0;
    return DrawStringToBuffer(size, str, 0, 0, nullptr, charDrawed, charSizes);
}

bool GraphicFont::IsCharAvaliable(char16 ch) const
{
    GraphicInternalFont::CharsMap::const_iterator iter = fontInternal->chars.find(ch);
    return iter != fontInternal->chars.end();
}

uint32 GraphicFont::GetFontHeight(float32 size) const
{
    return uint32(fontInternal->lineHeight * GetSizeScale(size));
}

Font* GraphicFont::Clone() const
{
    GraphicFont* graphicFont = new GraphicFont();
    graphicFont->fontInternal = SafeRetain(fontInternal);
    graphicFont->texture = SafeRetain(texture);
    return graphicFont;
}

bool GraphicFont::IsEqual(const Font* font) const
{
    if (!Font::IsEqual(font))
        return false;

    const GraphicFont* graphicFont = static_cast<const GraphicFont*>(font);
    if (graphicFont->fontInternal->configPath != fontInternal->configPath)
        return false;

    return true;
}

Font::StringMetrics GraphicFont::DrawStringToBuffer(float32 size,
                                                    const WideString& str,
                                                    int32 xOffset,
                                                    int32 yOffset,
                                                    GraphicFontVertex* vertexBuffer,
                                                    int32& charDrawed,
                                                    Vector<float32>* charSizes /*= NULL*/,
                                                    int32 justifyWidth,
                                                    int32 spaceAddon) const
{
    int32 countSpace = 0;
    uint32 strLength = static_cast<uint32>(str.length());
    for (uint32 i = 0; i < strLength; ++i)
    {
        if (L' ' == str[i])
        {
            countSpace++;
        }
    }
    int32 justifyOffset = 0;
    int32 fixJustifyOffset = 0;
    if (countSpace > 0 && justifyWidth > 0 && spaceAddon > 0)
    {
        int32 diff = justifyWidth - spaceAddon;
        justifyOffset = diff / countSpace;
        fixJustifyOffset = diff - justifyOffset * countSpace;
    }
    uint32 vertexAdded = 0;
    charDrawed = 0;

    float32 lastX = float32(xOffset);
    float32 lastY = 0;
    float32 sizeScale = GetSizeScale(size);

    GraphicInternalFont::CharsMap::const_iterator notDef = fontInternal->chars.find(NOT_DEF_CHAR);
    bool notDefExists = (notDef != fontInternal->chars.end());

    Font::StringMetrics metrics;
    metrics.drawRect = Rect2i(0x7fffffff, 0x7fffffff, 0, 0);

    float32 ascent = fontInternal->lineHeight * GetSizeScale(size);
    uint32 fontHeight = GetFontHeight(size);

    for (uint32 charPos = 0; charPos < strLength; ++charPos)
    {
        char16 charId = str.at(charPos);
        GraphicInternalFont::CharsMap::const_iterator iter = fontInternal->chars.find(charId);
        if (iter == fontInternal->chars.end())
        {
            if (notDefExists)
            {
                iter = notDef;
            }
            else
            {
                DVASSERT(false, "Font should contain .notDef character!");
                continue;
            }
        }

        if (charPos > 0 && justifyOffset > 0 && charId == L' ')
        {
            lastX += justifyOffset;
            if (fixJustifyOffset > 0)
            {
                lastX++;
                fixJustifyOffset--;
            }
        }

        const GraphicInternalFont::CharDescription& charDescription = iter->second;

        float32 width = charDescription.width * sizeScale;
        float32 startX = lastX + charDescription.xOffset * sizeScale;

        float32 startHeight = charDescription.yOffset * sizeScale;
        float32 fullHeight = (charDescription.height + charDescription.yOffset) * sizeScale;

        ascent = Min(startHeight, ascent);

        startHeight += yOffset;
        fullHeight += yOffset;

        metrics.drawRect.x = Min(metrics.drawRect.x, int32(startX));
        metrics.drawRect.y = Min(metrics.drawRect.y, int32(startHeight));
        metrics.drawRect.dx = Max(metrics.drawRect.dx, int32(startX + width));
        metrics.drawRect.dy = Max(metrics.drawRect.dy, int32(fullHeight));

        //const float32 borderAlign = (startHeight - yOffset)*2.0f;
        //metrics.drawRect.dy = Max(metrics.drawRect.dy, (int32)(fullHeight + borderAlign));

        if (vertexBuffer)
        {
            vertexBuffer[vertexAdded].position.x = startX;
            vertexBuffer[vertexAdded].position.y = startHeight;
            vertexBuffer[vertexAdded].position.z = 0;
            vertexBuffer[vertexAdded].texCoord.x = charDescription.u;
            vertexBuffer[vertexAdded].texCoord.y = charDescription.v;

            vertexBuffer[vertexAdded + 1].position.x = startX + width;
            vertexBuffer[vertexAdded + 1].position.y = startHeight;
            vertexBuffer[vertexAdded + 1].position.z = 0;
            vertexBuffer[vertexAdded + 1].texCoord.x = charDescription.u2;
            vertexBuffer[vertexAdded + 1].texCoord.y = charDescription.v;

            vertexBuffer[vertexAdded + 2].position.x = startX + width;
            vertexBuffer[vertexAdded + 2].position.y = fullHeight;
            vertexBuffer[vertexAdded + 2].position.z = 0;
            vertexBuffer[vertexAdded + 2].texCoord.x = charDescription.u2;
            vertexBuffer[vertexAdded + 2].texCoord.y = charDescription.v2;

            vertexBuffer[vertexAdded + 3].position.x = startX;
            vertexBuffer[vertexAdded + 3].position.y = fullHeight;
            vertexBuffer[vertexAdded + 3].position.z = 0;
            vertexBuffer[vertexAdded + 3].texCoord.x = charDescription.u;
            vertexBuffer[vertexAdded + 3].texCoord.y = charDescription.v2;
            vertexAdded += 4;
        }

        float32 nextKerning = 0;
        if (charPos + 1 < strLength)
        {
            auto i = charDescription.kerning.find(str.at(charPos + 1));
            if (i != charDescription.kerning.end())
            {
                nextKerning = i->second;
            }
        }
        float32 charWidth = (charDescription.xAdvance + nextKerning) * sizeScale;
        if (charSizes)
            charSizes->push_back(GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(charWidth));
        lastX += charWidth;

        charDrawed++;
    }
    lastY += yOffset + fontHeight;

    metrics.drawRect.dy += int32(ascent);

    //@note : "-1" fix magic fix from FTFont
    // Transform right/bottom edges into width/height
    metrics.drawRect.dx += -metrics.drawRect.x + 1;
    metrics.drawRect.dy += -metrics.drawRect.y + 1;

    metrics.height = lastY;
    metrics.width = lastX;
    metrics.baseline = yOffset + fontInternal->baselineHeight;
    return metrics;
}

float32 GraphicFont::GetSpread(float32 size) const
{
    return 0.25f / (fontInternal->spread * GetSizeScale(size));
}

float32 GraphicFont::GetSizeScale(float32 size) const
{
    return size / fontInternal->baseSize;
}

bool GraphicFont::LoadTexture(const FilePath& path)
{
    DVASSERT(texture == NULL);

    texture = Texture::CreateFromFile(path);
    if (!texture)
    {
        return false;
    }

    return true;
}

YamlNode* GraphicFont::SaveToYamlNode() const
{
    YamlNode* node = Font::SaveToYamlNode();
    //Type
    node->Set("type", "GraphicFont");

    String pathname = fontInternal->configPath.GetFrameworkPath();
    node->Set("name", pathname);

    return node;
}

const FilePath& GraphicFont::GetFontPath() const
{
    return fontInternal->configPath;
}

String GraphicFont::GetRawHashString()
{
    return fontInternal->configPath.GetFrameworkPath() + "_" + Font::GetRawHashString();
}
}
