#include "ApplyMaterialPresetCommand.h"

#include "Classes/Commands2/RECommandIDs.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include "TArc/DataProcessing/DataContext.h"

#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/SceneFile/VersionInfo.h"
#include "Render/Material/NMaterial.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/FilePath.h"
#include "Functional/Function.h"

namespace ApplyMaterialPresetDetail
{
using namespace DAVA;
const String contentNodeName("content");

template <typename T>
void ClearContent(const Function<const UnorderedMap<FastName, T>&()>& getContentFn, const Function<void(const FastName&)>& removeFn)
{
    // We have to clear local content before applying undo snapshot, because applying will not delete fields
    // Snapshot contains only info about "exists fields", but not containfo about "not exists fields"
    UnorderedMap<FastName, T> info = getContentFn();
    for (const auto& item : info)
    {
        removeFn(item.first);
    }
}

void StoreMaterialTextures(const MaterialConfig& config, KeyedArchive* content, const SerializationContext& context)
{
    for (const auto& lc : config.localTextures)
    {
        const FilePath& texturePath = lc.second->path;
        if (!texturePath.IsEmpty())
        {
            String textureRelativePath = texturePath.GetRelativePathname(context.GetScenePath());
            if (textureRelativePath.size() > 0)
            {
                content->SetString(lc.first.c_str(), textureRelativePath);
            }
        }
    }
}

void StoreMaterialFlags(const MaterialConfig& config, KeyedArchive* content)
{
    for (const auto& lf : config.localFlags)
    {
        content->SetInt32(lf.first.c_str(), lf.second);
    }
}

void StoreMaterialProperties(const MaterialConfig& config, KeyedArchive* content)
{
    for (const auto& lp : config.localProperties)
    {
        rhi::ShaderProp::Type propertyType = lp.second->type;
        const float32* propertyValue = lp.second->data.get();
        uint32 arraySize = lp.second->arraySize;
        uint32 dataSize = static_cast<uint32>(sizeof(float32) * ShaderDescriptor::CalculateDataSize(propertyType, 1));

        ScopedPtr<KeyedArchive> prop(new KeyedArchive());
        prop->SetUInt32("type", static_cast<uint32>(propertyType));
        prop->SetUInt32("size", arraySize);
        prop->SetByteArray("data", reinterpret_cast<const uint8*>(propertyValue), dataSize);
        content->SetArchive(lp.first.c_str(), prop);
    }
}

void UpdateMaterialPropertiesFromPreset(NMaterial* material, KeyedArchive* content, bool updateForUndo)
{
    if (updateForUndo)
    {
        ClearContent(MakeFunction(material, &NMaterial::GetLocalProperties), MakeFunction(material, &NMaterial::RemoveProperty));
    }

    const KeyedArchive::UnderlyingMap& properties = content->GetArchieveData();
    for (const auto& pm : properties)
    {
        DVASSERT(VariantType::TYPE_KEYED_ARCHIVE == pm.second->type);

        FastName propName(pm.first);
        KeyedArchive* propertyArchive = pm.second->AsKeyedArchive();

        /*
        * Here we are checking if propData if valid, because yaml parser can
        * completely delete (skip) byte array node if it contains invalid data
        */
        const float32* propData = reinterpret_cast<const float32*>(propertyArchive->GetByteArray("data"));
        if (nullptr != propData)
        {
            rhi::ShaderProp::Type propType = static_cast<rhi::ShaderProp::Type>(propertyArchive->GetUInt32("type"));
            uint32 propSize = propertyArchive->GetUInt32("size");

            if (material->HasLocalProperty(propName))
            {
                rhi::ShaderProp::Type existingType = material->GetLocalPropType(propName);
                DAVA::uint32 existingSize = material->GetLocalPropArraySize(propName);
                if ((existingType == propType) && (existingSize == propSize))
                {
                    material->SetPropertyValue(propName, propData);
                }
            }
            else
            {
                material->AddProperty(propName, propData, propType, propSize);
            }
        }
    }
}

void UpdateMaterialPropertiesFromPreset(MaterialConfig& config, KeyedArchive* content)
{
    const KeyedArchive::UnderlyingMap& properties = content->GetArchieveData();
    for (const auto& pm : properties)
    {
        DVASSERT(VariantType::TYPE_KEYED_ARCHIVE == pm.second->type);

        FastName propName(pm.first);
        KeyedArchive* propertyArchive = pm.second->AsKeyedArchive();

        const float32* propData = reinterpret_cast<const float32*>(propertyArchive->GetByteArray("data"));
        if (nullptr != propData)
        {
            NMaterialProperty* prop = new NMaterialProperty();
            prop->name = propName;
            prop->type = static_cast<rhi::ShaderProp::Type>(propertyArchive->GetUInt32("type"));
            prop->arraySize = propertyArchive->GetUInt32("size");
            prop->data.reset(new float[ShaderDescriptor::CalculateDataSize(prop->type, prop->arraySize)]);
            prop->SetPropertyValue(propData);
            config.localProperties[propName] = prop;
        }
    }
}

void UpdateMaterialFlagsFromPreset(NMaterial* material, KeyedArchive* content, bool updateForUndo)
{
    if (updateForUndo)
    {
        ClearContent(MakeFunction(material, &NMaterial::GetLocalFlags), MakeFunction(material, &NMaterial::RemoveFlag));
    }

    const auto flags = content->GetArchieveData();
    for (const auto& fm : flags)
    {
        if (material->HasLocalFlag(FastName(fm.first)))
            material->SetFlag(FastName(fm.first), fm.second->AsInt32());
        else
            material->AddFlag(FastName(fm.first), fm.second->AsInt32());
    }
}

void UpdateMaterialFlagsFromPreset(MaterialConfig& config, KeyedArchive* content)
{
    const auto flags = content->GetArchieveData();
    for (const auto& fm : flags)
    {
        config.localFlags[FastName(fm.first)] = fm.second->AsInt32();
    }
}

void UpdateMaterialTexturesFromPreset(NMaterial* material, KeyedArchive* content, const FilePath& scenePath, bool updateForUndo)
{
    if (updateForUndo)
    {
        ClearContent(MakeFunction(material, &NMaterial::GetLocalTextures), MakeFunction(material, &NMaterial::RemoveTexture));
    }

    const auto& texturesMap = content->GetArchieveData();
    for (const auto& tm : texturesMap)
    {
        ScopedPtr<Texture> texture(Texture::CreateFromFile(scenePath + tm.second->AsString()));

        FastName textureName(tm.first);
        if (material->HasLocalTexture(textureName))
        {
            material->SetTexture(textureName, texture);
        }
        else
        {
            material->AddTexture(textureName, texture);
        }
    }
}

void UpdateMaterialTexturesFromPreset(MaterialConfig& config, KeyedArchive* content, const FilePath& scenePath)
{
    const auto& texturesMap = content->GetArchieveData();
    for (const auto& tm : texturesMap)
    {
        MaterialTextureInfo* texInfo = new MaterialTextureInfo();
        texInfo->texture = Texture::CreateFromFile(scenePath + tm.second->AsString());
        texInfo->path = texInfo->texture->GetPathname();
        config.localTextures[FastName(tm.first)] = texInfo;
    }
}

bool FlagEnabled(uint32 value, uint32 flag)
{
    return (value & flag) == flag;
}

void LoadMaterialPresetOld(KeyedArchive* preset, NMaterial* material, const SerializationContext& context, uint32 parts, bool loadForUndo)
{
    if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEMPLATE) && preset->IsKeyExists("fxname"))
    {
        material->SetFXName(preset->GetFastName("fxname"));
    }

    if (FlagEnabled(parts, ApplyMaterialPresetCommand::PROPERTIES))
    {
        if (preset->IsKeyExists("flags"))
        {
            UpdateMaterialFlagsFromPreset(material, preset->GetArchive("flags"), loadForUndo);
        }

        if (preset->IsKeyExists("properties"))
        {
            UpdateMaterialPropertiesFromPreset(material, preset->GetArchive("properties"), loadForUndo);
        }
    }

    if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEXTURES) && preset->IsKeyExists("textures"))
    {
        UpdateMaterialTexturesFromPreset(material, preset->GetArchive("textures"), context.GetScenePath(), loadForUndo);
    }
}

