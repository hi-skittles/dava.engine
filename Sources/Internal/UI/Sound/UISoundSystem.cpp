#include "UI/Sound/UISoundSystem.h"

#include "Engine/Engine.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Sound/SoundSystem.h"
#include "UI/Sound/UISoundComponent.h"
#include "UI/Sound/UISoundValueFilterComponent.h"
#include "UI/UIEvent.h"
#include "UI/UISlider.h"

namespace DAVA
{
namespace UISoundSystemDetail
{
const FastName SOUND_PARAM_PAN("pan");
const FastName SOUND_GROUP("UI_SOUND_GROUP");
}

UISoundSystem::UISoundSystem()
{
}

UISoundSystem::~UISoundSystem()
{
}

void UISoundSystem::ProcessControlEvent(int32 eventType, const UIEvent* uiEvent, const UIControl* control)
{
    UISoundComponent* soundComponent = control->GetComponent<UISoundComponent>();

    if (soundComponent != nullptr
        && uiEvent != nullptr)
    {
        if (!ShouldSkipEvent(eventType, uiEvent, control))
        {
            const FastName& soundEventName = soundComponent->GetSoundEventName(static_cast<UIControl::eEventType>(eventType));

            if (soundEventName.IsValid())
            {
                TriggerEvent(soundEventName, uiEvent, control);
            }
        }
    }
}

void UISoundSystem::FreeEvents()
{
    soundEvents.clear();
}

void UISoundSystem::SetGlobalParameter(const FastName& parameter, float32 value)
{
    globalParameters[parameter] = value;
}

void UISoundSystem::SetVolume(float32 aVolume)
{
    volume = aVolume;

    SoundSystem::Instance()->SetGroupVolume(UISoundSystemDetail::SOUND_GROUP, volume);
}

void UISoundSystem::TriggerEvent(const FastName& eventName, const UIEvent* uiEvent, const UIControl* control)
{
    RefPtr<SoundEvent> event = GetEvent(eventName);

    if (event->IsActive())
    {
        event->Stop();
    }

    SetupEventPan(event.Get(), uiEvent, control);
    SetupEventGlobalParameters(event.Get());

    event->Trigger();
}

void UISoundSystem::SetupEventPan(SoundEvent* event, const UIEvent* uiEvent, const UIControl* control)
{
    if (event->IsParameterExists(UISoundSystemDetail::SOUND_PARAM_PAN))
    {
        const float32 PAN_DEFAULT = 0.0f;
        const float32 PAN_LEFT = -1.0f;
        const float32 PAN_RIGHT = 1.0f;
        const float32 PAN_RANGE = PAN_RIGHT - PAN_LEFT;

        float32 pan = PAN_DEFAULT;
        float32 pointX = 0.0f;
        if (uiEvent != nullptr)
        {
            pointX = uiEvent->point.x;
        }
        else
        {
            pointX = control->GetAbsoluteRect().GetCenter().x;
        }

        pan = PAN_LEFT + PAN_RANGE * (pointX / GetPrimaryWindow()->GetVirtualSize().dx);
        event->SetParameterValue(UISoundSystemDetail::SOUND_PARAM_PAN, pan);
    }
}

void UISoundSystem::SetupEventGlobalParameters(SoundEvent* event)
{
    for (const GlobalParameterMap::value_type& param : globalParameters)
    {
        if (event->IsParameterExists(param.first))
        {
            event->SetParameterValue(param.first, param.second);
        }
    }
}

RefPtr<SoundEvent> UISoundSystem::GetEvent(const FastName& eventName)
{
    SoundEventMap::iterator eventIter = soundEvents.find(eventName);
    if (eventIter == soundEvents.end())
    {
        RefPtr<SoundEvent> event(SoundSystem::Instance()->CreateSoundEventByID(eventName, UISoundSystemDetail::SOUND_GROUP));

        soundEvents[eventName] = event;

        return event;
    }
    else
    {
        return eventIter->second;
    }
}

bool UISoundSystem::ShouldSkipEvent(int32 eventType, const UIEvent* uiEvent, const UIControl* control) const
{
    if (eventType == UIControl::EVENT_VALUE_CHANGED)
    {
        UISoundValueFilterComponent* valueFilterComponent = control->GetComponent<UISoundValueFilterComponent>();
        if (valueFilterComponent != nullptr)
        {
            // we need some kind of generic way to get control value, for now branch here
            const UISlider* slider = dynamic_cast<const UISlider*>(control);

            if (slider != nullptr)
            {
                const float32 value = slider->GetValue();

                const int32 nvalue = valueFilterComponent->normalizedValue;

                const float32 step = valueFilterComponent->GetStep();
                const float32 deadZone = valueFilterComponent->GetDeadZone();

                if ((value < nvalue * step - deadZone)
                    || (value > (nvalue + 1) * step + deadZone))
                {
                    valueFilterComponent->normalizedValue = static_cast<int32>(value / step);
                }

                return nvalue == valueFilterComponent->normalizedValue;
            }
        }
    }

    return false;
}
}
