#include "UI/Layouts/UISizePolicyComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

#include "UI/UIControl.h"
#include "UI/Layouts/LayoutFormula.h"
#include "Math/Vector.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UISizePolicyComponent)
{
    ReflectionRegistrator<UISizePolicyComponent>::Begin()[M::DisplayName("Size Policy"), M::Group("Layout")]
    .ConstructorByPointer()
    .DestructorByPointer([](UISizePolicyComponent* o) { o->Release(); })
    .Field("horizontalPolicy", &UISizePolicyComponent::GetHorizontalPolicy, &UISizePolicyComponent::SetHorizontalPolicy)[M::EnumT<UISizePolicyComponent::eSizePolicy>(), M::DisplayName("H. Policy")]
    .Field("horizontalValue", &UISizePolicyComponent::GetHorizontalValue, &UISizePolicyComponent::SetHorizontalValue)[M::DisplayName("H. Value"), M::Bindable()]
    .Field("horizontalMin", &UISizePolicyComponent::GetHorizontalMinValue, &UISizePolicyComponent::SetHorizontalMinValue)[M::DisplayName("H. Min")]
    .Field("horizontalMax", &UISizePolicyComponent::GetHorizontalMaxValue, &UISizePolicyComponent::SetHorizontalMaxValue)[M::DisplayName("H. Max")]
    .Field("horizontalFormula", &UISizePolicyComponent::GetHorizontalFormula, &UISizePolicyComponent::SetHorizontalFormula)[M::DisplayName("H. Formula")]
    .Field("verticalPolicy", &UISizePolicyComponent::GetVerticalPolicy, &UISizePolicyComponent::SetVerticalPolicy)[M::EnumT<UISizePolicyComponent::eSizePolicy>(), M::DisplayName("V. Policy")]
    .Field("verticalValue", &UISizePolicyComponent::GetVerticalValue, &UISizePolicyComponent::SetVerticalValue)[M::DisplayName("V. Value"), M::Bindable()]
    .Field("verticalMin", &UISizePolicyComponent::GetVerticalMinValue, &UISizePolicyComponent::SetVerticalMinValue)[M::DisplayName("V. Min")]
    .Field("verticalMax", &UISizePolicyComponent::GetVerticalMaxValue, &UISizePolicyComponent::SetVerticalMaxValue)[M::DisplayName("V. Max")]
    .Field("verticalFormula", &UISizePolicyComponent::GetVerticalFormula, &UISizePolicyComponent::SetVerticalFormula)[M::DisplayName("V. Formula")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UISizePolicyComponent);

UISizePolicyComponent::UISizePolicyComponent()
{
    const float32 DEFAULT_VALUE = 100.0f;
    const float32 MIN_LIMIT = 0.0f;
    const float32 MAX_LIMIT = 99999.0f;

    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        policy[i].policy = IGNORE_SIZE;
        policy[i].value = DEFAULT_VALUE;
        policy[i].min = MIN_LIMIT;
        policy[i].max = MAX_LIMIT;
    }
}

UISizePolicyComponent::UISizePolicyComponent(const UISizePolicyComponent& src)
{
    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        policy[i].policy = src.policy[i].policy;
        policy[i].value = src.policy[i].value;
        policy[i].min = src.policy[i].min;
        policy[i].max = src.policy[i].max;

        if (src.policy[i].formula)
        {
            policy[i].formula.reset(new LayoutFormula());
            policy[i].formula->SetSource(src.policy[i].formula->GetSource());
        }
    }
}

UISizePolicyComponent::~UISizePolicyComponent()
{
}

