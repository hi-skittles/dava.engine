#include "Base/Platform.h"
#include "FileSystem/KeyedArchive.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/FTFont.h"
#include "Render/2D/GraphicFont.h"
#include "Render/2D/Private/FTManager.h"
#include "Logger/Logger.h"
#include "Render/2D/Sprite.h"
#include "Utils/StringFormat.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"

#include "Engine/Engine.h"

namespace DAVA
{
namespace FontManagerDetails
{
static const String TTF_FONT_EXT = ".ttf";
static const String OTF_FONT_EXT = ".otf";
static const String FNT_FONT_EXT = ".fnt";
static const String FNTCONF_FONT_EXT = ".fntconf";
static const String TXT_FONT_EXT = ".txt";
static const String FT_FONT_TYPE = "ft";
static const String GRAPHIC_FONT_TYPE = "graphic";

struct FontConfigDescriptor
{
    FilePath fontPath;
    int32 verticalSpacing = 0;

    virtual ~FontConfigDescriptor() = default;

    virtual void Load(const YamlNode* node)
    {
        DVASSERT(node);

        const YamlNode* fontPathNode = node->Get("fontPath");
        if (fontPathNode)
        {
            fontPath = fontPathNode->AsString();
        }

        const YamlNode* verticalSpacingNode = node->Get("verticalSpacing");
        if (verticalSpacingNode)
        {
            verticalSpacing = verticalSpacingNode->AsInt32();
        }
    }

    virtual RefPtr<Font> CreateFont() const = 0;
};

struct FTFontConfigDescriptor : public FontConfigDescriptor
{
    float32 ascendScale = 1.f;
    float32 descendScale = 1.f;

    void Load(const YamlNode* node) override
    {
        FontConfigDescriptor::Load(node);

        const YamlNode* ascendScaleNode = node->Get("ascendScale");
        if (ascendScaleNode)
        {
            ascendScale = ascendScaleNode->AsFloat();
        }

        const YamlNode* descendScaleNode = node->Get("descendScale");
        if (descendScaleNode)
        {
            descendScale = descendScaleNode->AsFloat();
        }
    }

    RefPtr<Font> CreateFont() const override
    {
        RefPtr<Font> f(FTFont::Create(fontPath));
        if (f)
        {
            f->SetVerticalSpacing(verticalSpacing);
            f->SetAscendScale(ascendScale);
            f->SetDescendScale(descendScale);
        }
        return f;
    }
};

struct GraphicFontConfigDescriptor : public FontConfigDescriptor
{
    FilePath texturePath;

    void Load(const YamlNode* node) override
    {
        FontConfigDescriptor::Load(node);

        const YamlNode* texturePathNode = node->Get("texturePath");
        if (texturePathNode)
        {
            texturePath = texturePathNode->AsString();
        }
    }

