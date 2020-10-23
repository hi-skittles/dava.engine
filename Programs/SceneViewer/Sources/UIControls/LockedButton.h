#pragma once

#include <UI/UIButton.h>

class LockedButton;
class LockedButtonHolder
{
public:
    virtual ~LockedButtonHolder()
    {
    }
    virtual void OnButtonPressed(LockedButton*) = 0;
};

class LockedButton : public DAVA::UIButton
{
public:
    explicit LockedButton(LockedButtonHolder& holder, DAVA::Font* font, const DAVA::WideString& text, const DAVA::Rect& rect);

    bool IsLocked() const;
    void SetLocked(bool selected);

private:
    void OnPressed(DAVA::BaseObject* caller, void* param, void* callerData);

    LockedButtonHolder& holder;
};