UISizePolicyComponent* UISizePolicyComponent::Clone() const
{
    return new UISizePolicyComponent(*this);
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetHorizontalPolicy() const
{
    return policy[Vector2::AXIS_X].policy;
}

void UISizePolicyComponent::SetHorizontalPolicy(eSizePolicy newPolicy)
{
    if (policy[Vector2::AXIS_X].policy == newPolicy)
    {
        return;
    }

    policy[Vector2::AXIS_X].policy = newPolicy;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalValue() const
{
    return policy[Vector2::AXIS_X].value;
}

void UISizePolicyComponent::SetHorizontalValue(float32 value)
{
    if (policy[Vector2::AXIS_X].value == value)
    {
        return;
    }

    policy[Vector2::AXIS_X].value = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalMinValue() const
{
    return policy[Vector2::AXIS_X].min;
}

void UISizePolicyComponent::SetHorizontalMinValue(float32 value)
{
    if (policy[Vector2::AXIS_X].min == value)
    {
        return;
    }

    policy[Vector2::AXIS_X].min = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalMaxValue() const
{
    return policy[Vector2::AXIS_X].max;
}

void UISizePolicyComponent::SetHorizontalMaxValue(float32 value)
{
    if (policy[Vector2::AXIS_X].max == value)
    {
        return;
    }

    policy[Vector2::AXIS_X].max = value;
    SetLayoutDirty();
}

String UISizePolicyComponent::GetHorizontalFormula() const
{
    if (policy[Vector2::AXIS_X].formula)
    {
        return policy[Vector2::AXIS_X].formula->GetSource();
    }
    return "";
}

void UISizePolicyComponent::SetHorizontalFormula(const String& formulaSource)
{
    if (policy[Vector2::AXIS_X].formula == nullptr)
    {
        policy[Vector2::AXIS_X].formula.reset(new LayoutFormula());
    }
    policy[Vector2::AXIS_X].formula->SetSource(formulaSource);

    SetLayoutDirty();
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetVerticalPolicy() const
{
    return policy[Vector2::AXIS_Y].policy;
}

void UISizePolicyComponent::SetVerticalPolicy(eSizePolicy newPolicy)
{
    if (policy[Vector2::AXIS_Y].policy == newPolicy)
    {
        return;
    }

    policy[Vector2::AXIS_Y].policy = newPolicy;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalValue() const
{
    return policy[Vector2::AXIS_Y].value;
}

void UISizePolicyComponent::SetVerticalValue(float32 value)
{
    if (policy[Vector2::AXIS_Y].value == value)
    {
        return;
    }

    policy[Vector2::AXIS_Y].value = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalMinValue() const
{
    return policy[Vector2::AXIS_Y].min;
}

void UISizePolicyComponent::SetVerticalMinValue(float32 value)
{
    if (policy[Vector2::AXIS_Y].min == value)
    {
        return;
    }

    policy[Vector2::AXIS_Y].min = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalMaxValue() const
{
    return policy[Vector2::AXIS_Y].max;
}

void UISizePolicyComponent::SetVerticalMaxValue(float32 value)
{
    if (policy[Vector2::AXIS_Y].max == value)
    {
        return;
    }

    policy[Vector2::AXIS_Y].max = value;
    SetLayoutDirty();
}

String UISizePolicyComponent::GetVerticalFormula() const
{
    if (policy[Vector2::AXIS_Y].formula)
    {
        return policy[Vector2::AXIS_Y].formula->GetSource();
    }
    return "";
}

void UISizePolicyComponent::SetVerticalFormula(const String& formulaSource)
{
    if (policy[Vector2::AXIS_Y].formula == nullptr)
    {
        policy[Vector2::AXIS_Y].formula.reset(new LayoutFormula());
    }
    policy[Vector2::AXIS_Y].formula->SetSource(formulaSource);

    SetLayoutDirty();
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetPolicyByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].policy;
}

float32 UISizePolicyComponent::GetValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].value;
}

float32 UISizePolicyComponent::GetMinValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].min;
}

float32 UISizePolicyComponent::GetMaxValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].max;
}

LayoutFormula* UISizePolicyComponent::GetFormula(int32 axis) const
{
    return policy[axis].formula.get();
}

void UISizePolicyComponent::RemoveFormula(int32 axis)
{
    policy[axis].formula.reset();
}

bool UISizePolicyComponent::IsDependsOnChildren(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    eSizePolicy p = policy[axis].policy;
    return p == PERCENT_OF_CHILDREN_SUM || p == PERCENT_OF_MAX_CHILD || p == PERCENT_OF_FIRST_CHILD || p == PERCENT_OF_LAST_CHILD || p == FORMULA;
}

void UISizePolicyComponent::SetLayoutDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}
}
