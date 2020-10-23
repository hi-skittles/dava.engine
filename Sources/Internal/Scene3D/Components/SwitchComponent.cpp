#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(SwitchComponent)
{
    ReflectionRegistrator<SwitchComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("newSwitchIndex", &SwitchComponent::GetSwitchIndex, &SwitchComponent::SetSwitchIndex)[M::DisplayName("Switch index")]
    .End();
}

SwitchComponent::SwitchComponent()
    : oldSwitchIndex(-1)
    , newSwitchIndex(0)
{
}

Component* SwitchComponent::Clone(Entity* toEntity)
{
    SwitchComponent* newComponent = new SwitchComponent();
    newComponent->SetEntity(toEntity);
    newComponent->SetSwitchIndex(GetSwitchIndex());

    return newComponent;
}

void SwitchComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive)
    {
        archive->SetInt32("sc.switchindex", newSwitchIndex);
    }
}

void SwitchComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);

    if (NULL != archive)
    {
        SetSwitchIndex(archive->GetInt32("sc.switchindex"));
    }
}

void SwitchComponent::SetSwitchIndex(const int32& _switchIndex)
{
    newSwitchIndex = _switchIndex;

    GlobalEventSystem::Instance()->Event(this, EventSystem::SWITCH_CHANGED);
}

int32 SwitchComponent::GetSwitchIndex() const
{
    return newSwitchIndex;
}
}