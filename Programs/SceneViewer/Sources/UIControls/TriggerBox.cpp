#include "TriggerBox.h"

#include <Render/2D/Font.h>
#include <UI/Layouts/UIAnchorComponent.h>
#include <Utils/StringFormat.h>

TriggerBox::TriggerBox(TriggerBoxListener& listener, DAVA::ScopedPtr<DAVA::Font>& font, DAVA::float32 fontSize)
    : listener(listener)
    , font(font)
    , fontSize(fontSize)
{
}

bool TriggerBox::AddOption(TriggerBox::OptionID optionId, const DAVA::WideString& text, bool toSelect /* = false*/)
{
    using namespace DAVA;

    OptionsMap::iterator optionIter = options.find(optionId);
    if (optionIter != options.end())
    {
        DVASSERT(false, Format("Option id %u is already added", optionId).c_str());
        return false;
    }

    if (toSelect == true && selectedOption != options.end())
    {
        DVASSERT(false, "Another option is already selected");
        return false;
    }

    float32 textWidth = font->GetStringMetrics(fontSize, text).width;
    float32 buttonWidth = textWidth + 20.f;
    float32 buttonHeight = fontSize * 1.5f;
    Rect buttonRect(nextButtonX, 0.f, buttonWidth, buttonHeight);
    nextButtonX += buttonWidth + 10.f;
    ScopedPtr<LockedButton> lockedButton(new LockedButton(*this, font, text, buttonRect));

    UIAnchorComponent* anchor = lockedButton->GetOrCreateComponent<UIAnchorComponent>();
    anchor->SetVCenterAnchorEnabled(true);

    optionIter = options.emplace_hint(optionIter, optionId, lockedButton);
    if (toSelect)
    {
        selectedOption = optionIter;
        LockedButton* button = selectedOption->second;
        button->SetLocked(true);
    }

    AddControl(lockedButton);

    return true;
}

void TriggerBox::SetOptionSelected(TriggerBox::OptionID optionID)
{
    DVASSERT(selectedOption != options.end());

    OptionsMap::iterator it = options.find(optionID);
    DVASSERT(it != options.end());

    if (it != selectedOption)
    {
        selectedOption->second->SetLocked(false);
        it->second->SetLocked(true);
        selectedOption = it;
    }
}

TriggerBox::OptionID TriggerBox::GetSelectedOptionID() const
{
    DVASSERT(selectedOption != options.end());
    return selectedOption->first;
}

void TriggerBox::OnButtonPressed(LockedButton* button)
{
    DVASSERT(selectedOption != options.end());
    selectedOption->second->SetLocked(false);
    selectedOption = FindOptionByValue(button);
    DVASSERT(selectedOption != options.end());
    selectedOption->second->SetLocked(true);
    listener.OnOptionChanged(this);
}

TriggerBox::OptionsMap::iterator TriggerBox::FindOptionByValue(LockedButton* button)
{
    return std::find_if(options.begin(), options.end(), [button](OptionsMap::value_type& entry)
                        {
                            return entry.second == button;
                        });
}
