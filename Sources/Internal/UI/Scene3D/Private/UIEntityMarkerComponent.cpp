#include "UI/Scene3D/UIEntityMarkerComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scene3D/Entity.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIEntityMarkerComponent)
{
    ReflectionRegistrator<UIEntityMarkerComponent>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIEntityMarkerComponent* c) { SafeRelease(c); })
    .Field("targetEntity", &UIEntityMarkerComponent::GetTargetEntity, &UIEntityMarkerComponent::SetTargetEntity)[M::HiddenField()]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIEntityMarkerComponent);

UIEntityMarkerComponent::UIEntityMarkerComponent() = default;

UIEntityMarkerComponent::~UIEntityMarkerComponent() = default;

UIEntityMarkerComponent::UIEntityMarkerComponent(const UIEntityMarkerComponent& src)
    : targetEntity(src.targetEntity)
{
}

UIEntityMarkerComponent* UIEntityMarkerComponent::Clone() const
{
    return new UIEntityMarkerComponent(*this);
}

void UIEntityMarkerComponent::SetTargetEntity(Entity* e)
{
    targetEntity = e;
}
}
