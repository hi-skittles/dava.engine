#include "UIDebugRenderComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::UIDebugRenderComponent::ePivotPointDrawMode)
{
    ENUM_ADD_DESCR(DAVA::UIDebugRenderComponent::DRAW_NEVER, "DRAW_NEVER");
    ENUM_ADD_DESCR(DAVA::UIDebugRenderComponent::DRAW_ONLY_IF_NONZERO, "DRAW_ONLY_IF_NONZERO");
    ENUM_ADD_DESCR(DAVA::UIDebugRenderComponent::DRAW_ALWAYS, "DRAW_ALWAYS");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIDebugRenderComponent)
{
    ReflectionRegistrator<UIDebugRenderComponent>::Begin()[M::DisplayName("Debug Render"), M::Group("Content")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIDebugRenderComponent* c) { SafeRelease(c); })
    .Field("enabled", &UIDebugRenderComponent::IsEnabled, &UIDebugRenderComponent::SetEnabled)[M::DisplayName("Enabled")]
    .Field("drawColor", &UIDebugRenderComponent::GetDrawColor, &UIDebugRenderComponent::SetDrawColor)[M::DisplayName("Color")]
    .Field("pivotPointDrawMode", &UIDebugRenderComponent::GetPivotPointDrawMode, &UIDebugRenderComponent::SetPivotPointDrawMode)
    [
    M::EnumT<ePivotPointDrawMode>(), M::DisplayName("Pivot")
    ]
    .End();
}
IMPLEMENT_UI_COMPONENT(UIDebugRenderComponent);

UIDebugRenderComponent::UIDebugRenderComponent()
{
}

UIDebugRenderComponent::UIDebugRenderComponent(const UIDebugRenderComponent& src)
    : UIComponent(src)
    , enabled(src.enabled)
    , drawColor(src.drawColor)
    , pivotPointDrawMode(src.pivotPointDrawMode)
{
}

void UIDebugRenderComponent::SetEnabled(bool _enabled)
{
    enabled = _enabled;
}

bool UIDebugRenderComponent::IsEnabled() const
{
    return enabled;
}

void UIDebugRenderComponent::SetDrawColor(const Color& color)
{
    drawColor = color;
}

const Color& UIDebugRenderComponent::GetDrawColor() const
{
    return drawColor;
}

void UIDebugRenderComponent::SetPivotPointDrawMode(ePivotPointDrawMode mode)
{
    pivotPointDrawMode = mode;
}

UIDebugRenderComponent::ePivotPointDrawMode UIDebugRenderComponent::GetPivotPointDrawMode() const
{
    return pivotPointDrawMode;
}

UIDebugRenderComponent* UIDebugRenderComponent::Clone() const
{
    return new UIDebugRenderComponent(*this);
}
}
