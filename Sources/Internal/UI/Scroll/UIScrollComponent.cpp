#include "UIScrollComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIScrollComponent)
{
    ReflectionRegistrator<UIScrollComponent>::Begin()[M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](UIScrollComponent* o) { o->Release(); })
    .End();
}

IMPLEMENT_UI_COMPONENT(UIScrollComponent);

UIScrollComponent::UIScrollComponent()
{
}

UIScrollComponent::UIScrollComponent(const UIScrollComponent& src)
{
}

UIScrollComponent::~UIScrollComponent()
{
}

UIScrollComponent* UIScrollComponent::Clone() const
{
    return new UIScrollComponent(*this);
}
}