    RefPtr<Font> CreateFont() const override
    {
        RefPtr<Font> f;
        if (texturePath.Exists())
        {
            f.Set(GraphicFont::Create(fontPath, texturePath));
        }
        else
        {
            FilePath guessTextPath(fontPath);
            guessTextPath.ReplaceExtension(TXT_FONT_EXT);
            if (guessTextPath.Exists())
            {
                f.Set(GraphicFont::Create(fontPath, guessTextPath));
            }
        }
        return f;
    }
};
}

FontManager::FontManager()
    : ftmanager(std::make_unique<FTManager>())
{
}

FontManager::~FontManager()
{
    FTFont::ClearCache();
    UnregisterFontsPresets();
}

RefPtr<Font> FontManager::LoadFont(const FilePath& fontPath)
{
    using namespace FontManagerDetails;

    const String& ext = fontPath.GetExtension();
    if (ext == FNTCONF_FONT_EXT)
    {
        auto it = fontConfigs.find(fontPath.GetStringValue());
        if (it != fontConfigs.end())
        {
            const std::unique_ptr<FontConfigDescriptor>& desc = it->second;
            return desc->CreateFont();
        }
        else
        {
            RefPtr<YamlParser> parser = YamlParser::Create(fontPath);
            if (parser)
            {
                const YamlNode* node = parser->GetRootNode();
                if (node)
                {
                    const YamlNode* typeNode = node->Get("type");
                    if (typeNode)
                    {
                        std::unique_ptr<FontConfigDescriptor> desc;
                        String fontType = typeNode->AsString();
                        if (fontType == FT_FONT_TYPE)
                        {
                            desc = std::make_unique<FTFontConfigDescriptor>();
                        }
                        else if (fontType == GRAPHIC_FONT_TYPE)
                        {
                            desc = std::make_unique<GraphicFontConfigDescriptor>();
                        }
                        else
                        {
                            Logger::Warning("Unknown font type in font config descriptor! (%s)", fontType.c_str());
                            DVASSERT(false, "Unknown font type in font config descriptor!");
                        }

                        desc->Load(node);
                        RefPtr<Font> font = desc->CreateFont();
                        fontConfigs[fontPath.GetStringValue()] = std::move(desc);
                        return font;
                    }
                    else
                    {
                        Logger::Warning("Font type in font config descriptor not found!");
                        DVASSERT(false, "Font type in font config descriptor not found!");
                    }
                }
            }
            else
            {
                Logger::Warning("Wrong font config descriptor file format!");
                DVASSERT(false, "Wrong font config descriptor file format!");
            }
        }
    }
    else if (ext == TTF_FONT_EXT || ext == OTF_FONT_EXT)
    {
        return RefPtr<Font>(FTFont::Create(fontPath));
    }
    else if (ext == FNT_FONT_EXT)
    {
        FilePath texPath(fontPath);
        texPath.ReplaceExtension(TXT_FONT_EXT);
        return RefPtr<Font>(GraphicFont::Create(fontPath, texPath));
    }
    else
    {
        Logger::Warning("Unknown font file extension! (%s)", ext.c_str());
        DVASSERT(false, "Unknown font file extension!");
    }
    return RefPtr<Font>();
}

void FontManager::RegisterFont(Font* font)
{
    if (!Engine::Instance()->GetOptions()->GetBool("trackFont"))
        return;

    if (registeredFonts.find(font) == registeredFonts.end())
    {
        registeredFonts.insert({ font, DAVA::String() });
    }
}

void FontManager::UnregisterFont(Font* font)
{
    registeredFonts.erase(font);
}

void FontManager::RegisterFontsPresets(const UnorderedMap<String, FontPreset>& presets)
{
    UnregisterFontsPresets();
    fontPresetMap = presets;
}

void FontManager::UnregisterFontsPresets()
{
    registeredFonts.clear();
    fontPresetMap.clear();
}

void FontManager::SetFontPreset(const FontPreset& preset, const String& name)
{
    fontPresetMap[name] = preset;

    // check if font already registered
    if (registeredFonts.find(preset.GetFontPtr()) != registeredFonts.end())
    {
        registeredFonts[preset.GetFontPtr()] = name;
    }
}

String FontManager::GetFontPresetName(const FontPreset& preset) const
{
    auto it = std::find_if(fontPresetMap.begin(), fontPresetMap.end(), [preset](const std::pair<String, FontPreset>& p)
                           {
                               return p.second == preset;
                           });
    if (it != fontPresetMap.end())
    {
        return it->first;
    }
    return String();
}

const FontPreset& FontManager::GetFontPreset(const String& name) const
{
    auto it = fontPresetMap.find(name);
    if (it != fontPresetMap.end())
    {
        return it->second;
    }
    return FontPreset::EMPTY;
}

const UnorderedMap<Font*, String>& FontManager::GetRegisteredFonts() const
{
    return registeredFonts;
}

String FontManager::GetFontHashName(Font* font) const
{
    return Format("Font_%X", font->GetHashCode());
}

const UnorderedMap<String, FontPreset>& FontManager::GetFontPresetMap() const
{
    return fontPresetMap;
}
};
