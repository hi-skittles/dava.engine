#include "UILayoutIsolationComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

#include "Math/Vector.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UILayoutIsolationComponent)
{
    ReflectionRegistrator<UILayoutIsolationComponent>::Begin()[M::HiddenField()]
    .ConstructorByPointer()
    .DestructorByPointer([](UILayoutIsolationComponent* o) { o->Release(); })
    .End();
}

IMPLEMENT_UI_COMPONENT(UILayoutIsolationComponent);

UILayoutIsolationComponent::UILayoutIsolationComponent()
{
}

UILayoutIsolationComponent::UILayoutIsolationComponent(const UILayoutIsolationComponent& src)
{
}

UILayoutIsolationComponent::~UILayoutIsolationComponent()
{
}

UILayoutIsolationComponent* UILayoutIsolationComponent::Clone() const
{
    return new UILayoutIsolationComponent(*this);
}
}
