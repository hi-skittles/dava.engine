#include "Scene3D/Components/GeoDecalComponent.h"
#include "Scene3D/Entity.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Render/Material/NMaterialNames.h"
#include "FileSystem/FileSystem.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(GeoDecalComponent)
{
    ReflectionRegistrator<GeoDecalComponent>::Begin()
    .ConstructorByPointer()
    .Field("dimensions", &GeoDecalComponent::GetDimensions, &GeoDecalComponent::SetDimensions)[M::DisplayName("Dimensions")]
    .Field("decalAlbedo", &GeoDecalComponent::GetDecalAlbedo, &GeoDecalComponent::SetDecalAlbedo)[M::DisplayName("Decal albedo")]
    .Field("decalNormal", &GeoDecalComponent::GetDecalNormal, &GeoDecalComponent::SetDecalNormal)[M::DisplayName("Decal normal")]
    .Field("decalSpecular", &GeoDecalComponent::GetDecalSpecular, &GeoDecalComponent::SetDecalSpecular)[M::DisplayName("Decal specular")]
    .Field("textureMapping", &GeoDecalComponent::GetMapping, &GeoDecalComponent::SetMapping)[M::DisplayName("Texture mapping"), M::EnumT<GeoDecalManager::Mapping>()]
    .Field("uvScale", &GeoDecalComponent::GetUVScale, &GeoDecalComponent::SetUVScale)[M::DisplayName("UV Scale")]
    .Field("uvOffset", &GeoDecalComponent::GetUVOffset, &GeoDecalComponent::SetUVOffset)[M::DisplayName("UV Offset")]
    .Field("specularScale", &GeoDecalComponent::GetSpecularScale, &GeoDecalComponent::SetSpecularScale)[M::DisplayName("Specular Scale")]
    .Field("rebakeOnTransform", &GeoDecalComponent::GetRebakeOnTransform, &GeoDecalComponent::SetRebakeOnTransform)[M::DisplayName("Rebake on transform")]
    .Field("debugOverlayEnabled", &GeoDecalComponent::GetDebugOverlayEnabled, &GeoDecalComponent::SetDebugOverlayEnabled)[M::DisplayName("Debug Overlay")]
    .End();
}

GeoDecalComponent::GeoDecalComponent()
{
    InitWithFlags(0);
}

GeoDecalComponent::GeoDecalComponent(uint32 flags)
{
    InitWithFlags(flags);
}

void GeoDecalComponent::InitWithFlags(uint32 flags)
{
    if ((flags & SuppressMaterialCreation) == 0)
    {
        char materialName[256] = {};
        sprintf(materialName, "Geo_Decal_Material_%p", reinterpret_cast<void*>(this));

        dataNodeMaterial.reset(new NMaterial());
        dataNodeMaterial->SetMaterialName(FastName(materialName));
        dataNodeMaterial->SetRuntime(true);
    }
}

Component* GeoDecalComponent::Clone(Entity* toEntity)
{
    GeoDecalComponent* result = new GeoDecalComponent();
    result->SetEntity(toEntity);
    result->config = config;
    result->ConfigChanged();
    return result;
}

void GeoDecalComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    if (archive)
    {
        archive->SetVector3("dimensions", config.dimensions);
        archive->SetString("albedo", config.albedo.GetRelativePathname(serializationContext->GetScenePath()));
        archive->SetString("normal", config.normal.GetRelativePathname(serializationContext->GetScenePath()));
        archive->SetString("specular", config.specular.GetRelativePathname(serializationContext->GetScenePath()));
        archive->SetUInt32("mapping", config.mapping);
        archive->SetVector2("uvoffset", config.uvOffset);
        archive->SetVector2("uvscale", config.uvScale);
        archive->SetFloat("specularscale", config.specularScale);
    }
}

void GeoDecalComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (archive)
    {
        Vector3 center = archive->GetVector3("box.center");
        String albedo = archive->GetString("albedo");
        String normal = archive->GetString("normal");
        String specular = archive->GetString("specular");

        GeoDecalManager::DecalConfig localConfig;
        localConfig.dimensions = archive->GetVector3("dimensions", Vector3(1.0f, 1.0f, 1.0f));
        localConfig.albedo = albedo.empty() ? FilePath() : (serializationContext->GetScenePath() + albedo);
        localConfig.normal = normal.empty() ? FilePath() : (serializationContext->GetScenePath() + normal);
        localConfig.specular = specular.empty() ? FilePath() : (serializationContext->GetScenePath() + specular);
        localConfig.mapping = static_cast<GeoDecalManager::Mapping>(archive->GetUInt32("mapping", localConfig.mapping));
        localConfig.uvOffset = archive->GetVector2("uvoffset", Vector2(0.0f, 0.0f));
        localConfig.uvScale = archive->GetVector2("uvscale", Vector2(1.0f, 1.0f));
        localConfig.specularScale = archive->GetFloat("specularscale", localConfig.specularScale);

        config = localConfig;
        ConfigChanged();
    }
    Component::Deserialize(archive, serializationContext);
}

void GeoDecalComponent::GetDataNodes(Set<DataNode*>& dataNodes)
{
    if (dataNodeMaterial.get() != nullptr)
        dataNodes.insert(dataNodeMaterial.get());
}

void GeoDecalComponent::ConfigChanged()
{
    if (dataNodeMaterial.get() == nullptr)
        return;

    if (FileSystem::Instance()->Exists(config.albedo))
    {
        ScopedPtr<Texture> albedoTexture(Texture::CreateFromFile(config.albedo));
        if (dataNodeMaterial->HasLocalTexture(NMaterialTextureName::TEXTURE_ALBEDO))
            dataNodeMaterial->SetTexture(NMaterialTextureName::TEXTURE_ALBEDO, albedoTexture);
        else
            dataNodeMaterial->AddTexture(NMaterialTextureName::TEXTURE_ALBEDO, albedoTexture);
    }

    if (FileSystem::Instance()->Exists(config.normal))
    {
        ScopedPtr<Texture> normalTexture(Texture::CreateFromFile(config.normal));
        if (dataNodeMaterial->HasLocalTexture(NMaterialTextureName::TEXTURE_NORMAL))
            dataNodeMaterial->SetTexture(NMaterialTextureName::TEXTURE_NORMAL, normalTexture);
        else
            dataNodeMaterial->AddTexture(NMaterialTextureName::TEXTURE_NORMAL, normalTexture);
    }

    if (FileSystem::Instance()->Exists(config.specular))
    {
        ScopedPtr<Texture> specularTexture(Texture::CreateFromFile(config.specular));
        if (dataNodeMaterial->HasLocalTexture(NMaterialTextureName::TEXTURE_SPECULAR))
            dataNodeMaterial->SetTexture(NMaterialTextureName::TEXTURE_SPECULAR, specularTexture);
        else
            dataNodeMaterial->AddTexture(NMaterialTextureName::TEXTURE_SPECULAR, specularTexture);
    }
}

AABBox3 GeoDecalComponent::GetBoundingBox() const
{
    return config.GetBoundingBox();
}
}
