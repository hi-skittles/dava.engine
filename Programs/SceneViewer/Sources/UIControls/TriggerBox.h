#pragma once

#include "LockedButton.h"

#include <UI/UIControl.h>
#include <Base/BaseTypes.h>

class TriggerBox;
class TriggerBoxListener
{
public:
    virtual ~TriggerBoxListener()
    {
    }
    virtual void OnOptionChanged(TriggerBox*) = 0;
};

class TriggerBox : public DAVA::UIControl, public LockedButtonHolder
{
public:
    using OptionID = DAVA::uint32;

    explicit TriggerBox(TriggerBoxListener& listener, DAVA::ScopedPtr<DAVA::Font>& font, DAVA::float32 fontSize);

    bool AddOption(OptionID optionId, const DAVA::WideString& text, bool toSelect = false);
    void SetOptionSelected(OptionID optionID);
    OptionID GetSelectedOptionID() const;

private:
    using OptionsMap = DAVA::UnorderedMap<OptionID, DAVA::ScopedPtr<LockedButton>>;

    void OnButtonPressed(LockedButton* button) override;
    OptionsMap::iterator FindOptionByValue(LockedButton* button);

private:
    TriggerBoxListener& listener;
    DAVA::ScopedPtr<DAVA::Font> font = nullptr;
    DAVA::float32 fontSize = 14.f;
    DAVA::float32 nextButtonX = 10.f;
    OptionsMap options;
    OptionsMap::iterator selectedOption = options.end();
};
