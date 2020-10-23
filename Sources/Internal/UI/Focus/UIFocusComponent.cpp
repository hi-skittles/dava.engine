#include "UI/Focus/UIFocusComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFocusComponent)
{
    ReflectionRegistrator<UIFocusComponent>::Begin()[M::DisplayName("Focus"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFocusComponent* o) { o->Release(); })
    .Field("enabled", &UIFocusComponent::IsEnabled, &UIFocusComponent::SetEnabled)[M::DisplayName("Enabled")]
    .Field("requestFocus", &UIFocusComponent::IsRequestFocus, &UIFocusComponent::SetRequestFocus)[M::DisplayName("Request Focus")]
    .End();
}
IMPLEMENT_UI_COMPONENT(UIFocusComponent);

UIFocusComponent::UIFocusComponent()
{
}

UIFocusComponent::UIFocusComponent(const UIFocusComponent& src)
    : enabled(src.enabled)
    , requestFocus(src.requestFocus)
{
}

UIFocusComponent::~UIFocusComponent()
{
}

UIFocusComponent* UIFocusComponent::Clone() const
{
    return new UIFocusComponent(*this);
}

bool UIFocusComponent::IsEnabled() const
{
    return enabled;
}

void UIFocusComponent::SetEnabled(bool value)
{
    enabled = value;
}

bool UIFocusComponent::IsRequestFocus() const
{
    return requestFocus;
}

void UIFocusComponent::SetRequestFocus(bool value)
{
    requestFocus = value;
}
}
