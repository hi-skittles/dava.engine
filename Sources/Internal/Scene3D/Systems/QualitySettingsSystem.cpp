#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Scene3D/Components/QualitySettingsComponent.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Scene.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Render/Highlevel/RenderObject.h"
#include "Logger/Logger.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
const FastName QualitySettingsSystem::QUALITY_OPTION_VEGETATION_ANIMATION("Vegetation Animation");
const FastName QualitySettingsSystem::QUALITY_OPTION_STENCIL_SHADOW("Stencil Shadows");
const FastName QualitySettingsSystem::QUALITY_OPTION_WATER_DECORATIONS("Water Decorations");
const FastName QualitySettingsSystem::QUALITY_OPTION_DISABLE_EFFECTS("Disable effects");
const FastName QualitySettingsSystem::QUALITY_OPTION_LOD0_EFFECTS("Lod0 effects");

const FastName QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG("Disable fog");
const FastName QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_ATMOSPHERE_ATTENUATION("Disable fog attenuation");
const FastName QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_ATMOSPHERE_SCATTERING("Disable fog scattering");
const FastName QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_HALF_SPACE("Disable half-space fog");

rhi::AntialiasingType AANameToType(const FastName& name);

QualitySettingsSystem::QualitySettingsSystem()
{
    Load("~res:/quality.yaml");

    EnableOption(QUALITY_OPTION_VEGETATION_ANIMATION, true);
    EnableOption(QUALITY_OPTION_STENCIL_SHADOW, true);
    EnableOption(QUALITY_OPTION_WATER_DECORATIONS, false);
}

