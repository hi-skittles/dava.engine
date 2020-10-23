#include "UIClipContentComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIClipContentComponent)
{
    ReflectionRegistrator<UIClipContentComponent>::Begin()[M::DisplayName("Clip"), M::Group("Content")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIClipContentComponent* c) { SafeRelease(c); })
    .Field("enabled", &UIClipContentComponent::IsEnabled, &UIClipContentComponent::SetEnabled)[M::DisplayName("Enabled")]
    .End();
}
IMPLEMENT_UI_COMPONENT(UIClipContentComponent);

UIClipContentComponent::UIClipContentComponent()
{
}

UIClipContentComponent::UIClipContentComponent(const UIClipContentComponent& src)
    : UIComponent(src)
    , enabled(src.enabled)
{
}

UIClipContentComponent* UIClipContentComponent::Clone() const
{
    return new UIClipContentComponent(*this);
}

void UIClipContentComponent::SetEnabled(bool _enabled)
{
    enabled = _enabled;
}

bool UIClipContentComponent::IsEnabled() const
{
    return enabled;
}
}