Vector<MaterialConfig> LoadMaterialConfigs(KeyedArchive* preset, NMaterial* material, const SerializationContext& context, uint32 parts)
{
    Vector<MaterialConfig> newConfigs;

    uint32 newConfigCount = preset->GetUInt32("configCount");
    if (newConfigCount > 0)
    {
        newConfigs.resize(newConfigCount);
        for (uint32 ci = 0; ci < newConfigCount; ++ci)
        {
            MaterialConfig& config = newConfigs[ci];

            String nameString = Format("name_%d", ci);
            if (preset->IsKeyExists(nameString))
            {
                config.name = preset->GetFastName(nameString);
            }

            if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEMPLATE))
            {
                String fxNameString = Format("fxname_%d", ci);
                if (preset->IsKeyExists(fxNameString))
                {
                    config.fxName = preset->GetFastName(fxNameString);
                }
            }

            if (FlagEnabled(parts, ApplyMaterialPresetCommand::PROPERTIES))
            {
                String flagsString = Format("flags_%d", ci);
                if (preset->IsKeyExists(flagsString))
                {
                    UpdateMaterialFlagsFromPreset(config, preset->GetArchive(flagsString));
                }

                String propertiesString = Format("properties_%d", ci);
                if (preset->IsKeyExists(propertiesString))
                {
                    UpdateMaterialPropertiesFromPreset(config, preset->GetArchive(propertiesString));
                }
            }

            if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEXTURES))
            {
                String texturesString = Format("textures_%d", ci);
                if (preset->IsKeyExists(texturesString))
                {
                    UpdateMaterialTexturesFromPreset(config, preset->GetArchive(texturesString), context.GetScenePath());
                }
            }
        }
    }

    return newConfigs;
}

