#include "UI/Layouts/UIIgnoreLayoutComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

#include "Math/Vector.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIIgnoreLayoutComponent)
{
    ReflectionRegistrator<UIIgnoreLayoutComponent>::Begin()[M::DisplayName("Ignore Layout"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIIgnoreLayoutComponent* o) { o->Release(); })
    .Field("enabled", &UIIgnoreLayoutComponent::IsEnabled, &UIIgnoreLayoutComponent::SetEnabled)[M::DisplayName("Enabled")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIIgnoreLayoutComponent);

UIIgnoreLayoutComponent::UIIgnoreLayoutComponent(const UIIgnoreLayoutComponent& src)
    : enabled(src.enabled)
{
}

UIIgnoreLayoutComponent* UIIgnoreLayoutComponent::Clone() const
{
    return new UIIgnoreLayoutComponent(*this);
}

bool UIIgnoreLayoutComponent::IsEnabled() const
{
    return enabled;
}

void UIIgnoreLayoutComponent::SetEnabled(bool enabled_)
{
    enabled = enabled_;

    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}
}
