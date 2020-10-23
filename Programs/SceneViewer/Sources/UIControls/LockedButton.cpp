#include "LockedButton.h"

LockedButton::LockedButton(LockedButtonHolder& holder, DAVA::Font* font, const DAVA::WideString& text, const DAVA::Rect& rect)
    : DAVA::UIButton(rect)
    , holder(holder)
{
    using namespace DAVA;

    SetStateDrawType(UIControl::STATE_NORMAL, UIControlBackground::DRAW_FILL);
    GetStateBackground(UIControl::STATE_NORMAL)->SetColor(Color(0.5f, 0.6f, 0.5f, 0.5f));
    SetStateDrawType(UIControl::STATE_SELECTED, UIControlBackground::DRAW_FILL);
    GetStateBackground(UIControl::STATE_SELECTED)->SetColor(Color(0.75f, 0.85f, 0.75f, 0.5f));
    SetStateFont(UIControl::STATE_NORMAL, font);
    SetStateTextColorInheritType(UIControl::STATE_NORMAL, UIControlBackground::COLOR_IGNORE_PARENT);
    SetStateText(UIControl::STATE_NORMAL | UIControl::STATE_PRESSED_INSIDE | UIControl::STATE_SELECTED, text);

    AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &LockedButton::OnPressed));
}

bool LockedButton::IsLocked() const
{
    return GetState() == DAVA::UIControl::STATE_SELECTED;
}

void LockedButton::SetLocked(bool selected)
{
    SetState(selected ? DAVA::UIControl::STATE_SELECTED : DAVA::UIControl::STATE_NORMAL);
    SetInputEnabled(!selected, false);
}

void LockedButton::OnPressed(DAVA::BaseObject* caller, void* param, void* callerData)
{
    SetSelected(true);
    holder.OnButtonPressed(this);
}
