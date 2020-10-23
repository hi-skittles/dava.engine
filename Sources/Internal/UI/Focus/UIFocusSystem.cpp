#include "UIFocusSystem.h"

#include "UI/Focus/UIFocusComponent.h"

#include "UI/Focus/FocusHelpers.h"
#include "UI/Focus/DirectionBasedNavigationAlgorithm.h"
#include "UI/Focus/TabTraversalAlgorithm.h"

#include "UI/UIControl.h"
#include "UI/UIList.h"
#include "UI/UIEvent.h"
#include "UI/UIControlHelpers.h"
#include "UI/UITextField.h"

namespace DAVA
{
UIFocusSystem::UIFocusSystem()
{
}

UIFocusSystem::~UIFocusSystem()
{
}

UIControl* UIFocusSystem::GetRoot() const
{
    return root.Get();
}

void UIFocusSystem::SetRoot(UIControl* newRoot)
{
    root = newRoot;

    if (root)
    {
        UIControl* focusedControl = FindFirstControl(newRoot);
        ClearFocusState(newRoot);
        SetFocusedControl(nullptr);

        SetFocusedControl(focusedControl);
    }
    else
    {
        SetFocusedControl(nullptr);
    }
}

UIControl* UIFocusSystem::GetFocusedControl() const
{
    return focusedControl.Get();
}

void UIFocusSystem::SetFocusedControl(UIControl* control)
{
    bool textFieldWasEditing = false;

    if (control)
    {
        UIControl* c = control;
        while (c && c != root.Get())
        {
            c = c->GetParent();
        }

        if (c != root.Get())
        {
            DVASSERT(false);
            return;
        }
    }

    if (control != focusedControl.Get())
    {
        if (focusedControl.Valid())
        {
            UITextField* textField = dynamic_cast<UITextField*>(focusedControl.Get());
            if (textField)
            {
                textFieldWasEditing = textField->IsEditing();
            }

            focusedControl->SystemOnFocusLost();
            focusedControl = nullptr;
        }

        if (control != nullptr)
        {
            if (FocusHelpers::CanFocusControl(control))
            {
                focusedControl = control;
                focusedControl->SystemOnFocused();
                UIControlHelpers::ScrollToControl(focusedControl.Get());

                if (textFieldWasEditing)
                {
                    UITextField* textField = dynamic_cast<UITextField*>(focusedControl.Get());
                    if (textField)
                    {
                        textField->StartEdit();
                    }
                }
            }
            else
            {
                DVASSERT(false);
            }
        }
    }
}

void UIFocusSystem::OnControlVisible(UIControl* control)
{
    if (!focusedControl.Valid() && FocusHelpers::CanFocusControl(control) && root.Valid())
    {
        UIControl* c = control;
        while (c != nullptr && c != root.Get())
        {
            c = c->GetParent();
        }

        if (c != nullptr) // control in current hierarchy
        {
            SetFocusedControl(control);
        }
    }
}

void UIFocusSystem::OnControlInvisible(UIControl* control)
{
    if (focusedControl == control)
    {
        if (root.Valid())
        {
            UIControl* focusedControl = FindFirstControl(root.Get());
            ClearFocusState(root.Get());
            SetFocusedControl(focusedControl);
        }
        else
        {
            SetFocusedControl(nullptr);
        }
    }
}

bool UIFocusSystem::MoveFocus(UINavigationComponent::Direction dir)
{
    if (root.Valid() && focusedControl.Valid())
    {
        DirectionBasedNavigationAlgorithm alg(root.Get());
        UIControl* next = alg.GetNextControl(focusedControl.Get(), dir);
        if (next != nullptr && next != focusedControl)
        {
            SetFocusedControl(next);
            return true;
        }
    }
    return false;
}

bool UIFocusSystem::MoveFocus(UITabOrderComponent::Direction dir, bool repeat)
{
    if (root.Valid() && focusedControl.Valid())
    {
        TabTraversalAlgorithm alg(root.Get());
        UIControl* next = alg.GetNextControl(focusedControl.Get(), dir, repeat);
        if (next != nullptr && next != focusedControl)
        {
            SetFocusedControl(next);
            return true;
        }
    }
    return false;
}

void UIFocusSystem::ClearFocusState(UIControl* control)
{
    control->SetState(control->GetState() & (~UIControl::STATE_FOCUSED));
    for (const auto& c : control->GetChildren())
    {
        ClearFocusState(c.Get());
    }
}

UIControl* UIFocusSystem::FindFirstControl(UIControl* control) const
{
    UIControl* candidate = nullptr;
    for (const auto& c : control->GetChildren())
    {
        UIControl* res = FindFirstControl(c.Get());
        if (res != nullptr && IsControlBetterForFocusThanCandidate(res, candidate))
        {
            candidate = res;
        }
    }

    if (candidate == nullptr && FocusHelpers::CanFocusControl(control))
    {
        return control;
    }

    return candidate;
}

bool UIFocusSystem::IsControlBetterForFocusThanCandidate(UIControl* c1, UIControl* c2) const
{
    DVASSERT(c1 != nullptr);
    if (c2 == nullptr)
    {
        return true;
    }

    UIFocusComponent* f1 = c1->GetComponent<UIFocusComponent>();
    DVASSERT(f1 != nullptr);
    UIFocusComponent* f2 = c2->GetComponent<UIFocusComponent>();
    DVASSERT(f2 != nullptr);

    if ((c1->GetState() & UIControl::STATE_FOCUSED) != 0 && (c2->GetState() & UIControl::STATE_FOCUSED) == 0)
    {
        return true;
    }

    if (f1->IsRequestFocus() && !f2->IsRequestFocus())
    {
        return true;
    }

    return false;
}
}
