#include "UI/Focus/UITabOrderComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UITabOrderComponent)
{
    ReflectionRegistrator<UITabOrderComponent>::Begin()[M::DisplayName("Tab Order"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UITabOrderComponent* o) { o->Release(); })
    .Field("tab", &UITabOrderComponent::GetTabOrder, &UITabOrderComponent::SetTabOrder)[M::DisplayName("Tab Order")]
    .End();
}
IMPLEMENT_UI_COMPONENT(UITabOrderComponent);

UITabOrderComponent::UITabOrderComponent()
{
}

UITabOrderComponent::UITabOrderComponent(const UITabOrderComponent& src)
    : tabOrder(src.tabOrder)
{
}

UITabOrderComponent::~UITabOrderComponent()
{
}

UITabOrderComponent* UITabOrderComponent::Clone() const
{
    return new UITabOrderComponent(*this);
}

int32 UITabOrderComponent::GetTabOrder() const
{
    return tabOrder;
}

void UITabOrderComponent::SetTabOrder(int32 val)
{
    tabOrder = val;
}
}
