#include "UI/UIJoypad.h"
#include "UI/UIControlBackground.h"
#include "UI/UIEvent.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
static const FastName UIJOYPAD_STICK_NAME("stick");

DAVA_VIRTUAL_REFLECTION_IMPL(UIJoypad)
{
    ReflectionRegistrator<UIJoypad>::Begin()[M::DisplayName("Joypad")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIJoypad* o) { o->Release(); })
    .Field("deadAreaSize", &UIJoypad::GetDeadAreaSize, &UIJoypad::SetDeadAreaSize)[M::DisplayName("Dead Area Size")]
    .Field("digitalSense", &UIJoypad::GetDigitalSense, &UIJoypad::SetDigitalSense)[M::DisplayName("Digital Sense")]
    .End();
}

UIJoypad::UIJoypad(const Rect& rect)
    : UIControl(rect)
    , stick(nullptr)
    , mainTouch(TOUCH_INVALID_ID)
    , deadAreaSize(10.0f)
    , digitalSense(0.5f)
    , needRecalcDigital(true)
    , needRecalcAnalog(true)
    , currentPos(Vector2(0, 0))
{
    SetInputEnabled(true);

    RefPtr<UIControl> stickCtrl(new UIControl(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
    stickCtrl->SetName(UIJOYPAD_STICK_NAME);
    UIControlBackground* stickCtrlBg = stickCtrl->GetOrCreateComponent<UIControlBackground>();
    stickCtrlBg->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    stickCtrl->SetInputEnabled(false);
    stickCtrl->SetPivot(Vector2(0.5f, 0.5f));
    stickCtrl->SetPosition(GetSize() / 2.0f);
    AddControl(stickCtrl.Get());
}

UIJoypad::~UIJoypad()
{
}

UIJoypad* UIJoypad::Clone()
{
    UIJoypad* control = new UIJoypad();
    control->CopyDataFrom(this);
    return control;
}

void UIJoypad::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UIJoypad* src = DynamicTypeCheck<UIJoypad*>(srcControl);

    mainTouch = TOUCH_INVALID_ID;
    deadAreaSize = src->deadAreaSize;
    digitalSense = src->digitalSense;
    needRecalcDigital = true;
    needRecalcAnalog = true;
    currentPos = Vector2();
}

void UIJoypad::AddControl(UIControl* control)
{
    UIControl::AddControl(control);
    if (control->GetName() == UIJOYPAD_STICK_NAME && stick.Get() != control)
    {
        stick = control;
    }
}

void UIJoypad::RemoveControl(UIControl* control)
{
    if (control == stick.Get())
    {
        stick = nullptr;
    }

    UIControl::RemoveControl(control);
}

const Vector2& UIJoypad::GetDigitalPosition()
{
    if (currentPos.x == 0.f && currentPos.y == 0.f)
    {
        digitalVector.x = 0.0f;
        digitalVector.y = 0.0f;
        return digitalVector;
    }
    if (needRecalcAnalog)
    {
        RecalcAnalogPosition();
    }

    Vector2 v = analogVector;
    if (std::abs(v.x) > std::abs(v.y))
    {
        float32 f = std::abs(1.f / v.x);
        v.y *= f;
        v.x *= f;
    }
    else
    {
        float32 f = std::abs(1.f / v.y);
        v.x *= f;
        v.y *= f;
    }

    //Logger::Info("V pos x = %f, y = %f", v.x, v.y);

    float32 xSign = v.x >= 0.0f ? digitalSense : -digitalSense;
    float32 ySign = v.y >= 0.0f ? digitalSense : -digitalSense;

    digitalVector.x = v.x + xSign;
    digitalVector.y = v.y + ySign;

    //Logger::Info("Digital joy pos x = %f, y = %f", digitalVector.x, digitalVector.y);

    return digitalVector;
}
const Vector2& UIJoypad::GetAnalogPosition()
{
    if (needRecalcAnalog)
    {
        RecalcAnalogPosition();
    }
    return analogVector;
}

float32 UIJoypad::GetStickAngle() const
{
    const Vector2& v = currentPos;

    const float32 len = std::sqrt(v.x * v.x + v.y * v.y);
    float32 ang = std::asin(v.x / len);

    if (v.y > 0)
    {
        ang = PI - ang;
    }

    if (ang < 0)
    {
        ang += PI * 2;
    }

    if (ang > PI * 2)
    {
        ang -= PI * 2;
    }

    return ang;
}

void UIJoypad::RecalcDigitalPosition()
{
    needRecalcDigital = false;
    if (!currentPos.x && !currentPos.y)
    {
        digitalVector.x = 0;
        digitalVector.y = 0;
        return;
    }

    float ang = GetStickAngle();

    if (ang > PI / 8 && ang < PI - PI / 8)
    {
        digitalVector.x = 1.0f;
    }
    else if (ang < PI * 2 - PI / 8 && ang > PI + PI / 8)
    {
        digitalVector.x = -1.0f;
    }
    else
    {
        digitalVector.x = 0;
    }

    if (ang < PI / 2 - PI / 8 || ang > PI * 2 - PI / 2 + PI / 8)
    {
        digitalVector.y = -1.0f;
    }
    else if (ang < PI * 2 - PI / 2 - PI / 8 && ang > PI / 2 + PI / 8)
    {
        digitalVector.y = 1.0f;
    }
    else
    {
        digitalVector.y = 0;
    }
    //	Logger::Info("x = %f, y = %f", digitalVector.x, digitalVector.y);
}

void UIJoypad::RecalcAnalogPosition()
{
    needRecalcAnalog = false;
    analogVector.x = currentPos.x / (size.x / 2);
    analogVector.y = currentPos.y / (size.y / 2);
    //Logger::Info("Analog joy pos x = %f, y = %f", analogVector.x, analogVector.y);
}

Sprite* UIJoypad::GetStickSprite() const
{
    return stick ? stick->GetComponent<UIControlBackground>()->GetSprite() : NULL;
}

int32 UIJoypad::GetStickSpriteFrame() const
{
    if (stick && stick->GetComponent<UIControlBackground>()->GetSprite())
    {
        return stick->GetComponent<UIControlBackground>()->GetFrame();
    }

    return 0;
}

void UIJoypad::SetStickSprite(Sprite* stickSprite, int32 frame)
{
    DVASSERT(stick.Valid());
    if (!stick.Valid())
        return;

    stick->GetComponent<UIControlBackground>()->SetSprite(stickSprite, frame);
}

void UIJoypad::SetStickSprite(const FilePath& stickSpriteName, int32 frame)
{
    DVASSERT(stick.Valid());
    if (!stick.Valid())
        return;

    stick->GetComponent<UIControlBackground>()->SetSprite(stickSpriteName, frame);
}

void UIJoypad::SetStickSpriteFrame(int32 frame)
{
    DVASSERT(stick.Valid());

    if (stick.Valid() && stick->GetComponent<UIControlBackground>()->GetSprite())
    {
        stick->GetComponent<UIControlBackground>()->SetFrame(frame);
    }
}

void UIJoypad::Input(UIEvent* currentInput)
{
    if ((TOUCH_INVALID_ID == mainTouch) && currentInput->phase == UIEvent::Phase::BEGAN)
    {
        mainTouch = currentInput->touchId;
    }

    if (mainTouch != currentInput->touchId)
    {
        return;
    }

    if (currentInput->phase == UIEvent::Phase::ENDED)
    {
        currentPos.x = 0;
        currentPos.y = 0;
        mainTouch = TOUCH_INVALID_ID;
    }
    else
    {
        Rect r = GetGeometricData().GetUnrotatedRect();
        currentPos = currentInput->point - r.GetPosition();

        currentPos -= Vector2(r.dx * 0.5f, r.dy * 0.5f);

        if (currentPos.x < deadAreaSize && currentPos.x > -deadAreaSize && currentPos.y < deadAreaSize && currentPos.y > -deadAreaSize)
        {
            currentPos.x = 0;
            currentPos.y = 0;
        }
        currentPos.x = Max(currentPos.x, -size.x / 2);
        currentPos.x = Min(currentPos.x, size.x / 2);
        currentPos.y = Max(currentPos.y, -size.y / 2);
        currentPos.y = Min(currentPos.y, size.y / 2);
    }

    if (stick.Valid())
    {
        stick->SetPosition(GetSize() / 2.0f + currentPos);
    }

    needRecalcAnalog = true;
    needRecalcDigital = true;
    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
}

void UIJoypad::InputCancelled(UIEvent* currentInput)
{
    if (currentInput->touchId == mainTouch)
    {
        mainTouch = TOUCH_INVALID_ID;

        currentPos.x = 0;
        currentPos.y = 0;

        if (stick.Valid())
        {
            stick->SetPosition(GetSize() / 2.0f + currentPos);
        }

        needRecalcAnalog = true;
        needRecalcDigital = true;
    }
}

float32 UIJoypad::GetDeadAreaSize() const
{
    return deadAreaSize;
}

void UIJoypad::SetDeadAreaSize(float32 newDeadAreaSize)
{
    deadAreaSize = newDeadAreaSize;
}

float32 UIJoypad::GetDigitalSense() const
{
    return digitalSense;
}

void UIJoypad::SetDigitalSense(float32 newDigitalSense)
{
    digitalSense = newDigitalSense;
}
};
