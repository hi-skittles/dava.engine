#include "RotationControllerComponent.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
Component* RotationControllerComponent::Clone(Entity* toEntity)
{
    RotationControllerComponent* component = new RotationControllerComponent();
    component->SetEntity(toEntity);

    return component;
}

DAVA_VIRTUAL_REFLECTION_IMPL(RotationControllerComponent)
{
    ReflectionRegistrator<RotationControllerComponent>::Begin()
    .ConstructorByPointer()
    .End();
}
};