void LoadMaterialPreset(KeyedArchive* archive, NMaterial* material, const SerializationContext& context, uint32 parts, bool loadForUndo)
{
    KeyedArchive* preset = archive->GetArchive(contentNodeName);

    if (FlagEnabled(parts, ApplyMaterialPresetCommand::GROUP) && preset->IsKeyExists("group"))
    {
        material->SetQualityGroup(preset->GetFastName("group"));
    }

    if (preset->IsKeyExists("configCount"))
    {
        Vector<MaterialConfig> newConfigs = LoadMaterialConfigs(preset, material, context, parts);

        uint32 newConfigCount = static_cast<uint32>(newConfigs.size());
        if (newConfigCount != 0)
        {
            uint32 prevConfigsCount = material->GetConfigCount(); //we need this thick because of material couldn't be without any config
            if (loadForUndo)
            {
                for (uint32 ci = 0; ci < newConfigCount; ++ci)
                {
                    material->InsertConfig(prevConfigsCount + ci, newConfigs[ci]); //add new configs to the end of container
                }

                while (prevConfigsCount-- > 0)
                {
                    material->RemoveConfig(0); //remove stored configs
                }
            }
            else
            {
                for (uint32 ci = 0; ci < newConfigCount; ++ci)
                {
                    uint32 index = material->FindConfigByName(newConfigs[ci].name);
                    if (index < prevConfigsCount)
                    {
                        MaterialConfig config = material->GetConfig(index);
                        if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEMPLATE))
                        {
                            config.fxName = newConfigs[ci].fxName;
                        }

                        if (FlagEnabled(parts, ApplyMaterialPresetCommand::PROPERTIES))
                        {
                            config.localFlags = newConfigs[ci].localFlags;

                            //release old properties
                            for (auto& prop : config.localProperties)
                            {
                                SafeDelete(prop.second);
                            }
                            config.localProperties.clear();

                            //copy new properties
                            for (auto& prop : newConfigs[ci].localProperties)
                            {
                                NMaterialProperty* newProp = new NMaterialProperty();
                                newProp->name = prop.first;
                                newProp->type = prop.second->type;
                                newProp->arraySize = prop.second->arraySize;
                                newProp->data.reset(new float[ShaderDescriptor::CalculateDataSize(newProp->type, newProp->arraySize)]);
                                newProp->SetPropertyValue(prop.second->data.get());
                                config.localProperties[newProp->name] = newProp;
                            }
                        }

                        if (FlagEnabled(parts, ApplyMaterialPresetCommand::TEXTURES))
                        {
                            // remove old textures
                            for (auto& texInfo : config.localTextures)
                            {
                                SafeRelease(texInfo.second->texture);
                                SafeDelete(texInfo.second);
                            }
                            config.localTextures.clear();

                            //copy new textures
                            for (auto& tex : newConfigs[ci].localTextures)
                            {
                                MaterialTextureInfo* texInfo = new MaterialTextureInfo();
                                texInfo->path = tex.second->path;
                                texInfo->texture = SafeRetain(tex.second->texture);
                                config.localTextures[tex.first] = texInfo;
                            }
                        }

                        material->InsertConfig(index + 1, config); //add new configs at proper position
                        material->RemoveConfig(index);
                    }
                    else
                    {
                        material->InsertConfig(prevConfigsCount + ci, newConfigs[ci]); //add new configs to the end of container
                    }
                }
            }
        }

        material->SetCurrentConfigIndex(preset->GetUInt32("currentConfig"));
        material->InvalidateRenderVariants();
    }
    else
    {
        LoadMaterialPresetOld(preset, material, context, parts, loadForUndo);
    }
}
}

