#include "UI/UIYamlLoader.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlEmitter.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlParser.h"
#include "Logger/Logger.h"
#include "Render/2D/FTFont.h"
#include "Render/2D/FontManager.h"
#include "Render/2D/GraphicFont.h"
#include "Render/2D/TextBlock.h"
#include "Time/SystemTimer.h"
#include "Utils/Utils.h"

namespace DAVA
{
const FontPreset& UIYamlLoader::GetFontPresetByName(const String& fontName) const
{
    return GetEngineContext()->fontManager->GetFontPreset(fontName);
}

void UIYamlLoader::LoadFonts(const FilePath& yamlPathname)
{
    ScopedPtr<UIYamlLoader> loader(new UIYamlLoader());
    YamlNode* rootNode = loader->CreateRootNode(yamlPathname);
    if (!rootNode)
    {
        // Empty YAML file.
        Logger::Warning("yaml file: %s is empty", yamlPathname.GetAbsolutePathname().c_str());
        return;
    }
    loader->LoadFontsFromNode(rootNode);
    SafeRelease(rootNode);
}

bool UIYamlLoader::SaveFonts(const FilePath& yamlPathname)
{
    const auto& fontMap = GetEngineContext()->fontManager->GetFontPresetMap();
    ScopedPtr<YamlNode> fontsNode(new YamlNode(YamlNode::TYPE_MAP));
    for (const auto& pair : fontMap)
    {
        const FontPreset& preset = pair.second;
        if (!preset.Valid())
            continue;
        fontsNode->AddNodeToMap(pair.first, CreateYamlNodeFromFontPreset(preset));
    }
    return YamlEmitter::SaveToYamlFile(yamlPathname, fontsNode, File::CREATE | File::WRITE);
}

YamlNode* UIYamlLoader::CreateRootNode(const FilePath& yamlPathname)
{
    RefPtr<YamlParser> parser = YamlParser::Create(yamlPathname);
    if (!parser)
    {
        Logger::Error("Failed to open yaml file: %s", yamlPathname.GetStringValue().c_str());
        return NULL;
    }
    YamlNode* rootNode = SafeRetain(parser->GetRootNode());
    return rootNode;
}

void UIYamlLoader::LoadFontsFromNode(const YamlNode* rootNode)
{
    FontManager* fontManager = GetEngineContext()->fontManager;
    for (auto t = rootNode->AsMap().begin(); t != rootNode->AsMap().end(); ++t)
    {
        YamlNode* node = t->second.Get();

        FontPreset preset = CreateFontPresetFromYamlNode(node);
        if (preset.Valid())
        {
            fontManager->SetFontPreset(preset, t->first);
        }
    }
}

FontPreset UIYamlLoader::CreateFontPresetFromYamlNode(const YamlNode* node)
{
    const YamlNode* typeNode = node->Get("type");
    if (!typeNode)
        return FontPreset::EMPTY;

    FontPreset preset;
    preset.SetSize(10.f); // Default size while loading

    // Font params
    const String& type = typeNode->AsString();
    if (type == "FTFont")
    {
        const YamlNode* fontNameNode = node->Get("name");
        if (!fontNameNode)
        {
            return FontPreset::EMPTY;
        }

        preset.SetFont(RefPtr<Font>(FTFont::Create(fontNameNode->AsString())));
    }
    else if (type == "GraphicFont")
    {
        const YamlNode* fontNameNode = node->Get("name");
        if (!fontNameNode)
        {
            return FontPreset::EMPTY;
        }

        const YamlNode* texNameNode = node->Get("texture");
        if (!texNameNode)
        {
            return FontPreset::EMPTY;
        }

        preset.SetFont(RefPtr<Font>(GraphicFont::Create(fontNameNode->AsString(), texNameNode->AsString())));
    }

    if (!preset.Valid())
    {
        return FontPreset::EMPTY;
    }

    const YamlNode* fontVerticalSpacingNode = node->Get("verticalSpacing");
    if (fontVerticalSpacingNode)
    {
        preset.GetFont()->SetVerticalSpacing(fontVerticalSpacingNode->AsInt32());
    }

    const YamlNode* fontFontAscendNode = node->Get("ascendScale");
    if (fontFontAscendNode)
    {
        preset.GetFont()->SetAscendScale(fontFontAscendNode->AsFloat());
    }

    const YamlNode* fontFontDescendNode = node->Get("descendScale");
    if (fontFontDescendNode)
    {
        preset.GetFont()->SetDescendScale(fontFontDescendNode->AsFloat());
    }

    // Preset params
    const YamlNode* fontSizeNode = node->Get("size");
    if (fontSizeNode)
    {
        preset.SetSize(fontSizeNode->AsFloat());
    }

    return preset;
}

YamlNode* UIYamlLoader::CreateYamlNodeFromFontPreset(const FontPreset& preset)
{
    if (!preset.Valid())
    {
        return nullptr;
    }

    YamlNode* node = preset.GetFont()->SaveToYamlNode();

    //Preset params
    node->Set("size", preset.GetSize());

    return node;
}
}