void QualitySettingsSystem::Load(const FilePath& path)
{
    if (GetEngineContext()->fileSystem->Exists(path))
    {
        RefPtr<YamlParser> parser(YamlParser::Create(path));
        YamlNode* rootNode = parser->GetRootNode();

        if (NULL != rootNode)
        {
            textureQualities.clear();
            materialGroups.clear();
            soundQualities.clear();
            landscapeQualities.clear();
            anisotropyQualities.clear();
            msaaQualities.clear();

            // materials
            const YamlNode* materialGroupsNode = rootNode->Get("materials");
            if (NULL != materialGroupsNode)
            {
                for (uint32 i = 0; i < materialGroupsNode->GetCount(); ++i)
                {
                    const YamlNode* groupNode = materialGroupsNode->Get(i);
                    const YamlNode* name = groupNode->Get("group");
                    const YamlNode* values = groupNode->Get("quality");
                    const YamlNode* deflt = groupNode->Get("default");

                    FastName defQualityName;
                    if (NULL != deflt && deflt->GetType() == YamlNode::TYPE_STRING)
                    {
                        defQualityName = FastName(deflt->AsString().c_str());
                    }

                    if (NULL != name && NULL != values &&
                        name->GetType() == YamlNode::TYPE_STRING &&
                        values->GetType() == YamlNode::TYPE_ARRAY)
                    {
                        const auto& v = values->AsVector();

                        MAGrQ maGr;
                        maGr.curQuality = 0;
                        maGr.qualities.reserve(v.size());

                        for (size_t j = 0; j < v.size(); ++j)
                        {
                            if (v[j]->GetType() == YamlNode::TYPE_STRING)
                            {
                                MaterialQuality mq;
                                mq.weight = j;
                                mq.qualityName = FastName(v[j]->AsString().c_str());

                                maGr.qualities.push_back(mq);

                                if (mq.qualityName == defQualityName)
                                {
                                    maGr.curQuality = j;
                                }
                            }
                        }

                        String nodeName = name->AsString();
                        FastName materialKey = FastName(nodeName);
                        materialGroups[materialKey] = std::move(maGr);
                    }
                }
            }

            // textures
            const YamlNode* texturesNode = rootNode->Get("textures");
            if (NULL != texturesNode)
            {
                const YamlNode* defltTx = texturesNode->Get("default");
                const YamlNode* qualitiesNode = texturesNode->Get("qualities");

                if (NULL != qualitiesNode)
                {
                    FastName defTxQualityName;
                    if (NULL != defltTx && defltTx->GetType() == YamlNode::TYPE_STRING)
                    {
                        defTxQualityName = FastName(defltTx->AsString().c_str());
                    }

                    textureQualities.reserve(qualitiesNode->GetCount());
                    for (uint32 i = 0; i < qualitiesNode->GetCount(); ++i)
                    {
                        const YamlNode* qualityNode = qualitiesNode->Get(i);
                        const YamlNode* name = qualityNode->Get("quality");
                        const YamlNode* albedoNode = qualityNode->Get("albedo");
                        const YamlNode* normalNode = qualityNode->Get("normalmap");
                        const YamlNode* sizeNode = qualityNode->Get("minsize");

                        if (NULL != name && name->GetType() == YamlNode::TYPE_STRING &&
                            NULL != albedoNode && albedoNode->GetType() == YamlNode::TYPE_STRING &&
                            NULL != normalNode && normalNode->GetType() == YamlNode::TYPE_STRING &&
                            NULL != sizeNode && sizeNode->GetType() == YamlNode::TYPE_STRING)
                        {
                            TXQ txq;

                            txq.name = FastName(name->AsString().c_str());
                            txq.quality.weight = i;
                            txq.quality.albedoBaseMipMapLevel = albedoNode->AsUInt32();
                            txq.quality.normalBaseMipMapLevel = normalNode->AsUInt32();
                            txq.quality.minSize = Vector2(sizeNode->AsFloat(), sizeNode->AsFloat());

                            textureQualities.push_back(txq);

                            if (txq.name == defTxQualityName)
                            {
                                curTextureQuality = i;
                            }
                        }
                    }
                }
            }

            // anisotropy
            const YamlNode* anisotropyNode = rootNode->Get("anisotropy");
            if (anisotropyNode != nullptr)
            {
                const YamlNode* defltAn = anisotropyNode->Get("default");
                const YamlNode* qualitiesNode = anisotropyNode->Get("qualities");

                if (qualitiesNode != nullptr)
                {
                    FastName defAnQualityName;
                    if (defltAn != nullptr && defltAn->GetType() == YamlNode::TYPE_STRING)
                    {
                        defAnQualityName = FastName(defltAn->AsString().c_str());
                    }

                    anisotropyQualities.reserve(qualitiesNode->GetCount());
                    for (uint32 i = 0; i < qualitiesNode->GetCount(); ++i)
                    {
                        const YamlNode* qualityNode = qualitiesNode->Get(i);
                        const YamlNode* name = qualityNode->Get("quality");
                        const YamlNode* maxNode = qualityNode->Get("max");

                        if ((name != nullptr) && (name->GetType() == YamlNode::TYPE_STRING) &&
                            (maxNode != nullptr) && (maxNode->GetType() == YamlNode::TYPE_STRING))
                        {
                            ANQ anq;
                            anq.name = FastName(name->AsString().c_str());
                            anq.quality.weight = i;
                            anq.quality.maxAnisotropy = maxNode->AsUInt32();
                            anisotropyQualities.push_back(anq);
                            if (anq.name == defAnQualityName)
                            {
                                curAnisotropyQuality = i;
                            }
                        }
                    }
                }
            }

            // msaa
            const YamlNode* msaaNode = rootNode->Get("msaa");
            if (msaaNode != nullptr)
            {
                const YamlNode* qualitiesNode = msaaNode->Get("qualities");

                if (qualitiesNode != nullptr)
                {
                    FastName defaultName;

                    const YamlNode* defltAA = msaaNode->Get("default");
                    if ((defltAA != nullptr) && (defltAA->GetType() == YamlNode::TYPE_STRING))
                    {
                        defaultName = FastName(defltAA->AsString().c_str());
                    }

                    msaaQualities.reserve(qualitiesNode->GetCount());
                    for (uint32 i = 0; i < qualitiesNode->GetCount(); ++i)
                    {
                        const YamlNode* qualityNode = qualitiesNode->Get(i);
                        const YamlNode* name = qualityNode->Get("quality");
                        if ((name != nullptr) && (name->GetType() == YamlNode::TYPE_STRING))
                        {
                            FastName qName(name->AsString().c_str());
                            msaaQualities.emplace_back(qName, i, AANameToType(qName));
                            if (qName == defaultName)
                            {
                                curMSAAQuality = i;
                            }
                        }
                    }
                }
            }

            // sound
            const YamlNode* soundsNode = rootNode->Get("sounds");
            if (NULL != soundsNode)
            {
                const YamlNode* defltSfx = soundsNode->Get("default");
                const YamlNode* qualitiesNode = soundsNode->Get("qualities");

                if (NULL != qualitiesNode)
                {
                    FastName defSfxQualityName;
                    if (NULL != defltSfx && defltSfx->GetType() == YamlNode::TYPE_STRING)
                    {
                        defSfxQualityName = FastName(defltSfx->AsString().c_str());
                    }

                    soundQualities.reserve(qualitiesNode->GetCount());
                    for (uint32 i = 0; i < qualitiesNode->GetCount(); ++i)
                    {
                        const YamlNode* qualityNode = qualitiesNode->Get(i);
                        const YamlNode* name = qualityNode->Get("quality");
                        const YamlNode* confgNode = qualityNode->Get("configPath");

                        if (NULL != name && name->GetType() == YamlNode::TYPE_STRING &&
                            NULL != confgNode && confgNode->GetType() == YamlNode::TYPE_STRING)
                        {
                            SFXQ sfxq;

                            sfxq.name = FastName(name->AsString().c_str());
                            sfxq.configPath = FilePath(confgNode->AsString());

                            soundQualities.push_back(sfxq);

                            if (sfxq.name == defSfxQualityName)
                            {
                                curSoundQuality = i;
                            }
                        }
                    }
                }
            }

            // landscape
            const YamlNode* landscapeNode = rootNode->Get("landscape");
            if (nullptr != landscapeNode)
            {
                const YamlNode* defaultNode = landscapeNode->Get("default");
                const YamlNode* qualitiesNode = landscapeNode->Get("qualities");

                if (nullptr != qualitiesNode)
                {
                    FastName defQualityName;
                    if (nullptr != defaultNode && defaultNode->GetType() == YamlNode::TYPE_STRING)
                    {
                        defQualityName = FastName(defaultNode->AsString().c_str());
                    }

                    landscapeQualities.reserve(qualitiesNode->GetCount());
                    for (uint32 i = 0; i < qualitiesNode->GetCount(); ++i)
                    {
                        const YamlNode* qualityNode = qualitiesNode->Get(i);
                        const YamlNode* nameNode = qualityNode->Get("quality");
                        const YamlNode* morphingNode = qualityNode->Get("morphing");
                        const YamlNode* metricsNodes[] = {
                            qualityNode->Get("normalMaxHeightError"),
                            qualityNode->Get("normalMaxPatchRadiusError"),
                            qualityNode->Get("normalMaxAbsoluteHeightError"),
                            qualityNode->Get("zoomMaxHeightError"),
                            qualityNode->Get("zoomMaxPatchRadiusError"),
                            qualityNode->Get("zoomMaxAbsoluteHeightError")
                        };

                        if (nameNode && nameNode->GetType() == YamlNode::TYPE_STRING &&
                            morphingNode && morphingNode->GetType() == YamlNode::TYPE_STRING)
                        {
                            bool metrcisValid = true;
                            LCQ lcq;
                            lcq.name = FastName(nameNode->AsString().c_str());
                            lcq.quality.morphing = morphingNode->AsBool();

                            for (size_t j = 0; j < 6; j++)
                            {
                                if (metricsNodes[j] && metricsNodes[j]->GetType() == YamlNode::TYPE_STRING)
                                {
                                    lcq.quality.metricsArray[j] = metricsNodes[j]->AsFloat();
                                }
                                else
                                {
                                    metrcisValid = false;
                                    break;
                                }
                            }

                            if (metrcisValid)
                            {
                                landscapeQualities.push_back(lcq);
                                if (lcq.name == defQualityName)
                                {
                                    curLandscapeQuality = i;
                                }
                            }
                        }
                    }
                }
            }

            // particles
            const YamlNode* particlesNode = rootNode->Get("particles");
            if (nullptr != particlesNode)
            {
                particlesQualitySettings.LoadFromYaml(particlesNode);
            }
        }
    }
}

