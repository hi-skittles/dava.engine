#include "UIAnchorSafeAreaComponent.h"

#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "UI/UIControl.h"
#include "Reflection/ReflectionRegistrator.h"

ENUM_DECLARE(DAVA::UIAnchorSafeAreaComponent::eInsetType)
{
    ENUM_ADD_DESCR(static_cast<DAVA::int32>(DAVA::UIAnchorSafeAreaComponent::eInsetType::NONE), "NONE");
    ENUM_ADD_DESCR(static_cast<DAVA::int32>(DAVA::UIAnchorSafeAreaComponent::eInsetType::INSET), "INSET");
    ENUM_ADD_DESCR(static_cast<DAVA::int32>(DAVA::UIAnchorSafeAreaComponent::eInsetType::INSET_ONLY_IF_NOTCH), "INSET_ONLY_IF_NOTCH");
    ENUM_ADD_DESCR(static_cast<DAVA::int32>(DAVA::UIAnchorSafeAreaComponent::eInsetType::REVERSE), "REVERSE");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIAnchorSafeAreaComponent)
{
    ReflectionRegistrator<UIAnchorSafeAreaComponent>::Begin()[M::DisplayName("Anchor Safe Area"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIAnchorSafeAreaComponent* o) { o->Release(); })
    .Field("leftSafeInset", &UIAnchorSafeAreaComponent::GetLeftInset, &UIAnchorSafeAreaComponent::SetLeftInset)[M::EnumT<UIAnchorSafeAreaComponent::eInsetType>(), M::DisplayName("Left Inset")]
    .Field("leftInsetCorrection", &UIAnchorSafeAreaComponent::GetLeftInsetCorrection, &UIAnchorSafeAreaComponent::SetLeftInsetCorrection)[M::DisplayName("Left Inset Correction")]
    .Field("topSafeInset", &UIAnchorSafeAreaComponent::GetTopInset, &UIAnchorSafeAreaComponent::SetTopInset)[M::EnumT<UIAnchorSafeAreaComponent::eInsetType>(), M::DisplayName("Top Inset")]
    .Field("topInsetCorrection", &UIAnchorSafeAreaComponent::GetTopInsetCorrection, &UIAnchorSafeAreaComponent::SetTopInsetCorrection)[M::DisplayName("Top Inset Correction")]
    .Field("rightSafeInset", &UIAnchorSafeAreaComponent::GetRightInset, &UIAnchorSafeAreaComponent::SetRightInset)[M::EnumT<UIAnchorSafeAreaComponent::eInsetType>(), M::DisplayName("Right Inset")]
    .Field("rightInsetCorrection", &UIAnchorSafeAreaComponent::GetRightInsetCorrection, &UIAnchorSafeAreaComponent::SetRightInsetCorrection)[M::DisplayName("Right Inset Correction")]
    .Field("bottomSafeInset", &UIAnchorSafeAreaComponent::GetBottomInset, &UIAnchorSafeAreaComponent::SetBottomInset)[M::EnumT<UIAnchorSafeAreaComponent::eInsetType>(), M::DisplayName("Bottom Inset")]
    .Field("bottomInsetCorrection", &UIAnchorSafeAreaComponent::GetBottomInsetCorrection, &UIAnchorSafeAreaComponent::SetBottomInsetCorrection)[M::DisplayName("Bottom Inset Correction")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIAnchorSafeAreaComponent);

UIAnchorSafeAreaComponent::UIAnchorSafeAreaComponent()
{
}

UIAnchorSafeAreaComponent::UIAnchorSafeAreaComponent(const UIAnchorSafeAreaComponent& src)
    : leftInset(src.leftInset)
    , leftInsetCorrection(src.leftInsetCorrection)
    , topInset(src.topInset)
    , topInsetCorrection(src.topInsetCorrection)
    , rightInset(src.rightInset)
    , rightInsetCorrection(src.rightInsetCorrection)
    , bottomInset(src.bottomInset)
    , bottomInsetCorrection(src.bottomInsetCorrection)
{
}

UIAnchorSafeAreaComponent::~UIAnchorSafeAreaComponent()
{
}

UIAnchorSafeAreaComponent* UIAnchorSafeAreaComponent::Clone() const
{
    return new UIAnchorSafeAreaComponent(*this);
}

UIAnchorSafeAreaComponent::eInsetType UIAnchorSafeAreaComponent::GetLeftInset() const
{
    return leftInset;
}

void UIAnchorSafeAreaComponent::SetLeftInset(eInsetType inset)
{
    leftInset = inset;
    MarkDirty();
}

UIAnchorSafeAreaComponent::eInsetType UIAnchorSafeAreaComponent::GetTopInset() const
{
    return topInset;
}

void UIAnchorSafeAreaComponent::SetTopInset(eInsetType inset)
{
    topInset = inset;
    MarkDirty();
}

UIAnchorSafeAreaComponent::eInsetType UIAnchorSafeAreaComponent::GetRightInset() const
{
    return rightInset;
}

void UIAnchorSafeAreaComponent::SetRightInset(eInsetType inset)
{
    rightInset = inset;
    MarkDirty();
}

UIAnchorSafeAreaComponent::eInsetType UIAnchorSafeAreaComponent::GetBottomInset() const
{
    return bottomInset;
}

void UIAnchorSafeAreaComponent::SetBottomInset(eInsetType inset)
{
    bottomInset = inset;
    MarkDirty();
}

float32 UIAnchorSafeAreaComponent::GetLeftInsetCorrection() const
{
    return leftInsetCorrection;
}

void UIAnchorSafeAreaComponent::SetLeftInsetCorrection(float32 correction)
{
    leftInsetCorrection = correction;
    MarkDirty();
}

float32 UIAnchorSafeAreaComponent::GetTopInsetCorrection() const
{
    return topInsetCorrection;
}

void UIAnchorSafeAreaComponent::SetTopInsetCorrection(float32 correction)
{
    topInsetCorrection = correction;
    MarkDirty();
}

float32 UIAnchorSafeAreaComponent::GetRightInsetCorrection() const
{
    return rightInsetCorrection;
}

void UIAnchorSafeAreaComponent::SetRightInsetCorrection(float32 correction)
{
    rightInsetCorrection = correction;
    MarkDirty();
}

float32 UIAnchorSafeAreaComponent::GetBottomInsetCorrection() const
{
    return bottomInsetCorrection;
}

void UIAnchorSafeAreaComponent::SetBottomInsetCorrection(float32 correction)
{
    bottomInsetCorrection = correction;
    MarkDirty();
}

void UIAnchorSafeAreaComponent::MarkDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}
}
