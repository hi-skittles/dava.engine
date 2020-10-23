#include "UI/Input/UIModalInputComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIModalInputComponent)
{
    ReflectionRegistrator<UIModalInputComponent>::Begin()[M::DisplayName("Modal Input"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIModalInputComponent* o) { o->Release(); })
    .Field("enabled", &UIModalInputComponent::IsEnabled, &UIModalInputComponent::SetEnabled)[M::DisplayName("Enabled")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIModalInputComponent);

UIModalInputComponent::UIModalInputComponent()
{
}

UIModalInputComponent::UIModalInputComponent(const UIModalInputComponent& src)
    : enabled(src.enabled)
{
}

UIModalInputComponent::~UIModalInputComponent()
{
}

UIModalInputComponent* UIModalInputComponent::Clone() const
{
    return new UIModalInputComponent(*this);
}

bool UIModalInputComponent::IsEnabled() const
{
    return enabled;
}

void UIModalInputComponent::SetEnabled(bool enabled_)
{
    enabled = enabled_;
}
}
