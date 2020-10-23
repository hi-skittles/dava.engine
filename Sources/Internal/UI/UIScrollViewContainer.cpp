#include "UI/UIScrollViewContainer.h"
#include "Engine/Engine.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/Input/UIInputSystem.h"
#include "UI/Scroll/UIScrollComponent.h"
#include "UI/ScrollHelper.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScrollView.h"

namespace DAVA
{
const int32 DEFAULT_TOUCH_TRESHOLD = 15; // Default value for finger touch tresshold

DAVA_VIRTUAL_REFLECTION_IMPL(UIScrollViewContainer)
{
    ReflectionRegistrator<UIScrollViewContainer>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIScrollViewContainer* o) { o->Release(); })
    .End();
}

UIScrollViewContainer::UIScrollViewContainer(const Rect& rect)
    : UIControl(rect)
    , state(STATE_NONE)
    , touchTreshold(DEFAULT_TOUCH_TRESHOLD)
    , mainTouch(-1)
    , oldPos(0.f, 0.f)
    , newPos(0.f, 0.f)
    , currentScroll(NULL)
    , lockTouch(false)
    , enableHorizontalScroll(true)
    , enableVerticalScroll(true)
{
    this->SetInputEnabled(true);
    this->SetMultiInput(true);
    GetOrCreateComponent<UIScrollComponent>();
}

UIScrollViewContainer::~UIScrollViewContainer()
{
}

UIScrollViewContainer* UIScrollViewContainer::Clone()
{
    UIScrollViewContainer* t = new UIScrollViewContainer(GetRect());
    t->CopyDataFrom(this);
    return t;
}

void UIScrollViewContainer::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
}

void UIScrollViewContainer::SetSize(const Vector2& size)
{
    UIControl::SetSize(size);
    ApplySizeChanges();
}

void UIScrollViewContainer::SetPosition(const Vector2& pos)
{
    UIControl::SetPosition(pos);
    ApplySizeChanges();
}

void UIScrollViewContainer::ApplySizeChanges()
{
    UIControl* parent = GetParent();
    if (parent)
    {
        const Vector2& size = GetSize();
        const Vector2& parentSize = parent->GetSize();
        // We should not allow scrolling when content rect is less than or is equal ScrollView "window"
        enableHorizontalScroll = size.dx > parentSize.dx;
        enableVerticalScroll = size.dy > parentSize.dy;
        Array<bool, Vector2::AXIS_COUNT> enableScroll;
        enableScroll[Vector2::AXIS_X] = enableHorizontalScroll;
        enableScroll[Vector2::AXIS_Y] = enableVerticalScroll;

        UIScrollView* scrollView = CastIfEqual<UIScrollView*>(parent);
        if (scrollView != nullptr)
        {
            scrollView->OnScrollViewContainerSizeChanged();

            if (scrollView->IsAutoUpdate())
            {
                for (int32 axis = 0; axis < Vector2::AXIS_COUNT; axis++)
                {
                    if (!enableScroll[axis])
                    {
                        if (scrollView->IsCenterContent())
                        {
                            relativePosition.data[axis] = (parentSize.data[axis] - size.data[axis]) / 2;
                        }
                        else
                        {
                            relativePosition.data[axis] = 0;
                        }
                    }
                }
            }
        }
    }
}

void UIScrollViewContainer::SetTouchTreshold(int32 holdDelta)
{
    touchTreshold = holdDelta;
}
int32 UIScrollViewContainer::GetTouchTreshold()
{
    return touchTreshold;
}

void UIScrollViewContainer::Input(UIEvent* currentTouch)
{
}

