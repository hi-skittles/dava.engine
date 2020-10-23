#pragma once

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UISoundComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UISoundComponent, UIComponent);
    DECLARE_UI_COMPONENT(UISoundComponent);

public:
    UISoundComponent();
    UISoundComponent(const UISoundComponent& src);

    virtual UISoundComponent* Clone() const override;

    const FastName& GetSoundEventName(UIControl::eEventType uiEventType) const;
    void SetSoundEventName(UIControl::eEventType uiEventType, const FastName& soundEventName);

    const FastName& GetOnTouchDownSoundEventName() const;
    void SetOnTouchDownSoundEventName(const FastName& soundEventName);

    const FastName& GetOnTouchUpInsideSoundEventName() const;
    void SetOnTouchUpInsideSoundEventName(const FastName& soundEventName);

    const FastName& GetOnTouchUpOutsideSoundEventName() const;
    void SetOnTouchUpOutsideSoundEventName(const FastName& soundEventName);

    const FastName& GetOnValueChangedSoundEventName() const;
    void SetOnValueChangedSoundEventName(const FastName& soundEventName);

protected:
    ~UISoundComponent() override = default;

private:
    UISoundComponent& operator=(const UISoundComponent&) = delete;

    DAVA::Array<FastName, UIControl::EVENTS_COUNT> soundEventNames;
};
}
