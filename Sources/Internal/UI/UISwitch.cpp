#include "UISwitch.h"
#include "Animation/LinearAnimation.h"
#include "UI/UIEvent.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
//use these names for children controls to define UISwitch in .yaml
const FastName UISwitch::BUTTON_LEFT_NAME("buttonLeft");
const FastName UISwitch::BUTTON_RIGHT_NAME("buttonRight");
const FastName UISwitch::BUTTON_TOGGLE_NAME("buttonToggle");

static const float32 UISWITCH_SWITCH_ANIMATION_TIME = 0.1f;
static const int32 UISWITCH_MOVE_ANIMATION_TRACK = 10;
static const float32 UISWITCH_ANCHOR_UNDEFINED = 10000.f;
static float32 dragAnchorX = UISWITCH_ANCHOR_UNDEFINED;

DAVA_VIRTUAL_REFLECTION_IMPL(UISwitch)
{
    ReflectionRegistrator<UISwitch>::Begin()[M::DisplayName("Switch")]
    .ConstructorByPointer()
    .DestructorByPointer([](UISwitch* o) { o->Release(); })
    .Field("isLeftSelected", &UISwitch::GetIsLeftSelected, &UISwitch::SetIsLeftSelected)[M::Bindable()]
    .End();
}

class TogglePositionAnimation : public LinearAnimation<float32>
{
protected:
    virtual ~TogglePositionAnimation()
    {
        SafeRelease(uiSwitch);
    }

public:
    TogglePositionAnimation(bool _isCausedByTap, UISwitch* _uiSwitch, float32* _var, float32 _endValue, float32 _animationTimeLength, Interpolation::FuncType _iType)
        : LinearAnimation(_uiSwitch->GetToggle(), _var, _endValue, _animationTimeLength, _iType)
        , uiSwitch(SafeRetain(_uiSwitch))
        , isFromLeftToRight(false)
        , centerNotPassed(_isCausedByTap) //center is not yet passed by in this case
        , centerPos(0.f)
    {
        if (_isCausedByTap) //toggle is on opposite side from _endValue, we can calculate center
        {
            centerPos = (_endValue + *_var) / 2;
            isFromLeftToRight = _endValue > *_var;
        }
    }

    virtual void Update(float32 timeElapsed)
    {
        LinearAnimation::Update(timeElapsed);
        if (centerNotPassed)
        {
            if (isFromLeftToRight ^ (*var < centerPos))
            {
                centerNotPassed = false;
                uiSwitch->ChangeVisualState();
            }
        }
    }

private:
    UISwitch* uiSwitch;
    bool isFromLeftToRight;
    bool centerNotPassed;
    float32 centerPos;
};

UISwitch::UISwitch(const Rect& rect)
    : UIControl(rect)
    , buttonLeft(new UIControl())
    , buttonRight(new UIControl())
    , toggle(new UIControl())
    , switchOnTapBesideToggle(true)
{
    buttonLeft->SetName(UISwitch::BUTTON_LEFT_NAME);
    buttonRight->SetName(UISwitch::BUTTON_RIGHT_NAME);
    toggle->SetName(UISwitch::BUTTON_TOGGLE_NAME);
    AddControl(buttonLeft.Get());
    AddControl(buttonRight.Get());
    AddControl(toggle.Get());
    InitControls();

    Vector2 leftAndRightSize(size.dx / 2, size.dy);
    buttonLeft->SetSize(leftAndRightSize);
    buttonRight->SetSize(leftAndRightSize);
    Vector2 newPivotPoint = buttonRight->GetPivotPoint();
    newPivotPoint.x = leftAndRightSize.dx;
    buttonRight->SetPivotPoint(newPivotPoint);
    buttonRight->SetPosition(Vector2(size.x, buttonRight->GetPosition().y));
}

UISwitch::~UISwitch()
{
}

void UISwitch::InitControls()
{
    buttonLeft->SetInputEnabled(false);
    buttonRight->SetInputEnabled(false);
    toggle->SetInputEnabled(false);
    BringChildFront(toggle.Get());
    CheckToggleSideChange();
    float32 toggleXPosition = GetToggleUttermostPosition();
    toggle->SetPosition(Vector2(toggleXPosition, toggle->GetPosition().y));
    ChangeVisualState(); //forcing visual state change cause it can be skipped in CheckToggleSideChange()
}

void UISwitch::AddControl(UIControl* control)
{
    // Synchronize the pointers to the buttons each time new control is added.
    UIControl::AddControl(control);

    if (control->GetName() == UISwitch::BUTTON_LEFT_NAME && buttonLeft.Get() != control)
    {
        UIControl::RemoveControl(buttonLeft.Get());
        buttonLeft = control;
    }
    else if (control->GetName() == UISwitch::BUTTON_TOGGLE_NAME && toggle.Get() != control)
    {
        UIControl::RemoveControl(toggle.Get());
        toggle = control;
    }
    else if (control->GetName() == UISwitch::BUTTON_RIGHT_NAME && buttonRight.Get() != control)
    {
        UIControl::RemoveControl(buttonRight.Get());
        buttonRight = control;
    }
}

void UISwitch::RemoveControl(UIControl* control)
{
    if (control == buttonRight.Get())
    {
        buttonRight = nullptr;
    }
    else if (control == buttonLeft.Get())
    {
        buttonLeft = nullptr;
    }
    else if (control == toggle.Get())
    {
        toggle = nullptr;
    }

    UIControl::RemoveControl(control);
}

