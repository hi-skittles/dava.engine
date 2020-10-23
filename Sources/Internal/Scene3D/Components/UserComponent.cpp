#include "Scene3D/Components/UserComponent.h"
#include "Scene3D/Entity.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UserComponent)
{
    ReflectionRegistrator<UserComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .End();
}

UserComponent::UserComponent()
{
}

Component* UserComponent::Clone(Entity* toEntity)
{
    UserComponent* uc = new UserComponent();
    uc->SetEntity(toEntity);

    return uc;
}

void UserComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);
}

void UserComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Deserialize(archive, serializationContext);
}
}
