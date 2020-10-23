#ifndef __DAVAENGINE_UI_FOCUS_SYSTEM_H__
#define __DAVAENGINE_UI_FOCUS_SYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"
#include "FocusHelpers.h"

#include "UI/Focus/UITabOrderComponent.h"
#include "UI/Focus/UINavigationComponent.h"

namespace DAVA
{
class UIControl;
class UIList;
class UIEvent;

class UIFocusSystem
{
public:
    UIFocusSystem();
    ~UIFocusSystem();

    UIControl* GetRoot() const;
    void SetRoot(UIControl* control);

    UIControl* GetFocusedControl() const;
    void SetFocusedControl(UIControl* control);

    void OnControlVisible(UIControl* control);
    void OnControlInvisible(UIControl* control);

    bool MoveFocus(UINavigationComponent::Direction dir);
    bool MoveFocus(UITabOrderComponent::Direction dir, bool repeat);

private:
    void ClearFocusState(UIControl* control);
    UIControl* FindFirstControl(UIControl* control) const;
    bool IsControlBetterForFocusThanCandidate(UIControl* c1, UIControl* c2) const;

    RefPtr<UIControl> focusedControl;
    RefPtr<UIControl> root;
};
}


#endif //__DAVAENGINE_UI_FOCUS_SYSTEM_H__