ApplyMaterialPresetCommand::ApplyMaterialPresetCommand(const DAVA::FilePath& presetPath, DAVA::NMaterial* material_, DAVA::Scene* scene_)
    : RECommand(CMDID_MATERIAL_APPLY_PRESET, "Apply material preset")
    , redoInfo(new DAVA::KeyedArchive())
    , undoInfo(new DAVA::KeyedArchive())
    , material(DAVA::RefPtr<DAVA::NMaterial>::ConstructWithRetain(material_))
    , scene(scene_)
{
    redoInfo->LoadFromYamlFile(presetPath);
}

bool ApplyMaterialPresetCommand::IsValidPreset() const
{
    return redoInfo->IsKeyExists(ApplyMaterialPresetDetail::contentNodeName);
}

void ApplyMaterialPresetCommand::Init(DAVA::uint32 materialParts_)
{
    materialParts = materialParts_;
    DAVA::SerializationContext context;
    PrepareSerializationContext(context);
    StoreAllConfigsPreset(undoInfo.get(), material.Get(), context);

    DVASSERT(undoInfo->IsKeyExists(ApplyMaterialPresetDetail::contentNodeName));
}

void ApplyMaterialPresetCommand::Undo()
{
    LoadMaterialPreset(undoInfo.get(), ALL, true);
}

bool ApplyMaterialPresetCommand::IsClean() const
{
    return false;
}

void ApplyMaterialPresetCommand::Redo()
{
    LoadMaterialPreset(redoInfo.get(), materialParts, false);
}

