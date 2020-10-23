#ifndef __DAVAENGINE_UI_SWITCH_H__
#define __DAVAENGINE_UI_SWITCH_H__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class TogglePositionAnimation;

/*
 * buttonLeft.relativePosition.x and buttonRight.relativePosition.x mark movement bounds for 'toggle': it does mean that toggle never leaves 
 * space between pivot points of these controls. So if you want to have toggle that moves between the left edge of 'buttonLeft' and 
 * the right edge of 'buttonRight' set buttonLeft.pivotPoint.x = 0 and buttonRight.pivotPoint.x = buttonRight.size.dx.
 *
 * Default sizes for buttonLeft, buttonRight and toggle are set only once in UISwitch ctor and only if you provide 'rect' argument.
 * After this they never adjust their sizes and it's your responsibility to control this parameter.
 *
 * Don't use GetButtonXXX()->GetSelected() to determine what side is currently selected: GetIsLeftSelected() is what you need.
 */
class UISwitch : public UIControl
{
    friend class TogglePositionAnimation;
    DAVA_VIRTUAL_REFLECTION(UISwitch, UIControl);

protected:
    virtual ~UISwitch();

public:
    UISwitch(const Rect& rect = Rect());

    void LoadFromYamlNodeCompleted() override;
    void CopyDataFrom(DAVA::UIControl* srcControl) override;

    virtual void AddControl(UIControl* control) override;
    virtual void RemoveControl(UIControl* control) override;
    virtual UISwitch* Clone() override;

    virtual void Input(UIEvent* currentInput) override;

    bool GetIsLeftSelected() const
    {
        return isLeftSelected;
    }
    void SetIsLeftSelected(bool aIsLeftSelected);

    UIControl* GetButtonNext() const
    {
        return buttonLeft.Get();
    }
    UIControl* GetButtonPrevious() const
    {
        return buttonRight.Get();
    }
    UIControl* GetToggle() const
    {
        return toggle.Get();
    }

    /*
     * If tap on any place beside toggle must provoke switch of controls state.
     */
    void SetSwitchOnTapBesideToggle(bool aSwitchOnTapBesideToggle)
    {
        switchOnTapBesideToggle = aSwitchOnTapBesideToggle;
    }
    bool GetSwitchOnTapBesideToggle() const
    {
        return switchOnTapBesideToggle;
    }

protected:
    void InternalSetIsLeftSelected(bool aIsLeftSelected, bool changeVisualState, UIEvent* inputEvent = NULL);
    void InitControls();
    void ReleaseControls();

    float32 GetToggleUttermostPosition() const;
    float32 GetToggleLeftPosition() const;
    float32 GetToggleRightPosition() const;
    void CheckToggleSideChange(UIEvent* inputEvent = NULL);
    void ChangeVisualState();

    RefPtr<UIControl> buttonLeft;
    RefPtr<UIControl> buttonRight;
    RefPtr<UIControl> toggle;

    // Boolean variables are grouped together because of DF-2149.
    bool switchOnTapBesideToggle : 1;
    bool isLeftSelected : 1;

public:
    static const FastName BUTTON_LEFT_NAME;
    static const FastName BUTTON_RIGHT_NAME;
    static const FastName BUTTON_TOGGLE_NAME;
};
}
#endif //__DAVAENGINE_UI_SWITCH_H__
