#ifndef __DAVAENGINE_UI_SCROLLVIEWCONTAINER__
#define __DAVAENGINE_UI_SCROLLVIEWCONTAINER__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class ScrollHelper;

class UIScrollViewContainer : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIScrollViewContainer, UIControl);

protected:
    virtual ~UIScrollViewContainer();

public:
    UIScrollViewContainer(const Rect& rect = Rect());

    UIScrollViewContainer* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

public:
    void Update(float32 timeElapsed) override;
    void Input(UIEvent* currentTouch) override;
    void InputCancelled(UIEvent* currentInput) override;
    bool SystemInput(UIEvent* currentInput) override;
    void SetSize(const Vector2& size) override;
    void SetPosition(const Vector2& pos) override;
    void OnInactive() override;

    // Set container relative position and enable*Scroll properties based on self and parent size
    void ApplySizeChanges();

    // The amount of pixels user must move the finger on the button to switch from button to scrolling (default 15)
    void SetTouchTreshold(int32 holdDelta);
    int32 GetTouchTreshold();

protected:
    enum
    {
        STATE_NONE = 0,
        STATE_BEFORE_SCROLL,
        STATE_SCROLL,
        STATE_ZOOM,
        STATE_DECCELERATION,
        STATE_SCROLL_TO_SPECIAL,
    };

    int32 state;
    // Scroll information
    Vector2 scrollStartInitialPosition; // position of click
    int32 touchTreshold;

    int mainTouch;

    Vector2 oldPos;
    Vector2 newPos;

    Vector2 oldScroll;
    Vector2 newScroll;

    ScrollHelper* currentScroll;

    // All boolean variables are grouped together because of DF-2149.
    bool lockTouch : 1;
    bool enableHorizontalScroll : 1;
    bool enableVerticalScroll : 1;
};
};

#endif /* defined(__DAVAENGINE_UI_SCROLLVIEWCONTAINER__) */
