#include "UI/UISlider.h"
#include "Base/ObjectFactory.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/RenderHelper.h"
#include "UI/UIControlBackground.h"
#include "UI/UIControlSystem.h"
#include "UI/UIEvent.h"
#include "Utils/Utils.h"

namespace DAVA
{
// Use these names for children buttons to define UISlider in .yaml
const FastName UISlider::THUMB_SPRITE_CONTROL_NAME("thumbSpriteControl");
const FastName UISlider::MIN_SPRITE_CONTROL_NAME("minSpriteControl");
const FastName UISlider::MAX_SPRITE_CONTROL_NAME("maxSpriteControl");

DAVA_VIRTUAL_REFLECTION_IMPL(UISlider)
{
    ReflectionRegistrator<UISlider>::Begin()[M::DisplayName("Slider")]
    .ConstructorByPointer()
    .DestructorByPointer([](UISlider* o) { o->Release(); })
    .Field("minValue", &UISlider::GetMinValue, &UISlider::SetMinValue)[M::DisplayName("Min")]
    .Field("maxValue", &UISlider::GetMaxValue, &UISlider::SetMaxValue)[M::DisplayName("Max")]
    .Field("value", &UISlider::GetValue, &UISlider::SetValue)[M::DisplayName("Value")]
    .End();
}

UISlider::UISlider(const Rect& rect)
    : UIControl(rect)
    , thumbButton(NULL)
{
    SetInputEnabled(true, false);
    isEventsContinuos = true;

    leftInactivePart = 0;
    rightInactivePart = 0;
    minValue = 0.0f;
    maxValue = 1.0f;
    currentValue = 0.5f;

    InitThumb();
}

void UISlider::InitThumb()
{
    thumbButton = new UIControl(Rect(0, 0, 40.f, 40.f));
    thumbButton->SetName(UISlider::THUMB_SPRITE_CONTROL_NAME);
    thumbButton->GetOrCreateComponent<UIControlBackground>();
    AddControl(thumbButton);

    thumbButton->SetInputEnabled(false);
    thumbButton->relativePosition.y = size.y * 0.5f;
    thumbButton->SetPivot(Vector2(0.5f, 0.5f));

    SetValue(currentValue);
}

void UISlider::InitInactiveParts(UIControl* thumb)
{
    UIControlBackground* bg = thumb->GetComponent<UIControlBackground>();
    if (bg == nullptr || bg->GetSprite() == nullptr)
    {
        return;
    }

    leftInactivePart = rightInactivePart = static_cast<int32>((bg->GetSprite()->GetWidth() / 2.0f));
}

void UISlider::SetThumb(UIControl* newThumb)
{
    if (thumbButton == newThumb)
    {
        return;
    }

    RemoveControl(thumbButton);
    SafeRelease(thumbButton);

    thumbButton = SafeRetain(newThumb);
    thumbButton->SetName(UISlider::THUMB_SPRITE_CONTROL_NAME);
    thumbButton->SetInputEnabled(false);

    thumbButton->relativePosition.y = size.y * 0.5f;
    thumbButton->SetPivot(Vector2(0.5f, 0.5f));

    UIControl::AddControl(thumbButton);

    SetValue(currentValue);
}

UISlider::~UISlider()
{
    SafeRelease(thumbButton);
}

void UISlider::RecalcButtonPos()
{
    if (thumbButton)
    {
        thumbButton->relativePosition.x = Interpolation::Linear(static_cast<float32>(leftInactivePart), size.x - rightInactivePart, minValue, currentValue, maxValue);
        thumbButton->relativePosition.y = GetSize().y / 2; // thumb button pivot point is on center.
    }
}

void UISlider::SyncThumbWithSprite()
{
    RecalcButtonPos();
}

void UISlider::SetValue(float32 value)
{
    bool needSendEvent = !FLOAT_EQUAL(currentValue, value);
    currentValue = value;
    RecalcButtonPos();

    if (needSendEvent)
    {
        PerformEvent(EVENT_VALUE_CHANGED, nullptr);
    }
}

void UISlider::SetMinValue(float32 value)
{
    minValue = value;
    if (currentValue < minValue)
    {
        SetValue(minValue);
    }
    else
    {
        RecalcButtonPos();
    }
}

void UISlider::SetMaxValue(float32 value)
{
    maxValue = value;
    if (currentValue > maxValue)
    {
        SetValue(maxValue);
    }
    else
    {
        RecalcButtonPos();
    }
}

void UISlider::SetMinMaxValue(float32 _minValue, float32 _maxValue)
{
    minValue = _minValue;
    maxValue = _maxValue;

    if (currentValue < minValue)
    {
        SetValue(minValue);
    }
    else if (currentValue > maxValue)
    {
        SetValue(maxValue);
    }
    else
    {
        RecalcButtonPos();
    }
}

void UISlider::AddControl(UIControl* control)
{
    // Synchronize the pointers to the thumb each time new control is added.
    UIControl::AddControl(control);

    if (control->GetName() == UISlider::THUMB_SPRITE_CONTROL_NAME && thumbButton != control)
    {
        SafeRelease(thumbButton);
        thumbButton = SafeRetain(control);
    }
}

void UISlider::RemoveControl(UIControl* control)
{
    if (control == thumbButton)
    {
        SafeRelease(thumbButton);
    }

    UIControl::RemoveControl(control);
}

void UISlider::Input(UIEvent* currentInput)
{
    // not supported for now.
    if (UIEvent::Phase::WHEEL == currentInput->phase || UIEvent::Phase::MOVE == currentInput->phase || UIEvent::Phase::CHAR == currentInput->phase || UIEvent::Phase::KEY_DOWN == currentInput->phase || UIEvent::Phase::KEY_UP == currentInput->phase || UIEvent::Phase::KEY_DOWN_REPEAT == currentInput->phase || UIEvent::Phase::CHAR_REPEAT == currentInput->phase || UIEvent::Phase::ERROR == currentInput->phase || UIEvent::Phase::JOYSTICK == currentInput->phase)
    {
        return;
    }

    const Rect& absRect = GetGeometricData().GetUnrotatedRect();
    //absTouchPoint = currentInput->point;

    relTouchPoint = currentInput->point;
    relTouchPoint -= absRect.GetPosition();

    float oldVal = currentValue;
    currentValue = Interpolation::Linear(minValue, maxValue, static_cast<float32>(leftInactivePart), relTouchPoint.x, size.x - static_cast<float32>(rightInactivePart));

    if (currentValue < minValue)
    {
        currentValue = minValue;
    }
    if (currentValue > maxValue)
    {
        currentValue = maxValue;
    }

    if (isEventsContinuos) // if continuos events
    {
        if (oldVal != currentValue)
        {
            PerformEventWithData(EVENT_VALUE_CHANGED, currentInput, currentInput);
        }
    }
    else if (currentInput->phase == UIEvent::Phase::ENDED)
    {
        /* if not continuos always perform event because last move position almost always the same as end pos */
        PerformEventWithData(EVENT_VALUE_CHANGED, currentInput, currentInput);
    }

    RecalcButtonPos();
    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
}

void UISlider::SetSize(const DAVA::Vector2& newSize)
{
    UIControl::SetSize(newSize);
    RecalcButtonPos();
}

void UISlider::LoadFromYamlNodeCompleted()
{
    AttachToSubcontrols();
    SyncThumbWithSprite();
}

UISlider* UISlider::Clone()
{
    UISlider* t = new UISlider(GetRect());
    t->CopyDataFrom(this);
    return t;
}

void UISlider::CopyDataFrom(UIControl* srcControl)
{
    RemoveControl(thumbButton);
    SafeRelease(thumbButton);

    UIControl::CopyDataFrom(srcControl);
    UISlider* t = static_cast<UISlider*>(srcControl);

    isEventsContinuos = t->isEventsContinuos;

    leftInactivePart = t->leftInactivePart;
    rightInactivePart = t->rightInactivePart;

    minValue = t->minValue;
    maxValue = t->maxValue;

    currentValue = t->currentValue;

    relTouchPoint = t->relTouchPoint;
}

void UISlider::AttachToSubcontrols()
{
    if (!thumbButton)
    {
        thumbButton = FindByName(UISlider::THUMB_SPRITE_CONTROL_NAME);
        DVASSERT(thumbButton);
        thumbButton->Retain();
    }

    InitInactiveParts(thumbButton);
}

} // ns
