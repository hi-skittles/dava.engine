#include "SnapToLandscapeControllerComponent.h"
#include "FileSystem/KeyedArchive.h"

#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Entity/ComponentManager.h"
#include "Engine/Engine.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(SnapToLandscapeControllerComponent)
{
    ReflectionRegistrator<SnapToLandscapeControllerComponent>::Begin()
    .ConstructorByPointer()
    .Field("heightOnLandscape", &SnapToLandscapeControllerComponent::GetHeightOnLandscape, &SnapToLandscapeControllerComponent::SetHeightOnLandscape)[M::DisplayName("Height On Landscape")]
    .End();
}

SnapToLandscapeControllerComponent::SnapToLandscapeControllerComponent()
    : Component()
    , heightOnLandscape(0.f)
{
}

Component* SnapToLandscapeControllerComponent::Clone(Entity* toEntity)
{
    SnapToLandscapeControllerComponent* component = new SnapToLandscapeControllerComponent();
    component->SetEntity(toEntity);
    component->heightOnLandscape = heightOnLandscape;

    return component;
}

void SnapToLandscapeControllerComponent::SetHeightOnLandscape(float32 height)
{
    heightOnLandscape = height;
    GlobalEventSystem::Instance()->Event(this, EventSystem::SNAP_TO_LANDSCAPE_HEIGHT_CHANGED);
}

void SnapToLandscapeControllerComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
    archive->SetFloat("heightOnLandscape", heightOnLandscape);
}

void SnapToLandscapeControllerComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);
    heightOnLandscape = archive->GetFloat("heightOnLandscape", 0.f);
}
};