void ApplyMaterialPresetCommand::StoreCurrentConfigPreset(DAVA::KeyedArchive* preset, DAVA::NMaterial* material, const DAVA::SerializationContext& context)
{
    using namespace ApplyMaterialPresetDetail;

    DAVA::ScopedPtr<DAVA::KeyedArchive> content(new DAVA::KeyedArchive());
    DAVA::ScopedPtr<DAVA::KeyedArchive> texturesArchive(new DAVA::KeyedArchive());
    DAVA::ScopedPtr<DAVA::KeyedArchive> flagsArchive(new DAVA::KeyedArchive());
    DAVA::ScopedPtr<DAVA::KeyedArchive> propertiesArchive(new DAVA::KeyedArchive());

    const MaterialConfig& config = material->GetConfig(material->GetCurrentConfigIndex());

    StoreMaterialTextures(config, texturesArchive, context);
    StoreMaterialFlags(config, flagsArchive);
    StoreMaterialProperties(config, propertiesArchive);

    content->SetArchive("flags", flagsArchive);
    content->SetArchive("textures", texturesArchive);
    content->SetArchive("properties", propertiesArchive);

    const FastName& fxName = material->GetLocalFXName();
    if (fxName.IsValid())
        content->SetFastName("fxname", fxName);

    const FastName& qualityGroup = material->GetQualityGroup();
    if (qualityGroup.IsValid())
        content->SetFastName("group", qualityGroup);

    preset->SetUInt32("serializationContextVersion", context.GetVersion());
    preset->SetArchive("content", content);
}

void ApplyMaterialPresetCommand::LoadMaterialPreset(DAVA::KeyedArchive* archive, DAVA::uint32 parts, bool loadForUndo)
{
    DAVA::SerializationContext context;
    PrepareSerializationContext(context);
    ApplyMaterialPresetDetail::LoadMaterialPreset(archive, material.Get(), context, parts, loadForUndo);
    context.ResolveMaterialBindings();
}

void ApplyMaterialPresetCommand::StoreAllConfigsPreset(DAVA::KeyedArchive* archive, DAVA::NMaterial* material, const DAVA::SerializationContext& context)
{
    using namespace ApplyMaterialPresetDetail;
    using namespace DAVA;

    ScopedPtr<KeyedArchive> content(new KeyedArchive());
    archive->SetUInt32("serializationContextVersion", context.GetVersion());

    const FastName& qualityGroup = material->GetQualityGroup();
    if (qualityGroup.IsValid())
    {
        content->SetFastName("group", qualityGroup);
    }

    uint32 configCount = material->GetConfigCount();
    content->SetUInt32("configCount", configCount);

    for (uint32 ci = 0; ci < configCount; ++ci)
    {
        const MaterialConfig& config = material->GetConfig(ci);

        if (config.name.IsValid())
        {
            content->SetFastName(Format("name_%d", ci), config.name);
        }

        if (config.fxName.IsValid())
        {
            content->SetFastName(Format("fxname_%d", ci), config.fxName);
        }

        ScopedPtr<KeyedArchive> texturesArchive(new KeyedArchive());
        ScopedPtr<KeyedArchive> flagsArchive(new KeyedArchive());
        ScopedPtr<KeyedArchive> propertiesArchive(new KeyedArchive());

        StoreMaterialTextures(config, texturesArchive, context);
        StoreMaterialFlags(config, flagsArchive);
        StoreMaterialProperties(config, propertiesArchive);

        content->SetArchive(Format("flags_%d", ci), flagsArchive);
        content->SetArchive(Format("textures_%d", ci), texturesArchive);
        content->SetArchive(Format("properties_%d", ci), propertiesArchive);
    }

    uint32 currentConfig = material->GetCurrentConfigIndex();
    content->SetUInt32("currentConfig", currentConfig);

    archive->SetArchive("content", content);
}

void ApplyMaterialPresetCommand::PrepareSerializationContext(DAVA::SerializationContext& context)
{
    context.SetScene(scene);

    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);
    context.SetScenePath(data->GetProjectPath());
    context.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);
}
