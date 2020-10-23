#include "FocusHelpers.h"

#include "UIFocusComponent.h"

#include "UI/UIControl.h"

namespace DAVA
{
bool FocusHelpers::CanFocusControl(UIControl* control)
{
    if (control == nullptr || !control->IsVisible() || control->GetDisabled())
    {
        return false;
    }

    UIFocusComponent* focus = control->GetComponent<UIFocusComponent>();
    return focus != nullptr && focus->IsEnabled();
}
}
