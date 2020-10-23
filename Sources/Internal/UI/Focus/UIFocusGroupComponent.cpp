#include "UI/Focus/UIFocusGroupComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFocusGroupComponent)
{
    ReflectionRegistrator<UIFocusGroupComponent>::Begin()[M::DisplayName("Focus Group"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFocusGroupComponent* o) { o->Release(); })
    .End();
}
IMPLEMENT_UI_COMPONENT(UIFocusGroupComponent);

UIFocusGroupComponent::UIFocusGroupComponent()
{
}

UIFocusGroupComponent::UIFocusGroupComponent(const UIFocusGroupComponent& src)
{
}

UIFocusGroupComponent::~UIFocusGroupComponent()
{
}

UIFocusGroupComponent* UIFocusGroupComponent::Clone() const
{
    return new UIFocusGroupComponent(*this);
}
}
