#include "UISoundValueFilterComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UISoundValueFilterComponent)
{
    ReflectionRegistrator<UISoundValueFilterComponent>::Begin()[M::DisplayName("Sound Value Filter")]
    .ConstructorByPointer()
    .DestructorByPointer([](UISoundValueFilterComponent* c) { SafeRelease(c); })
    .Field("step", &UISoundValueFilterComponent::GetStep, &UISoundValueFilterComponent::SetStep)[M::DisplayName("Step")]
    .Field("deadZone", &UISoundValueFilterComponent::GetDeadZone, &UISoundValueFilterComponent::SetDeadZone)[M::DisplayName("Dead Zone")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UISoundValueFilterComponent);

UISoundValueFilterComponent::UISoundValueFilterComponent()
{
}

UISoundValueFilterComponent::UISoundValueFilterComponent(const UISoundValueFilterComponent& src)
    : step(src.step)
    , deadZone(src.deadZone)
    , normalizedValue(src.normalizedValue)
{
}

UISoundValueFilterComponent* UISoundValueFilterComponent::Clone() const
{
    return new UISoundValueFilterComponent(*this);
}

float32 UISoundValueFilterComponent::GetStep() const
{
    return step;
}

void UISoundValueFilterComponent::SetStep(float32 step_)
{
    step = step_;
}

float32 UISoundValueFilterComponent::GetDeadZone() const
{
    return deadZone;
}

void UISoundValueFilterComponent::SetDeadZone(float32 deadZone_)
{
    deadZone = deadZone_;
}
}
