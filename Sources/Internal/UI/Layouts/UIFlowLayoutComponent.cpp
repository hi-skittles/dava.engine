#include "UIFlowLayoutComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "UI/UIControl.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowLayoutComponent)
{
    ReflectionRegistrator<UIFlowLayoutComponent>::Begin()[M::DisplayName("Flow Layout"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowLayoutComponent* o) { o->Release(); })
    .Field("enabled", &UIFlowLayoutComponent::IsEnabled, &UIFlowLayoutComponent::SetEnabled)[M::DisplayName("Enabled")]
    .Field("orientation", &UIFlowLayoutComponent::GetOrientation, &UIFlowLayoutComponent::SetOrientation)[M::EnumT<eOrientation>(), M::DisplayName("Orientation")]
    .Field("hPadding", &UIFlowLayoutComponent::GetHorizontalPadding, &UIFlowLayoutComponent::SetHorizontalPadding)[M::DisplayName("H. Padding")]
    .Field("hDynamicPadding", &UIFlowLayoutComponent::IsDynamicHorizontalPadding, &UIFlowLayoutComponent::SetDynamicHorizontalPadding)[M::DisplayName("H. Dynamic Padding")]
    .Field("hDynamicInLinePadding", &UIFlowLayoutComponent::IsDynamicHorizontalInLinePadding, &UIFlowLayoutComponent::SetDynamicHorizontalInLinePadding)[M::DisplayName("H. Dynamic In Line Padding")]
    .Field("hSpacing", &UIFlowLayoutComponent::GetHorizontalSpacing, &UIFlowLayoutComponent::SetHorizontalSpacing)[M::DisplayName("H. Spacing")]
    .Field("hDynamicSpacing", &UIFlowLayoutComponent::IsDynamicHorizontalSpacing, &UIFlowLayoutComponent::SetDynamicHorizontalSpacing)[M::DisplayName("H. Dynamic Spacing")]
    .Field("hSafeAreaPaddingInset", &UIFlowLayoutComponent::IsHorizontalSafeAreaPaddingInset, &UIFlowLayoutComponent::SetHorizontalSafeAreaPaddingInset)[M::DisplayName("H. Safe Area Inset")]
    .Field("vPadding", &UIFlowLayoutComponent::GetVerticalPadding, &UIFlowLayoutComponent::SetVerticalPadding)[M::DisplayName("V. Padding")]
    .Field("vDynamicPadding", &UIFlowLayoutComponent::IsDynamicVerticalPadding, &UIFlowLayoutComponent::SetDynamicVerticalPadding)[M::DisplayName("V. Dynamic Padding")]
    .Field("vSpacing", &UIFlowLayoutComponent::GetVerticalSpacing, &UIFlowLayoutComponent::SetVerticalSpacing)[M::DisplayName("V. Spacing")]
    .Field("vDynamicSpacing", &UIFlowLayoutComponent::IsDynamicVerticalSpacing, &UIFlowLayoutComponent::SetDynamicVerticalSpacing)[M::DisplayName("V. Dynamic Spacing")]
    .Field("vSafeAreaPaddingInset", &UIFlowLayoutComponent::IsVerticalSafeAreaPaddingInset, &UIFlowLayoutComponent::SetVerticalSafeAreaPaddingInset)[M::DisplayName("V. Safe Area Inset")]
    .Field("skipInvisible", &UIFlowLayoutComponent::IsSkipInvisibleControls, &UIFlowLayoutComponent::SetSkipInvisibleControls)[M::DisplayName("Skip Invisible")]
    .Field("useRtl", &UIFlowLayoutComponent::IsUseRtl, &UIFlowLayoutComponent::SetUseRtl)[M::DisplayName("Use RTL")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowLayoutComponent);

UIFlowLayoutComponent::UIFlowLayoutComponent()
{
    SetEnabled(true);
    SetSkipInvisibleControls(true);

    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        padding[i] = 0.0f;
        spacing[i] = 0.0f;
    }
}

UIFlowLayoutComponent::UIFlowLayoutComponent(const UIFlowLayoutComponent& src)
    : flags(src.flags)
{
    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        padding[i] = src.padding[i];
        spacing[i] = src.spacing[i];
    }
}

UIFlowLayoutComponent::~UIFlowLayoutComponent()
{
}

UIFlowLayoutComponent* UIFlowLayoutComponent::Clone() const
{
    return new UIFlowLayoutComponent(*this);
}

bool UIFlowLayoutComponent::IsEnabled() const
{
    return flags.test(FLAG_ENABLED);
}

void UIFlowLayoutComponent::SetEnabled(bool enabled)
{
    SetFlag(FLAG_ENABLED, enabled);
}

UIFlowLayoutComponent::eOrientation UIFlowLayoutComponent::GetOrientation() const
{
    return flags.test(FLAG_IS_RIGHT_TO_LEFT) ? ORIENTATION_RIGHT_TO_LEFT : ORIENTATION_LEFT_TO_RIGHT;
}

void UIFlowLayoutComponent::SetOrientation(eOrientation orientation)
{
    SetFlag(FLAG_IS_RIGHT_TO_LEFT, orientation == ORIENTATION_RIGHT_TO_LEFT);
}

