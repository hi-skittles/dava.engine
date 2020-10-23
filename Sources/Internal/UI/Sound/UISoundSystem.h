#pragma once

#include "Base/BaseTypes.h"
#include "Sound/SoundEvent.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIControl;
class UIEvent;

class UISoundSystem
: public UISystem
{
public:
    UISoundSystem();
    ~UISoundSystem() override;

    void ProcessControlEvent(int32 eventType, const UIEvent* uiEvent, const UIControl* control);
    void FreeEvents();

    void SetGlobalParameter(const DAVA::FastName& parameter, float32 value);

    void SetVolume(float32 volume);

private:
    using SoundEventMap = UnorderedMap<FastName, RefPtr<SoundEvent>>;
    using GlobalParameterMap = UnorderedMap<FastName, float32>;

    void TriggerEvent(const FastName& eventName, const UIEvent* uiEvent, const UIControl* control);
    void SetupEventPan(SoundEvent* event, const UIEvent* uiEvent, const UIControl* control);
    void SetupEventGlobalParameters(SoundEvent* event);

    RefPtr<SoundEvent> GetEvent(const FastName& eventName);

    bool ShouldSkipEvent(int32 eventType, const UIEvent* uiEvent, const UIControl* control) const;

    GlobalParameterMap globalParameters;
    SoundEventMap soundEvents;

    float32 volume = 1.0f;
};
}
