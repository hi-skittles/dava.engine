#include "GraphicsLevel.h"
#include "Platform/DeviceInfo.h"

using namespace DAVA;

#define GET_BOOL(prop) ReadBool(prop, #prop, archive, node) 
#define GET_STRING(prop) ReadString(prop, #prop, archive, node)

GraphicsLevel::GraphicsLevel(const DAVA::String& fileName, YamlNode* node)
    : archive(new DAVA::KeyedArchive)
{
    vegetationAnimation = false;
    stencilShadows = true;

    ReadSettings(fileName, node);
}

GraphicsLevel::~GraphicsLevel()
{
}

void ReadString(String& var, const String& key, KeyedArchive* arch, DAVA::YamlNode* node)
{
    var = arch->GetString(key, var);
    if (!node)
        return;
    const auto& map = node->AsMap();
    auto fit = map.find(key);
    if (fit != map.end() && fit->second->GetType() == YamlNode::TYPE_MAP)
    {
        const auto& typeMap = fit->second->AsMap();
        auto fit2 = typeMap.find("string");
        if (fit2 != typeMap.end() && fit2->second->GetType() == YamlNode::TYPE_STRING)
        {
            var = fit2->second->AsString();
        }
    }
}

void ReadBool(bool& var, const String& key, KeyedArchive* arch, DAVA::YamlNode* node)
{
    var = arch->GetBool(key, var);
    if (!node)
        return;
    const auto& map = node->AsMap();
    auto fit = map.find(key);
    if (fit != map.end() && fit->second->GetType() == YamlNode::TYPE_MAP)
    {
        const auto& typeMap = fit->second->AsMap();
        auto fit2 = typeMap.find("bool");
        if (fit2 != typeMap.end() && fit2->second->GetType() == YamlNode::TYPE_STRING)
        {
            var = fit2->second->AsBool();
        }
    }
}

void GraphicsLevel::ReadSettings(const DAVA::String& fileName, DAVA::YamlNode* node)
{
    archive->DeleteAllKeys();

    String path = "~res:/GraphicSettings/" + fileName;
    archive->LoadFromYamlFile(path);

    GET_STRING(water);
    GET_STRING(vegetation);
    GET_STRING(tree_lighting);
    GET_STRING(landscape);
    GET_STRING(static_object);

    GET_BOOL(vegetationAnimation);
    GET_BOOL(stencilShadows);
}

void GraphicsLevel::Activate(void)
{
    QualitySettingsSystem* qs = QualitySettingsSystem::Instance();
    qs->SetCurMaterialQuality(FastName("Water"), FastName(water));
    qs->SetCurMaterialQuality(FastName("Vegetation"), FastName(vegetation));
    qs->SetCurMaterialQuality(FastName("Spherical Harmonics"), FastName(tree_lighting));
    qs->SetCurMaterialQuality(FastName("Landscape"), FastName(landscape));
    qs->SetCurMaterialQuality(FastName("Static object"), FastName(static_object));

    qs->EnableOption(QualitySettingsSystem::QUALITY_OPTION_VEGETATION_ANIMATION, vegetationAnimation);
    qs->EnableOption(QualitySettingsSystem::QUALITY_OPTION_STENCIL_SHADOW, stencilShadows);
    qs->SetCurTextureQuality(FastName(archive->GetString("textureQuality")));
}
