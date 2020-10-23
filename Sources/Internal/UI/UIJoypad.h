#ifndef __DAVAENGINE_UI_JOYPAD__
#define __DAVAENGINE_UI_JOYPAD__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
/**
     \ingroup controlsystem
     \brief Joypad realisation for the touch screen supported platforms.
        Incomplete!!!.
     */

class UIJoypad : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIJoypad, UIControl);

    enum eTouchID
    {
        TOUCH_INVALID_ID = -1
    };

public:
    UIJoypad(const Rect& rect = Rect());

protected:
    virtual ~UIJoypad();

public:
    UIJoypad* Clone() override;
    void CopyDataFrom(DAVA::UIControl* srcControl) override;

    void AddControl(UIControl* control) override;
    void RemoveControl(UIControl* control) override;

    void Input(UIEvent* currentInput) override; // Can be overrided for control additioanl functionality implementation
    void InputCancelled(UIEvent* currentInput) override; // Can be overrided for control additioanl functionality implementation

    const Vector2& GetDigitalPosition();
    const Vector2& GetAnalogPosition();

    Sprite* GetStickSprite() const;
    int32 GetStickSpriteFrame() const;

    void SetStickSprite(Sprite* stickSprite, int32 frame);
    void SetStickSprite(const FilePath& stickSpriteName, int32 frame);
    void SetStickSpriteFrame(int32 frame);

    float32 GetDeadAreaSize() const;
    void SetDeadAreaSize(float32 newDeadAreaSize); //!< Size of the middle joypad area where the tuches do not come.

    float32 GetDigitalSense() const;
    void SetDigitalSense(float32 newDigitalSense); //!< Sense of the diagonal joypad ways. 0.5 by default.

    float32 GetStickAngle() const;

protected:
    void RecalcDigitalPosition();
    void RecalcAnalogPosition();

    RefPtr<UIControl> stick;

private:
    int32 mainTouch;
    float deadAreaSize; // dead area size in pixels (must be positive value)
    float32 digitalSense;
    bool needRecalcDigital;
    bool needRecalcAnalog;
    Vector2 currentPos;

    Vector2 digitalVector;
    Vector2 analogVector;
};
};

#endif