void UISwitch::CopyDataFrom(UIControl* srcControl)
{
    //release default buttons - they have to be copied from srcControl
    buttonLeft = nullptr;
    buttonRight = nullptr;
    toggle = nullptr;

    UIControl::CopyDataFrom(srcControl);

    InitControls();
}

UISwitch* UISwitch::Clone()
{
    UISwitch* t = new UISwitch(GetRect());
    t->CopyDataFrom(this);
    return t;
}

void UISwitch::LoadFromYamlNodeCompleted()
{
    InitControls();
}

void UISwitch::Input(UIEvent* currentInput)
{
    if (toggle->IsAnimating(UISWITCH_MOVE_ANIMATION_TRACK))
    {
        return;
    }

    Vector2 touchPos = currentInput->point;
    if (currentInput->phase == UIEvent::Phase::BEGAN)
    {
        if (toggle->IsPointInside(touchPos))
        {
            dragAnchorX = touchPos.x - toggle->GetPosition().x;
            toggle->SetSelected(true);
        }
        else
        {
            dragAnchorX = UISWITCH_ANCHOR_UNDEFINED;
        }
    }
    else if (currentInput->phase == UIEvent::Phase::DRAG)
    {
        if (dragAnchorX < UISWITCH_ANCHOR_UNDEFINED)
        {
            CheckToggleSideChange(currentInput);

            float32 newToggleX = touchPos.x - dragAnchorX;
            float32 newToggleLeftEdge = newToggleX - toggle->GetPivotPoint().x;

            float32 leftBound = buttonLeft->GetPosition().x;
            float32 rightBound = buttonRight->GetPosition().x;
            Vector2 togglePos = toggle->GetPosition();
            if (newToggleLeftEdge < leftBound)
            {
                togglePos.x = GetToggleLeftPosition();
            }
            else if (newToggleLeftEdge + toggle->GetSize().dx > rightBound)
            {
                togglePos.x = GetToggleRightPosition();
            }
            else
            {
                togglePos.x = newToggleX;
            }
            toggle->SetPosition(togglePos);
        }
    }
    else if (currentInput->phase == UIEvent::Phase::ENDED || currentInput->phase == UIEvent::Phase::CANCELLED)
    {
        if (dragAnchorX < UISWITCH_ANCHOR_UNDEFINED)
        {
            CheckToggleSideChange(currentInput);
            toggle->SetSelected(false);
        }
        else if (switchOnTapBesideToggle)
        {
            InternalSetIsLeftSelected(!isLeftSelected, false, currentInput); //switch logical state immediately,
        }
        float32 toggleX = GetToggleUttermostPosition();

        bool causedByTap = dragAnchorX >= UISWITCH_ANCHOR_UNDEFINED;
        Animation* animation = new TogglePositionAnimation(causedByTap, this, &(toggle->relativePosition.x), toggleX, UISWITCH_SWITCH_ANIMATION_TIME, Interpolation::EASY_IN);
        animation->Start(UISWITCH_MOVE_ANIMATION_TRACK);
    }

    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
}

void UISwitch::SetIsLeftSelected(bool aIsLeftSelected)
{
    if (!buttonLeft.Valid() || !buttonRight.Valid() || !toggle.Valid())
    {
        return;
    }

    InternalSetIsLeftSelected(aIsLeftSelected, true);
    float32 toggleXPosition = GetToggleUttermostPosition();
    toggle->StopAnimations(UISWITCH_MOVE_ANIMATION_TRACK);
    toggle->SetPosition(Vector2(toggleXPosition, toggle->GetPosition().y));
}

void UISwitch::InternalSetIsLeftSelected(bool aIsLeftSelected, bool changeVisualState, UIEvent* inputEvent /*= NULL*/)
{
    bool prevIsLeftSelected = isLeftSelected;
    isLeftSelected = aIsLeftSelected;
    if (prevIsLeftSelected != isLeftSelected)
    {
        if (changeVisualState)
        {
            ChangeVisualState();
        }

        PerformEventWithData(EVENT_VALUE_CHANGED, inputEvent, inputEvent);
    }
}

void UISwitch::ChangeVisualState()
{
    buttonLeft->SetSelected(isLeftSelected);
    buttonRight->SetSelected(!isLeftSelected);
    SetSelected(!isLeftSelected);
    BringChildBack(isLeftSelected ? buttonLeft.Get() : buttonRight.Get());
}

float32 UISwitch::GetToggleUttermostPosition() const
{
    return isLeftSelected ? GetToggleLeftPosition() : GetToggleRightPosition();
}

float32 UISwitch::GetToggleLeftPosition() const
{
    return buttonLeft->GetPosition().x + toggle->GetPivotPoint().x;
}

float32 UISwitch::GetToggleRightPosition() const
{
    return buttonRight->GetPosition().x - toggle->GetSize().dx + toggle->GetPivotPoint().x;
}

void UISwitch::CheckToggleSideChange(UIEvent* inputEvent /*= NULL*/)
{
    float32 leftBound = buttonLeft->GetPosition().x;
    float32 rightBound = buttonRight->GetPosition().x;
    float32 toggleCenter = toggle->GetPosition().x - toggle->GetPivotPoint().x + toggle->GetSize().dx / 2;
    float32 toggleSpaceCenter = (leftBound + rightBound) / 2;
    InternalSetIsLeftSelected(toggleCenter < toggleSpaceCenter, true, inputEvent);
}
}
