#include "BinaryTriggerBox.h"

namespace BinaryExclusiveSetDetails
{
TriggerBox::OptionID OptionOn = 0;
TriggerBox::OptionID OptionOff = 1;
}

BinaryTriggerBox::BinaryTriggerBox(TriggerBoxListener& listener, DAVA::ScopedPtr<DAVA::Font>& font, DAVA::float32 fontSize, const DAVA::WideString& onOptionText, const DAVA::WideString& offOptionText)
    : TriggerBox(listener, font, fontSize)
{
    AddOption(BinaryExclusiveSetDetails::OptionOn, onOptionText, true);
    AddOption(BinaryExclusiveSetDetails::OptionOff, offOptionText);
}

void BinaryTriggerBox::SetOn(bool isOn)
{
    SetOptionSelected(isOn ? BinaryExclusiveSetDetails::OptionOn : BinaryExclusiveSetDetails::OptionOff);
}

bool BinaryTriggerBox::IsOn() const
{
    return (GetSelectedOptionID() == BinaryExclusiveSetDetails::OptionOn);
}
