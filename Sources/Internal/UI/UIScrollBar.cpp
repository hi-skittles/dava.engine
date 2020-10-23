#include "UI/UIScrollBar.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIEvent.h"
#include "UI/UIScrollBarDelegate.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
//use these names for children controls to define UIScrollBar in .yaml
static const FastName UISCROLLBAR_SLIDER_NAME("slider");

DAVA_VIRTUAL_REFLECTION_IMPL(UIScrollBar)
{
    ReflectionRegistrator<UIScrollBar>::Begin()[M::DisplayName("Scroll Bar")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIScrollBar* o) { o->Release(); })
    .Field("orientation", &UIScrollBar::GetOrientation, &UIScrollBar::SetOrientation)[M::EnumT<eScrollOrientation>(), M::DisplayName("Orientation")]
    .End();
}

UIScrollBar::UIScrollBar(const Rect& rect, eScrollOrientation requiredOrientation)
    : UIControl(rect)
    , orientation(requiredOrientation)
    , delegate(NULL)
    , slider(NULL)
    , resizeSliderProportionally(true)
{
    InitControls(rect);
}

UIScrollBar::~UIScrollBar()
{
    SafeRelease(slider);
}

void UIScrollBar::SetDelegate(UIScrollBarDelegate* newDelegate)
{
    delegate = newDelegate;
}

UIControl* UIScrollBar::GetSlider()
{
    return slider;
}

void UIScrollBar::AddControl(UIControl* control)
{
    // Synchronize the pointers to the buttons each time new control is added.
    UIControl::AddControl(control);

    if (control->GetName() == UISCROLLBAR_SLIDER_NAME && slider != control)
    {
        SafeRelease(slider);
        slider = SafeRetain(control);
    }
}

void UIScrollBar::RemoveControl(UIControl* control)
{
    if (control == slider)
    {
        SafeRelease(slider);
    }

    UIControl::RemoveControl(control);
}

UIScrollBar* UIScrollBar::Clone()
{
    UIScrollBar* t = new UIScrollBar(GetRect());
    t->CopyDataFrom(this);
    return t;
}

void UIScrollBar::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);

    UIScrollBar* src = static_cast<UIScrollBar*>(srcControl);
    orientation = src->orientation;
    resizeSliderProportionally = src->resizeSliderProportionally;
}

void UIScrollBar::InitControls(const Rect& rect)
{
    ScopedPtr<UIControl> slider(new UIControl(Rect(0, 0, rect.dx, rect.dy)));
    slider->SetName(UISCROLLBAR_SLIDER_NAME);
    slider->SetInputEnabled(false, false);
    AddControl(slider);
}

void UIScrollBar::LoadFromYamlNodeCompleted()
{
    if (!slider)
    {
        InitControls();
    }
}

void UIScrollBar::Input(UIEvent* currentInput)
{
    if (!delegate)
    {
        return;
    }

    if ((currentInput->phase == UIEvent::Phase::BEGAN) ||
        (currentInput->phase == UIEvent::Phase::DRAG) ||
        (currentInput->phase == UIEvent::Phase::ENDED))
    {
        if (currentInput->phase == UIEvent::Phase::BEGAN)
        {
            startPoint = currentInput->point;
            CalculateStartOffset(currentInput->point);
        }

        float32 newPos;
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            float32 centerOffsetX = (currentInput->point.x - startPoint.x);
            newPos = (startOffset.x + centerOffsetX) * (delegate->TotalAreaSize(this) / size.x);
        }
        else
        {
            float32 centerOffsetY = (currentInput->point.y - startPoint.y);
            newPos = (startOffset.y + centerOffsetY) * (delegate->TotalAreaSize(this) / size.y);
        }

        // Clamp.
        newPos = Min(Max(0.0f, newPos), delegate->TotalAreaSize(this) - delegate->VisibleAreaSize(this));
        delegate->OnViewPositionChanged(this, newPos);

        currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
    }
}