float32 UIFlowLayoutComponent::GetHorizontalPadding() const
{
    return padding[Vector2::AXIS_X];
}

void UIFlowLayoutComponent::SetHorizontalPadding(float32 newPadding)
{
    padding[Vector2::AXIS_X] = newPadding;
    SetLayoutDirty();
}

float32 UIFlowLayoutComponent::GetHorizontalSpacing() const
{
    return spacing[Vector2::AXIS_X];
}

void UIFlowLayoutComponent::SetHorizontalSpacing(float32 newSpacing)
{
    spacing[Vector2::AXIS_X] = newSpacing;
    SetLayoutDirty();
}

bool UIFlowLayoutComponent::IsDynamicHorizontalPadding() const
{
    return flags.test(FLAG_DYNAMIC_HORIZONTAL_PADDING);
}

void UIFlowLayoutComponent::SetDynamicHorizontalPadding(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_HORIZONTAL_PADDING, dynamic);
}

bool UIFlowLayoutComponent::IsDynamicHorizontalInLinePadding() const
{
    return flags.test(FLAG_DYNAMIC_HORIZONTAL_IN_LINE_PADDING);
}

void UIFlowLayoutComponent::SetDynamicHorizontalInLinePadding(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_HORIZONTAL_IN_LINE_PADDING, dynamic);
}

bool UIFlowLayoutComponent::IsHorizontalSafeAreaPaddingInset() const
{
    return flags.test(FLAG_HORIZONTAL_SAFE_AREA_PADDING);
}

void UIFlowLayoutComponent::SetHorizontalSafeAreaPaddingInset(bool inset)
{
    SetFlag(FLAG_HORIZONTAL_SAFE_AREA_PADDING, inset);
}

bool UIFlowLayoutComponent::IsDynamicHorizontalSpacing() const
{
    return flags.test(FLAG_DYNAMIC_HORIZONTAL_SPACING);
}

void UIFlowLayoutComponent::SetDynamicHorizontalSpacing(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_HORIZONTAL_SPACING, dynamic);
}

float32 UIFlowLayoutComponent::GetVerticalPadding() const
{
    return padding[Vector2::AXIS_Y];
}

void UIFlowLayoutComponent::SetVerticalPadding(float32 newPadding)
{
    padding[Vector2::AXIS_Y] = newPadding;
    SetLayoutDirty();
}

float32 UIFlowLayoutComponent::GetVerticalSpacing() const
{
    return spacing[Vector2::AXIS_Y];
}

void UIFlowLayoutComponent::SetVerticalSpacing(float32 newSpacing)
{
    spacing[Vector2::AXIS_Y] = newSpacing;
    SetLayoutDirty();
}

bool UIFlowLayoutComponent::IsDynamicVerticalPadding() const
{
    return flags.test(FLAG_DYNAMIC_VERTICAL_PADDING);
}

void UIFlowLayoutComponent::SetDynamicVerticalPadding(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_VERTICAL_PADDING, dynamic);
}

bool UIFlowLayoutComponent::IsDynamicVerticalSpacing() const
{
    return flags.test(FLAG_DYNAMIC_VERTICAL_SPACING);
}

bool UIFlowLayoutComponent::IsVerticalSafeAreaPaddingInset() const
{
    return flags.test(FLAG_VERTICAL_SAFE_AREA_PADDING);
}

void UIFlowLayoutComponent::SetVerticalSafeAreaPaddingInset(bool inset)
{
    SetFlag(FLAG_VERTICAL_SAFE_AREA_PADDING, inset);
}

void UIFlowLayoutComponent::SetDynamicVerticalSpacing(bool dynamic)
{
    SetFlag(FLAG_DYNAMIC_VERTICAL_SPACING, dynamic);
}

float32 UIFlowLayoutComponent::GetPaddingByAxis(int32 axis) const
{
    return padding[axis];
}

float32 UIFlowLayoutComponent::GetSpacingByAxis(int32 axis) const
{
    return spacing[axis];
}

bool UIFlowLayoutComponent::IsUseRtl() const
{
    return flags.test(FLAG_USE_RTL);
}

void UIFlowLayoutComponent::SetUseRtl(bool use)
{
    SetFlag(FLAG_USE_RTL, use);
}

bool UIFlowLayoutComponent::IsSkipInvisibleControls() const
{
    return flags.test(FLAG_SKIP_INVISIBLE_CONTROLS);
}

void UIFlowLayoutComponent::SetSkipInvisibleControls(bool skip)
{
    SetFlag(FLAG_SKIP_INVISIBLE_CONTROLS, skip);
}

int32 UIFlowLayoutComponent::GetOrientationAsInt() const
{
    return GetOrientation();
}

void UIFlowLayoutComponent::SetOrientationFromInt(int32 orientation)
{
    SetOrientation(static_cast<eOrientation>(orientation));
}

void UIFlowLayoutComponent::SetLayoutDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}

void UIFlowLayoutComponent::SetFlag(eFlags flag, bool enabled)
{
    if (flags.test(flag) != enabled)
    {
        flags.set(flag, enabled);
        SetLayoutDirty();
    }
}
}
