#include "UISoundComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UISoundComponent)
{
    ReflectionRegistrator<UISoundComponent>::Begin()[M::DisplayName("Sound")]
    .ConstructorByPointer()
    .DestructorByPointer([](UISoundComponent* c) { SafeRelease(c); })
    .Field("touchDown", &UISoundComponent::GetOnTouchDownSoundEventName, &UISoundComponent::SetOnTouchDownSoundEventName)[M::DisplayName("Touch Down")]
    .Field("touchUpInside", &UISoundComponent::GetOnTouchUpInsideSoundEventName, &UISoundComponent::SetOnTouchUpInsideSoundEventName)[M::DisplayName("Touch Up Inside")]
    .Field("touchUpOutside", &UISoundComponent::GetOnTouchUpOutsideSoundEventName, &UISoundComponent::SetOnTouchUpOutsideSoundEventName)[M::DisplayName("Touch Up Outside")]
    .Field("valueChanged", &UISoundComponent::GetOnValueChangedSoundEventName, &UISoundComponent::SetOnValueChangedSoundEventName)[M::DisplayName("Value Changed")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UISoundComponent);

UISoundComponent::UISoundComponent()
{
}

UISoundComponent::UISoundComponent(const UISoundComponent& src)
    : soundEventNames(src.soundEventNames)
{
}

UISoundComponent* UISoundComponent::Clone() const
{
    return new UISoundComponent(*this);
}

const FastName& UISoundComponent::GetSoundEventName(UIControl::eEventType uiEventType) const
{
    return soundEventNames[uiEventType];
}

void UISoundComponent::SetSoundEventName(UIControl::eEventType uiEventType, const FastName& soundEventName)
{
    soundEventNames[uiEventType] = soundEventName;
}

const FastName& UISoundComponent::GetOnTouchDownSoundEventName() const
{
    return GetSoundEventName(UIControl::EVENT_TOUCH_DOWN);
}

void UISoundComponent::SetOnTouchDownSoundEventName(const FastName& soundEventName)
{
    SetSoundEventName(UIControl::EVENT_TOUCH_DOWN, soundEventName);
}

const FastName& UISoundComponent::GetOnTouchUpInsideSoundEventName() const
{
    return GetSoundEventName(UIControl::EVENT_TOUCH_UP_INSIDE);
}

void UISoundComponent::SetOnTouchUpInsideSoundEventName(const FastName& soundEventName)
{
    SetSoundEventName(UIControl::EVENT_TOUCH_UP_INSIDE, soundEventName);
}

const FastName& UISoundComponent::GetOnTouchUpOutsideSoundEventName() const
{
    return GetSoundEventName(UIControl::EVENT_TOUCH_UP_OUTSIDE);
}

void UISoundComponent::SetOnTouchUpOutsideSoundEventName(const FastName& soundEventName)
{
    SetSoundEventName(UIControl::EVENT_TOUCH_UP_OUTSIDE, soundEventName);
}

const FastName& UISoundComponent::GetOnValueChangedSoundEventName() const
{
    return GetSoundEventName(UIControl::EVENT_VALUE_CHANGED);
}

void UISoundComponent::SetOnValueChangedSoundEventName(const FastName& soundEventName)
{
    SetSoundEventName(UIControl::EVENT_VALUE_CHANGED, soundEventName);
}
}