size_t QualitySettingsSystem::GetTextureQualityCount() const
{
    return textureQualities.size();
}

FastName QualitySettingsSystem::GetTextureQualityName(size_t index) const
{
    FastName ret;

    if (index < textureQualities.size())
    {
        ret = textureQualities[index].name;
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurTextureQuality() const
{
    return GetTextureQualityName(curTextureQuality);
}

void QualitySettingsSystem::SetCurTextureQuality(const FastName& name)
{
    for (size_t i = 0; i < textureQualities.size(); ++i)
    {
        if (textureQualities[i].name == name)
        {
            curTextureQuality = static_cast<int32>(i);
            return;
        }
    }

    DVASSERT(0 && "No such quality");
}

const TextureQuality* QualitySettingsSystem::GetTxQuality(const FastName& name) const
{
    const TextureQuality* ret = NULL;

    for (size_t i = 0; i < textureQualities.size(); ++i)
    {
        if (textureQualities[i].name == name)
        {
            ret = &textureQualities[i].quality;
            break;
        }
    }

    //DVASSERT(NULL != ret && "No such quality");

    return ret;
}

/*
 * Anisotropy
 */
size_t QualitySettingsSystem::GetAnisotropyQualityCount() const
{
    return anisotropyQualities.size();
}

FastName QualitySettingsSystem::GetAnisotropyQualityName(size_t index) const
{
    FastName ret;

    if (index < anisotropyQualities.size())
    {
        ret = anisotropyQualities[index].name;
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurAnisotropyQuality() const
{
    return GetAnisotropyQualityName(curAnisotropyQuality);
}

void QualitySettingsSystem::SetCurAnisotropyQuality(const FastName& name)
{
    for (size_t i = 0; i < anisotropyQualities.size(); ++i)
    {
        if (anisotropyQualities[i].name == name)
        {
            curAnisotropyQuality = static_cast<int32>(i);
            return;
        }
    }

    DVASSERT(0 && "No such quality");
}

const AnisotropyQuality* QualitySettingsSystem::GetAnisotropyQuality(const FastName& name) const
{
    const AnisotropyQuality* ret = nullptr;

    for (size_t i = 0; i < anisotropyQualities.size(); ++i)
    {
        if (anisotropyQualities[i].name == name)
        {
            ret = &anisotropyQualities[i].quality;
            break;
        }
    }

    return ret;
}

/*
 * MSAA
 */
size_t QualitySettingsSystem::GetMSAAQualityCount() const
{
    return msaaQualities.size();
}

FastName QualitySettingsSystem::GetMSAAQualityName(size_t index) const
{
    FastName ret;

    if (index < msaaQualities.size())
    {
        ret = msaaQualities[index].name;
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurMSAAQuality() const
{
    return GetMSAAQualityName(curMSAAQuality);
}

void QualitySettingsSystem::SetCurMSAAQuality(const FastName& name)
{
    for (size_t i = 0; i < msaaQualities.size(); ++i)
    {
        if (msaaQualities[i].name == name)
        {
            curMSAAQuality = static_cast<int32>(i);
            return;
        }
    }

    DVASSERT(0 && "No such quality");
}

const MSAAQuality* QualitySettingsSystem::GetMSAAQuality(const FastName& name) const
{
    const MSAAQuality* ret = nullptr;

    for (size_t i = 0; i < msaaQualities.size(); ++i)
    {
        if (msaaQualities[i].name == name)
        {
            ret = &msaaQualities[i].quality;
            break;
        }
    }

    return ret;
}

size_t QualitySettingsSystem::GetSFXQualityCount() const
{
    return soundQualities.size();
}

FastName QualitySettingsSystem::GetSFXQualityName(size_t index) const
{
    FastName ret;

    if (index < soundQualities.size())
    {
        ret = soundQualities[index].name;
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurSFXQuality() const
{
    return GetSFXQualityName(curSoundQuality);
}

void QualitySettingsSystem::SetCurSFXQuality(const FastName& name)
{
    for (size_t i = 0; i < soundQualities.size(); ++i)
    {
        if (soundQualities[i].name == name)
        {
            curSoundQuality = static_cast<int32>(i);
            return;
        }
    }
}

FilePath QualitySettingsSystem::GetSFXQualityConfigPath(const FastName& name) const
{
    FilePath ret;

    for (size_t i = 0; i < soundQualities.size(); ++i)
    {
        if (soundQualities[i].name == name)
        {
            ret = soundQualities[i].configPath;
            break;
        }
    }

    return ret;
}

FilePath QualitySettingsSystem::GetSFXQualityConfigPath(size_t index) const
{
    FilePath ret;

    if (index < soundQualities.size())
    {
        ret = soundQualities[index].configPath;
    }

    return ret;
}

size_t QualitySettingsSystem::GetLandscapeQualityCount() const
{
    return landscapeQualities.size();
}

FastName QualitySettingsSystem::GetLandscapeQualityName(size_t index) const
{
    FastName ret;

    if (index < landscapeQualities.size())
    {
        ret = landscapeQualities[index].name;
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurLandscapeQuality() const
{
    return GetLandscapeQualityName(curLandscapeQuality);
}

void QualitySettingsSystem::SetCurLandscapeQuality(const FastName& name)
{
    for (size_t i = 0; i < landscapeQualities.size(); ++i)
    {
        if (landscapeQualities[i].name == name)
        {
            curLandscapeQuality = static_cast<int32>(i);
            return;
        }
    }

    DVASSERT(0 && "No such quality");
}

const LandscapeQuality* QualitySettingsSystem::GetLandscapeQuality(const FastName& name) const
{
    const LandscapeQuality* ret = nullptr;

    for (size_t i = 0; i < landscapeQualities.size(); ++i)
    {
        if (landscapeQualities[i].name == name)
        {
            ret = &landscapeQualities[i].quality;
            break;
        }
    }

    return ret;
}

bool QualitySettingsSystem::GetAllowCutUnusedVertexStreams()
{
    return cutUnusedVertexStreams;
}
void QualitySettingsSystem::SetAllowCutUnusedVertexStreams(bool cut)
{
    cutUnusedVertexStreams = cut;
}

size_t QualitySettingsSystem::GetMaterialQualityGroupCount() const
{
    return materialGroups.size();
}

FastName QualitySettingsSystem::GetMaterialQualityGroupName(size_t index) const
{
    if (index < materialGroups.size())
    {
        return std::next(materialGroups.begin(), index)->first;
    }

    return FastName();
}

size_t QualitySettingsSystem::GetMaterialQualityCount(const FastName& group) const
{
    size_t ret = 0;

    auto it = materialGroups.find(group);

    if (it != materialGroups.end())
    {
        ret = it->second.qualities.size();
    }

    return ret;
}

FastName QualitySettingsSystem::GetMaterialQualityName(const FastName& group, size_t index) const
{
    FastName ret;

    auto it = materialGroups.find(group);

    if (it != materialGroups.end())
    {
        auto& materialGroup = it->second;

        if (index < materialGroup.qualities.size())
        {
            ret = materialGroup.qualities[index].qualityName;
        }
    }

    return ret;
}

FastName QualitySettingsSystem::GetCurMaterialQuality(const FastName& group) const
{
    FastName ret;

    auto it = materialGroups.find(group);

    if (it != materialGroups.end())
    {
        ret = GetMaterialQualityName(group, it->second.curQuality);
    }

    return ret;
}

void QualitySettingsSystem::SetCurMaterialQuality(const FastName& group, const FastName& quality)
{
    auto it = materialGroups.find(group);

    if (it != materialGroups.end())
    {
        auto& materialGroup = it->second;
        for (size_t i = 0; i < materialGroup.qualities.size(); ++i)
        {
            if (materialGroup.qualities[i].qualityName == quality)
            {
                materialGroup.curQuality = i;
                return;
            }
        }
    }

    DVASSERT(0 && "Not such quality");
}

const MaterialQuality* QualitySettingsSystem::GetMaterialQuality(const FastName& group, const FastName& quality) const
{
    const MaterialQuality* ret = NULL;

    auto it = materialGroups.find(group);

    if (it != materialGroups.end())
    {
        auto& materialGroup = it->second;
        for (size_t i = 0; i < materialGroup.qualities.size(); ++i)
        {
            if (materialGroup.qualities[i].qualityName == quality)
            {
                ret = &materialGroup.qualities[i];
                break;
            }
        }
    }

    //DVASSERT(NULL != ret && "No such quality");

    return ret;
}

const ParticlesQualitySettings& QualitySettingsSystem::GetParticlesQualitySettings() const
{
    return particlesQualitySettings;
}

ParticlesQualitySettings& QualitySettingsSystem::GetParticlesQualitySettings()
{
    return particlesQualitySettings;
}

void QualitySettingsSystem::EnableOption(const FastName& option, bool enabled)
{
    qualityOptions[option] = enabled;
}

bool QualitySettingsSystem::IsOptionEnabled(const FastName& option) const
{
    auto it = qualityOptions.find(option);

    if (it != qualityOptions.end())
    {
        return it->second;
    }

    return false;
}

int32 QualitySettingsSystem::GetOptionsCount() const
{
    return static_cast<int32>(qualityOptions.size());
}

FastName QualitySettingsSystem::GetOptionName(int32 index) const
{
    DVASSERT(index >= 0);

    if (static_cast<size_t>(index) < qualityOptions.size())
    {
        return std::next(qualityOptions.begin(), index)->first;
    }

    return FastName();
}

void QualitySettingsSystem::UpdateEntityAfterLoad(Entity* entity)
{
    if (qualityOptions.empty() || (NULL == entity))
        return;

    Vector<Entity*> entitiesWithQualityComponent;
    entity->GetChildEntitiesWithComponent(entitiesWithQualityComponent, Type::Instance<QualitySettingsComponent>());

    for (size_t i = 0, sz = entitiesWithQualityComponent.size(); i < sz; ++i)
    {
        if (!IsQualityVisible(entitiesWithQualityComponent[i]))
        {
            if (keepUnusedQualityEntities)
            {
                UpdateEntityVisibilityRecursively(entitiesWithQualityComponent[i], false);
            }
            else
            {
                Entity* parent = entitiesWithQualityComponent[i]->GetParent();
                parent->RemoveNode(entitiesWithQualityComponent[i]);
            }
        }
    }
}

bool QualitySettingsSystem::IsQualityVisible(const Entity* entity)
{
    QualitySettingsComponent* comp = GetQualitySettingsComponent(entity);
    if (comp)
    {
        if (comp->filterByType)
            return (!comp->modelType.IsValid()) || IsOptionEnabled(comp->GetModelType());
        else
            return (GetCurMaterialQuality(comp->requiredGroup) == comp->requiredQuality);
    }

    return true;
}

void QualitySettingsSystem::UpdateEntityVisibility(Entity* e)
{
    QualitySettingsComponent* comp = GetQualitySettingsComponent(e);
    if (comp)
        UpdateEntityVisibilityRecursively(e, IsQualityVisible(e));
}

void QualitySettingsSystem::UpdateEntityVisibilityRecursively(Entity* e, bool qualityVisible)
{
    RenderObject* ro = GetRenderObject(e);
    if (ro)
    {
        if (qualityVisible)
            ro->AddFlag(RenderObject::VISIBLE_QUALITY);
        else
            ro->RemoveFlag(RenderObject::VISIBLE_QUALITY);
    }

    for (int32 i = 0, sz = e->GetChildrenCount(); i < sz; ++i)
        UpdateEntityVisibilityRecursively(e->GetChild(i), qualityVisible);
}

/*
 * Helper function
 */
rhi::AntialiasingType AANameToType(const FastName& name)
{
    if (name == FastName("MSAA_4X"))
        return rhi::AntialiasingType::MSAA_4X;

    if (name == FastName("MSAA_2X"))
        return rhi::AntialiasingType::MSAA_2X;

    if (name == FastName("NONE"))
        return rhi::AntialiasingType::NONE;

    DAVA::Logger::Error("Invalid or unknown AA type specified: %s", name.c_str());
    return rhi::AntialiasingType::NONE;
}
}
