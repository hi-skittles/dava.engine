#pragma once

#include "TriggerBox.h"

class BinaryTriggerBox : public TriggerBox
{
public:
    BinaryTriggerBox(TriggerBoxListener& listener,
                     DAVA::ScopedPtr<DAVA::Font>& font,
                     DAVA::float32 fontSize,
                     const DAVA::WideString& onOptionText,
                     const DAVA::WideString& offOptionText);
    void SetOn(bool isOn);
    bool IsOn() const;
};
