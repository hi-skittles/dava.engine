#include "UI/Layouts/UIFlowLayoutHintComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

#include "UI/UIControl.h"
#include "Math/Vector.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowLayoutHintComponent)
{
    ReflectionRegistrator<UIFlowLayoutHintComponent>::Begin()[M::DisplayName("Flow Layout Hint"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowLayoutHintComponent* o) { o->Release(); })
    .Field("newLineBeforeThis", &UIFlowLayoutHintComponent::IsNewLineBeforeThis, &UIFlowLayoutHintComponent::SetNewLineBeforeThis)[M::DisplayName("New Line Before This")]
    .Field("newLineAfterThis", &UIFlowLayoutHintComponent::IsNewLineAfterThis, &UIFlowLayoutHintComponent::SetNewLineAfterThis)[M::DisplayName("New Line After This")]
    .Field("stickItemBeforeThis", &UIFlowLayoutHintComponent::IsStickItemBeforeThis, &UIFlowLayoutHintComponent::SetStickItemBeforeThis)[M::DisplayName("Stick Item Before This")]
    .Field("stickItemAfterThis", &UIFlowLayoutHintComponent::IsStickItemAfterThis, &UIFlowLayoutHintComponent::SetStickItemAfterThis)[M::DisplayName("Stick Item After This")]
    .Field("contentDirection", &UIFlowLayoutHintComponent::GetContentDirection, &UIFlowLayoutHintComponent::SetContentDirection)[M::EnumT<BiDiHelper::Direction>(), M::DisplayName("Content Direction")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowLayoutHintComponent);

UIFlowLayoutHintComponent::UIFlowLayoutHintComponent()
{
}

UIFlowLayoutHintComponent::UIFlowLayoutHintComponent(const UIFlowLayoutHintComponent& src)
    : flags(src.flags)
{
}

UIFlowLayoutHintComponent::~UIFlowLayoutHintComponent()
{
}

UIFlowLayoutHintComponent* UIFlowLayoutHintComponent::Clone() const
{
    return new UIFlowLayoutHintComponent(*this);
}

bool UIFlowLayoutHintComponent::IsNewLineBeforeThis() const
{
    return flags.test(FLAG_NEW_LINE_BEFORE_THIS);
}

void UIFlowLayoutHintComponent::SetNewLineBeforeThis(bool flag)
{
    flags.set(FLAG_NEW_LINE_BEFORE_THIS, flag);
    SetLayoutDirty();
}

bool UIFlowLayoutHintComponent::IsNewLineAfterThis() const
{
    return flags.test(FLAG_NEW_LINE_AFTER_THIS);
}

void UIFlowLayoutHintComponent::SetNewLineAfterThis(bool flag)
{
    flags.set(FLAG_NEW_LINE_AFTER_THIS, flag);
    SetLayoutDirty();
}

bool UIFlowLayoutHintComponent::IsStickItemBeforeThis() const
{
    return flags.test(FLAG_STICK_ITEM_BEFORE_THIS);
}

void UIFlowLayoutHintComponent::SetStickItemBeforeThis(bool flag)
{
    flags.set(FLAG_STICK_ITEM_BEFORE_THIS, flag);
    SetLayoutDirty();
}

bool UIFlowLayoutHintComponent::IsStickItemAfterThis() const
{
    return flags.test(FLAG_STICK_ITEM_AFTER_THIS);
}

void UIFlowLayoutHintComponent::SetStickItemAfterThis(bool flag)
{
    flags.set(FLAG_STICK_ITEM_AFTER_THIS, flag);
    SetLayoutDirty();
}

bool UIFlowLayoutHintComponent::IsStickHardBeforeThis() const
{
    return flags.test(FLAG_STICK_HARD_BEFORE_THIS);
}

void UIFlowLayoutHintComponent::SetStickHardBeforeThis(bool flag)
{
    flags.set(FLAG_STICK_HARD_BEFORE_THIS, flag);
    SetLayoutDirty();
}

bool UIFlowLayoutHintComponent::IsStickHardAfterThis() const
{
    return flags.test(FLAG_STICK_HARD_AFTER_THIS);
}

void UIFlowLayoutHintComponent::SetStickHardAfterThis(bool flag)
{
    flags.set(FLAG_STICK_HARD_AFTER_THIS, flag);
    SetLayoutDirty();
}

BiDiHelper::Direction UIFlowLayoutHintComponent::GetContentDirection() const
{
    return contentDirection;
}

void UIFlowLayoutHintComponent::SetContentDirection(BiDiHelper::Direction direction)
{
    contentDirection = direction;
    SetLayoutDirty();
}

void UIFlowLayoutHintComponent::SetLayoutDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}
}