void UIScrollBar::CalculateStartOffset(const Vector2& inputPoint)
{
    const Rect& r = GetGeometricData().GetUnrotatedRect();
    Rect sliderRect = slider->GetRect();

    if (orientation == ORIENTATION_HORIZONTAL)
    {
        if (((inputPoint.x - r.x) >= sliderRect.x) &&
            ((inputPoint.x - r.x) <= sliderRect.x + sliderRect.dx))
        {
            // The tap happened inside the slider - start "as is".
            startOffset.x = (sliderRect.x - r.x);
        }
        else
        {
            // The tap happened outside of the slider - center the slider.
            startOffset.x = (inputPoint.x - r.x - slider->size.x / 2);
        }
    }
    else
    {
        // The same with Y.
        if (((inputPoint.y - r.y) >= sliderRect.y) &&
            ((inputPoint.y - r.y) <= sliderRect.y + sliderRect.dy))
        {
            // The tap happened inside the slider - start "as is".
            startOffset.y = (sliderRect.y - r.y);
        }
        else
        {
            // The tap happened outside of the slider - center the slider.
            startOffset.y = (inputPoint.y - r.y - slider->size.y / 2);
        }
    }

    if (startOffset.x < 0.0f)
    {
        startOffset.x = 0.0f;
    }

    if (startOffset.y < 0.0f)
    {
        startOffset.y = 0.0f;
    }
}

void UIScrollBar::Draw(const UIGeometricData& geometricData)
{
    if (delegate)
    {
        float32 visibleArea = delegate->VisibleAreaSize(this);
        float32 totalSize = delegate->TotalAreaSize(this);
        float32 viewPos = -delegate->ViewPosition(this);
        float32 diff = totalSize - visibleArea;
        diff = FLOAT_EQUAL(diff, 0.0f) ? 1.0f : diff;

        switch (orientation)
        {
        case ORIENTATION_VERTICAL:
        {
            if (resizeSliderProportionally)
            {
                slider->size.y = FLOAT_EQUAL(totalSize, 0.0f) ? 0.0f : size.y * (visibleArea / totalSize);
                slider->size.y = GetValidSliderSize(slider->size.y);
                if ((slider->size.y >= size.y) || FLOAT_EQUAL(totalSize, 0.0f))
                {
                    slider->SetVisibilityFlag(false);
                }
                else
                {
                    slider->SetVisibilityFlag(true);
                }
            }
            //TODO: optimize
            slider->relativePosition.y = (size.y - slider->size.y) * (viewPos / diff);
            if (slider->relativePosition.y < 0)
            {
                slider->size.y += slider->relativePosition.y;
                // DF-1998 - Don't allow to set size of slider less than minimum size
                slider->size.y = GetValidSliderSize(slider->size.y);
                slider->relativePosition.y = 0;
            }
            else if (slider->relativePosition.y + slider->size.y > size.y)
            {
                slider->size.y = size.y - slider->relativePosition.y;
                // DF-1998 - Don't allow to set size of slider less than minimum size
                // Also keep slider inside control's rect
                slider->size.y = GetValidSliderSize(slider->size.y);
                slider->relativePosition.y = size.y - slider->size.y;
            }
        }
        break;
        case ORIENTATION_HORIZONTAL:
        {
            if (resizeSliderProportionally)
            {
                slider->size.x = FLOAT_EQUAL(totalSize, 0.0f) ? 0.0f : size.x * (visibleArea / totalSize);
                slider->size.x = GetValidSliderSize(slider->size.x);
                if ((slider->size.x >= size.x) || FLOAT_EQUAL(totalSize, 0.0f))
                {
                    slider->SetVisibilityFlag(false);
                }
                else
                {
                    slider->SetVisibilityFlag(true);
                }
            }
            slider->relativePosition.x = (size.x - slider->size.x) * (viewPos / diff);
            if (slider->relativePosition.x < 0)
            {
                slider->size.x += slider->relativePosition.x;
                // DF-1998 - Don't allow to set size of slider less than minimum size
                slider->size.x = GetValidSliderSize(slider->size.x);
                slider->relativePosition.x = 0;
            }
            else if (slider->relativePosition.x + slider->size.x > size.x)
            {
                slider->size.x = size.x - slider->relativePosition.x;
                // DF-1998 - Don't allow to set size of slider less than minimum size
                // Also keep slider inside control's rect
                slider->size.x = GetValidSliderSize(slider->size.x);
                slider->relativePosition.x = size.x - slider->size.x;
            }
        }
        break;
        }
    }
    UIControl::Draw(geometricData);
}

int32 UIScrollBar::GetOrientation() const
{
    return orientation;
}

void UIScrollBar::SetOrientation(int32 value)
{
    orientation = static_cast<eScrollOrientation>(value);
}

float32 UIScrollBar::GetValidSliderSize(float32 size)
{
    return (size < MINIMUM_SLIDER_SIZE) ? MINIMUM_SLIDER_SIZE : size;
}
};
