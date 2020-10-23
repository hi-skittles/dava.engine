#include "Scene3D/Components/CustomPropertiesComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneFileV2.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

#include "FileSystem/FileSystem.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(CustomPropertiesComponent)
{
    ReflectionRegistrator<CustomPropertiesComponent>::Begin()[M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent()]
    .ConstructorByPointer()
    .Field("properties", &CustomPropertiesComponent::properties)[M::DisplayName("Custom properties")]
    .End();
}

CustomPropertiesComponent::CustomPropertiesComponent()
{
    properties = new KeyedArchive();
}

CustomPropertiesComponent::CustomPropertiesComponent(const KeyedArchive& srcProperties)
{
    properties = new KeyedArchive(srcProperties);
}

CustomPropertiesComponent::~CustomPropertiesComponent()
{
    SafeRelease(properties);
}

Component* CustomPropertiesComponent::Clone(Entity* toEntity)
{
    CustomPropertiesComponent* newProperties = new CustomPropertiesComponent(*properties);
    newProperties->SetEntity(toEntity);

    return newProperties;
}

void CustomPropertiesComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive && properties->Count() > 0)
    {
        String savedPath = "";
        if (properties->IsKeyExists("editor.referenceToOwner"))
        {
            savedPath = properties->GetString("editor.referenceToOwner");
            String newPath = FilePath(savedPath).GetRelativePathname(serializationContext->GetScenePath());
            properties->SetString("editor.referenceToOwner", newPath);
        }

        archive->SetByteArrayFromArchive("cpc.properties", properties);

        if (savedPath.length())
        {
            properties->SetString("editor.referenceToOwner", savedPath);
        }
    }
}

void CustomPropertiesComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    properties->DeleteAllKeys();

    if (NULL != archive && archive->IsKeyExists("cpc.properties"))
    {
        KeyedArchive* props = archive->GetArchiveFromByteArray("cpc.properties");
        LoadFromArchive(*props, serializationContext);
        props->Release();
    }

    Component::Deserialize(archive, serializationContext);
}

KeyedArchive* CustomPropertiesComponent::GetArchive()
{
    return properties;
}

void CustomPropertiesComponent::LoadFromArchive(const KeyedArchive& srcProperties, SerializationContext* serializationContext)
{
    SafeRelease(properties);
    properties = new KeyedArchive(srcProperties);

    if (properties && properties->IsKeyExists("editor.referenceToOwner"))
    {
        FilePath newPath(serializationContext->GetScenePath());
        newPath += properties->GetString("editor.referenceToOwner");

        //TODO: why we use absolute pathname instead of relative?
        properties->SetString("editor.referenceToOwner", newPath.GetAbsolutePathname());
    }
}
};