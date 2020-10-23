#include "UI/Events/UIInputEventComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIInputEventComponent)
{
    ReflectionRegistrator<UIInputEventComponent>::Begin()[M::DisplayName("Input Event"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIInputEventComponent* o) { o->Release(); })
    .Field("onTouchDown", &UIInputEventComponent::GetOnTouchDownEvent, &UIInputEventComponent::SetOnTouchDownEvent)[M::DisplayName("Touch Down")]
    .Field("onTouchDownData", &UIInputEventComponent::GetOnTouchDownDataExpression, &UIInputEventComponent::SetOnTouchDownDataExpression)[M::DisplayName("Data")]

    .Field("onTouchUpInside", &UIInputEventComponent::GetOnTouchUpInsideEvent, &UIInputEventComponent::SetOnTouchUpInsideEvent)[M::DisplayName("Touch Up Inside")]
    .Field("onTouchUpInsideData", &UIInputEventComponent::GetOnTouchUpInsideDataExpression, &UIInputEventComponent::SetOnTouchUpInsideDataExpression)[M::DisplayName("Data")]

    .Field("onTouchUpOutside", &UIInputEventComponent::GetOnTouchUpOutsideEvent, &UIInputEventComponent::SetOnTouchUpOutsideEvent)[M::DisplayName("Touch Up Outside")]
    .Field("onTouchUpOutsideData", &UIInputEventComponent::GetOnTouchUpOutsideDataExpression, &UIInputEventComponent::SetOnTouchUpOutsideDataExpression)[M::DisplayName("Data")]

    .Field("onValueChanged", &UIInputEventComponent::GetOnValueChangedEvent, &UIInputEventComponent::SetOnValueChangedEvent)[M::DisplayName("Value Changed")]
    .Field("onValueChangedData", &UIInputEventComponent::GetOnValueChangedDataExpression, &UIInputEventComponent::SetOnValueChangedDataExpression)[M::DisplayName("Data")]

    .Field("onHoverSet", &UIInputEventComponent::GetOnHoverSetEvent, &UIInputEventComponent::SetOnHoverSetEvent)[M::DisplayName("Hover Start")]
    .Field("onHoverSetData", &UIInputEventComponent::GetOnHoverSetDataExpression, &UIInputEventComponent::SetOnHoverSetDataExpression)[M::DisplayName("Data")]

    .Field("onHoverRemoved", &UIInputEventComponent::GetOnHoverRemovedEvent, &UIInputEventComponent::SetOnHoverRemovedEvent)[M::DisplayName("Hover End")]
    .Field("onHoverRemovedData", &UIInputEventComponent::GetOnHoverRemovedDataExpression, &UIInputEventComponent::SetOnHoverRemovedDataExpression)[M::DisplayName("Data")]

    .End();
}

IMPLEMENT_UI_COMPONENT(UIInputEventComponent);

UIInputEventComponent::UIInputEventComponent()
{
}

UIInputEventComponent::UIInputEventComponent(const UIInputEventComponent& src)
    : onTouchDown(src.onTouchDown)
    , onTouchDownDataExpression(src.onTouchDownDataExpression)
    , onTouchUpInside(src.onTouchUpInside)
    , onTouchUpInsideDataExpression(src.onTouchUpInsideDataExpression)
    , onTouchUpOutside(src.onTouchUpOutside)
    , onTouchUpOutsideDataExpression(src.onTouchUpOutsideDataExpression)
    , onValueChanged(src.onValueChanged)
    , onValueChangedDataExpression(src.onValueChangedDataExpression)
    , onHoverSet(src.onHoverSet)
    , onHoverSetDataExpression(src.onHoverSetDataExpression)
    , onHoverRemoved(src.onHoverRemoved)
    , onHoverRemovedDataExpression(src.onHoverRemovedDataExpression)
{
}

UIInputEventComponent::~UIInputEventComponent()
{
}

UIInputEventComponent* UIInputEventComponent::Clone() const
{
    return new UIInputEventComponent(*this);
}

const FastName& UIInputEventComponent::GetOnTouchDownEvent() const
{
    return onTouchDown;
}

void UIInputEventComponent::SetOnTouchDownEvent(const FastName& value)
{
    onTouchDown = value;
}

const String& UIInputEventComponent::GetOnTouchDownDataExpression() const
{
    return onTouchDownDataExpression;
}

void UIInputEventComponent::SetOnTouchDownDataExpression(const String& exp)
{
    onTouchDownDataExpression = exp;
}

const FastName& UIInputEventComponent::GetOnTouchUpInsideEvent() const
{
    return onTouchUpInside;
}

void UIInputEventComponent::SetOnTouchUpInsideEvent(const FastName& value)
{
    onTouchUpInside = value;
}

const String& UIInputEventComponent::GetOnTouchUpInsideDataExpression() const
{
    return onTouchUpInsideDataExpression;
}

void UIInputEventComponent::SetOnTouchUpInsideDataExpression(const String& exp)
{
    onTouchUpInsideDataExpression = exp;
}

const FastName& UIInputEventComponent::GetOnTouchUpOutsideEvent() const
{
    return onTouchUpOutside;
}

void UIInputEventComponent::SetOnTouchUpOutsideEvent(const FastName& value)
{
    onTouchUpOutside = value;
}

const String& UIInputEventComponent::GetOnTouchUpOutsideDataExpression() const
{
    return onTouchUpOutsideDataExpression;
}

void UIInputEventComponent::SetOnTouchUpOutsideDataExpression(const String& exp)
{
    onTouchUpOutsideDataExpression = exp;
}

const FastName& UIInputEventComponent::GetOnValueChangedEvent() const
{
    return onValueChanged;
}

void UIInputEventComponent::SetOnValueChangedEvent(const FastName& value)
{
    onValueChanged = value;
}

const String& UIInputEventComponent::GetOnValueChangedDataExpression() const
{
    return onValueChangedDataExpression;
}

void UIInputEventComponent::SetOnValueChangedDataExpression(const String& exp)
{
    onValueChangedDataExpression = exp;
}

const FastName& UIInputEventComponent::GetOnHoverSetEvent() const
{
    return onHoverSet;
}

void UIInputEventComponent::SetOnHoverSetEvent(const FastName& value)
{
    onHoverSet = value;
}

const String& UIInputEventComponent::GetOnHoverSetDataExpression() const
{
    return onHoverSetDataExpression;
}

void UIInputEventComponent::SetOnHoverSetDataExpression(const String& exp)
{
    onHoverSetDataExpression = exp;
}

const FastName& UIInputEventComponent::GetOnHoverRemovedEvent() const
{
    return onHoverRemoved;
}

void UIInputEventComponent::SetOnHoverRemovedEvent(const FastName& value)
{
    onHoverRemoved = value;
}

const String& UIInputEventComponent::GetOnHoverRemovedDataExpression() const
{
    return onHoverRemovedDataExpression;
}

void UIInputEventComponent::SetOnHoverRemovedDataExpression(const String& exp)
{
    onHoverRemovedDataExpression = exp;
}
}