bool UIScrollViewContainer::SystemInput(UIEvent* currentTouch)
{
    if (!GetInputEnabled() || !visible || (GetState() & STATE_DISABLED))
    {
        return UIControl::SystemInput(currentTouch);
    }

    bool systemInput = UIControl::SystemInput(currentTouch);
    if (currentTouch->GetInputHandledType() == UIEvent::INPUT_HANDLED_HARD)
    {
        // Can't scroll - some child control already processed this input.
        mainTouch = -1;
        lockTouch = false;
        return systemInput;
    }

    if (currentTouch->touchId == mainTouch && STATE_SCROLL == state)
    {
        // Refresh 'scroll' target point
        newPos = currentTouch->point;
    }

    switch (currentTouch->phase)
    {
    case UIEvent::Phase::BEGAN:
    {
        if (currentTouch->touchId != mainTouch &&
            mainTouch != -1 &&
            !GetEngineContext()->uiControlSystem->GetInputSystem()->IsAnyInputLockedByControl(this))
        {
            // Workaround for missed 'CancelInput()' in rare case.
            // Force reset internal 'touch id' when control has not active 'touchLocker' in UIInputSystem.
            mainTouch = -1;
            lockTouch = false;
        }

        if (mainTouch == -1 && IsPointInside(currentTouch->point))
        {
            currentScroll = nullptr;
            PerformEvent(EVENT_TOUCH_DOWN, currentTouch);

            mainTouch = currentTouch->touchId;
            scrollStartInitialPosition = currentTouch->point;
            newPos = currentTouch->point;
            oldPos = newPos;
            state = STATE_BEFORE_SCROLL;
            lockTouch = true;
        }
    }
    break;
    case UIEvent::Phase::DRAG:
    {
        if (currentTouch->touchId == mainTouch)
        {
            if (state == STATE_BEFORE_SCROLL)
            {
                // Don't scroll if touchTreshold is not exceeded
                if ((Abs(currentTouch->point.x - scrollStartInitialPosition.x) > touchTreshold) ||
                    (Abs(currentTouch->point.y - scrollStartInitialPosition.y) > touchTreshold))
                {
                    state = STATE_SCROLL;
                }
            }
            if (state == STATE_SCROLL)
            {
                UIScrollView* scrollView = DynamicTypeCheck<UIScrollView*>(this->GetParent());
                DVASSERT(scrollView);
                if (enableHorizontalScroll
                    && Abs(currentTouch->point.x - scrollStartInitialPosition.x) > touchTreshold
                    && (!currentScroll || currentScroll == scrollView->GetHorizontalScroll()))
                {
                    currentScroll = scrollView->GetHorizontalScroll();
                }
                else if (enableVerticalScroll
                         && (Abs(currentTouch->point.y - scrollStartInitialPosition.y) > touchTreshold)
                         && (!currentScroll || currentScroll == scrollView->GetVerticalScroll()))
                {
                    currentScroll = scrollView->GetVerticalScroll();
                }
                if (currentTouch->touchLocker != this && currentScroll)
                {
                    GetEngineContext()->uiControlSystem->SwitchInputToControl(mainTouch, this);
                }
            }
        }
    }
    break;
    case UIEvent::Phase::ENDED:
    {
        if (currentTouch->touchId == mainTouch)
        {
            mainTouch = -1;
            lockTouch = false;
            state = STATE_DECCELERATION;
        }
    }
    break;
    case UIEvent::Phase::WHEEL:
    {
        Vector2 wheelDelta(currentTouch->wheelDelta.x * GetWheelSensitivity(), currentTouch->wheelDelta.y * GetWheelSensitivity());
        if (currentTouch->device == eInputDevices::MOUSE && (currentTouch->modifiers & eModifierKeys::SHIFT) != eModifierKeys::NONE)
        {
            std::swap(wheelDelta.x, wheelDelta.y);
        }
        newScroll += wheelDelta;
    }
    break;
    default:
        break;
    }

    if (STATE_SCROLL == state && currentTouch->touchId == mainTouch)
    {
        return true;
    }

    return systemInput;
}

void UIScrollViewContainer::Update(float32 timeElapsed)
{
    UIScrollView* scrollView = CastIfEqual<UIScrollView*>(this->GetParent());
    if (scrollView)
    {
        const float32 accuracyDelta = 0.1f;
        Vector2 posDelta = newPos - oldPos;
        oldPos = newPos;

        Vector2 deltaScroll = newScroll - oldScroll;
        oldScroll = newScroll;

        // Get scrolls positions and change scroll container relative position
        if (enableHorizontalScroll)
        {
            if (accuracyDelta <= Abs(deltaScroll.x))
            {
                float32 dx = scrollView->GetRect().dx;
                scrollView->GetHorizontalScroll()->ScrollWithoutAnimation(deltaScroll.x, dx, &relativePosition.x);
            }
            else
            {
                if (scrollView->GetHorizontalScroll() == currentScroll)
                {
                    relativePosition.x = currentScroll->GetPosition(posDelta.x, timeElapsed, lockTouch);
                }
                else
                {
                    relativePosition.x = scrollView->GetHorizontalScroll()->GetPosition(0, timeElapsed, false);
                }
            }
        }
        else if (scrollView->IsAutoUpdate())
        {
            if (scrollView->IsCenterContent())
            {
                relativePosition.x = (scrollView->GetSize().dx - GetSize().dx) / 2;
            }
            else
            {
                relativePosition.x = 0;
            }
        }

        if (enableVerticalScroll)
        {
            if (accuracyDelta <= Abs(deltaScroll.y))
            {
                float32 dy = scrollView->GetRect().dy;
                scrollView->GetVerticalScroll()->ScrollWithoutAnimation(deltaScroll.y, dy, &relativePosition.y);
            }
            else
            {
                if (scrollView->GetVerticalScroll() == currentScroll)
                {
                    relativePosition.y = currentScroll->GetPosition(posDelta.y, timeElapsed, lockTouch);
                }
                else
                {
                    relativePosition.y = scrollView->GetVerticalScroll()->GetPosition(0, timeElapsed, false);
                }
            }
        }
        else if (scrollView->IsAutoUpdate())
        {
            if (scrollView->IsCenterContent())
            {
                relativePosition.y = (scrollView->GetSize().dy - GetSize().dy) / 2;
            }
            else
            {
                relativePosition.y = 0;
            }
        }

        // Change state when scrolling is not active
        if (state != STATE_NONE && !lockTouch && (scrollView->GetHorizontalScroll()->GetCurrentSpeed() == 0) && (scrollView->GetVerticalScroll()->GetCurrentSpeed() == 0))
        {
            state = STATE_NONE;
        }
    }
}

void UIScrollViewContainer::InputCancelled(UIEvent* currentInput)
{
    if (currentInput->touchId == mainTouch)
    {
        mainTouch = -1;
        lockTouch = false;
    }
}

void UIScrollViewContainer::OnInactive()
{
    mainTouch = -1;
    lockTouch = false;
    state = STATE_NONE;
}
};
