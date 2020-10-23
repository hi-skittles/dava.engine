#include "UI/Layouts/UILinearLayoutComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UILinearLayoutComponent)
{
    ReflectionRegistrator<UILinearLayoutComponent>::Begin()[M::DisplayName("Linear Layout"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UILinearLayoutComponent* o) { o->Release(); })
    .Field("enabled", &UILinearLayoutComponent::IsEnabled, &UILinearLayoutComponent::SetEnabled)[M::DisplayName("Enabled")]
    .Field("orientation", &UILinearLayoutComponent::GetOrientation, &UILinearLayoutComponent::SetOrientation)[M::EnumT<eOrientation>(), M::DisplayName("Orientation")]
    .Field("padding", &UILinearLayoutComponent::GetPadding, &UILinearLayoutComponent::SetPadding)[M::DisplayName("Padding")]
    .Field("dynamicPadding", &UILinearLayoutComponent::IsDynamicPadding, &UILinearLayoutComponent::SetDynamicPadding)[M::DisplayName("Dynamic Padding")]
    .Field("safeAreaPaddingInset", &UILinearLayoutComponent::IsSafeAreaPaddingInset, &UILinearLayoutComponent::SetSafeAreaPaddingInset)[M::DisplayName("Safe Area Inset")]
    .Field("spacing", &UILinearLayoutComponent::GetSpacing, &UILinearLayoutComponent::SetSpacing)[M::DisplayName("Spacing")]
    .Field("dynamicSpacing", &UILinearLayoutComponent::IsDynamicSpacing, &UILinearLayoutComponent::SetDynamicSpacing)[M::DisplayName("Dynamic Spacing")]
    .Field("skipInvisible", &UILinearLayoutComponent::IsSkipInvisibleControls, &UILinearLayoutComponent::SetSkipInvisibleControls)[M::DisplayName("Skip Invisible")]
    .Field("useRtl", &UILinearLayoutComponent::IsUseRtl, &UILinearLayoutComponent::SetUseRtl)[M::DisplayName("Use RTL")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UILinearLayoutComponent);

UILinearLayoutComponent::UILinearLayoutComponent()
{
    SetEnabled(true);
    SetSkipInvisibleControls(true);
}

UILinearLayoutComponent::UILinearLayoutComponent(const UILinearLayoutComponent& src)
    : flags(src.flags)
    , padding(src.padding)
    , spacing(src.spacing)
{
}

UILinearLayoutComponent::~UILinearLayoutComponent()
{
}

UILinearLayoutComponent* UILinearLayoutComponent::Clone() const
{
    return new UILinearLayoutComponent(*this);
}

bool UILinearLayoutComponent::IsEnabled() const
{
    return flags.test(FLAG_ENABLED);
}

void UILinearLayoutComponent::SetEnabled(bool enabled)
{
    SetFlag(FLAG_ENABLED, enabled);
}

UILinearLayoutComponent::eOrientation UILinearLayoutComponent::GetOrientation() const
{
    if (flags.test(FLAG_ORIENTATION_VERTICAL))
    {
        return flags.test(FLAG_ORIENTATION_INVERSE) ? BOTTOM_UP : TOP_DOWN;
    }
    else
    {
        return flags.test(FLAG_ORIENTATION_INVERSE) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
    }
}

void UILinearLayoutComponent::SetOrientation(eOrientation direction)
{
    bool vertical = false;
    bool inverse = false;
    switch (direction)
    {
    case LEFT_TO_RIGHT:
        vertical = false;
        inverse = false;
        break;
    case RIGHT_TO_LEFT:
        vertical = false;
        inverse = true;
        break;
    case TOP_DOWN:
        vertical = true;
        inverse = false;
        break;
    case BOTTOM_UP:
        vertical = true;
        inverse = true;
        break;
    }

    SetFlag(FLAG_ORIENTATION_VERTICAL, vertical);
    SetFlag(FLAG_ORIENTATION_INVERSE, inverse);
}

Vector2::eAxis UILinearLayoutComponent::GetAxis() const
{
    return flags.test(FLAG_ORIENTATION_VERTICAL) ? Vector2::AXIS_Y : Vector2::AXIS_X;
}

bool UILinearLayoutComponent::IsInverse() const
{
    return flags.test(FLAG_ORIENTATION_INVERSE);
}

float32 UILinearLayoutComponent::GetPadding() const
{
    return padding;
}

void UILinearLayoutComponent::SetPadding(float32 newPadding)
{
    if (padding == newPadding)
    {
        return;
    }

    padding = newPadding;
    SetLayoutDirty();
}

float32 UILinearLayoutComponent::GetSpacing() const
{
    return spacing;
}

void UILinearLayoutComponent::SetSpacing(float32 newSpacing)
{
    if (spacing == newSpacing)
    {
        return;
    }

    spacing = newSpacing;
    SetLayoutDirty();
}

bool UILinearLayoutComponent::IsDynamicPadding() const
{
    return flags.test(FLAG_DYNAMIC_PADDING);
}

void UILinearLayoutComponent::SetDynamicPadding(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_PADDING, dynamic);
}

bool UILinearLayoutComponent::IsSafeAreaPaddingInset() const
{
    return flags.test(FLAG_SAFE_AREA_PADDING_INSET);
}

void UILinearLayoutComponent::SetSafeAreaPaddingInset(bool inset)
{
    SetFlag(FLAG_SAFE_AREA_PADDING_INSET, inset);
}

bool UILinearLayoutComponent::IsDynamicSpacing() const
{
    return flags.test(FLAG_DYNAMIC_SPACING);
}

void UILinearLayoutComponent::SetDynamicSpacing(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_SPACING, dynamic);
}

bool UILinearLayoutComponent::IsSkipInvisibleControls() const
{
    return flags.test(FLAG_SKIP_INVISIBLE_CONTROLS);
}

void UILinearLayoutComponent::SetSkipInvisibleControls(bool skip)
{
    SetFlag(FLAG_SKIP_INVISIBLE_CONTROLS, skip);
}

bool UILinearLayoutComponent::IsUseRtl() const
{
    return flags.test(FLAG_RTL);
}

void UILinearLayoutComponent::SetUseRtl(bool use)
{
    SetFlag(FLAG_RTL, use);
}

int32 UILinearLayoutComponent::GetOrientationAsInt() const
{
    return static_cast<int32>(GetOrientation());
}

void UILinearLayoutComponent::SetOrientationFromInt(int32 orientation)
{
    SetOrientation(static_cast<eOrientation>(orientation));
}

void UILinearLayoutComponent::SetLayoutDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}

void UILinearLayoutComponent::SetFlag(eFlags flag, bool enabled)
{
    if (flags.test(flag) != enabled)
    {
        flags.set(flag, enabled);
        SetLayoutDirty();
    }
}
}
