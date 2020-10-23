#pragma once

#include "DAVAEngine.h"
#include "Base/BaseTypes.h"
#include "Infrastructure/BaseScreen.h"
#include "Input/InputEvent.h"
#include "UI/Joypad/UIJoypadComponent.h"

using namespace DAVA;

class UIJoypadSystemTest : public BaseScreen
{
public:
    UIJoypadSystemTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    enum eStickElements
    {
        STICK_AREA = 0,
        STICK_ARM,
        STICK_ARROW
    };

    void OnUpdate(float32 timeElapsed);

    void ToggleVisibility(UIControl* c);
    void ToggleDynamic(UIControl* c);
    void SetRadius(UIControl*);

    void ToggleStickElement(eStickElements element, UIControl* c);

    bool OnInputEvent(const InputEvent& event);

private:
    RefPtr<UIJoypadComponent> joypad;
    RefPtr<UIControl> cancelZone;
    RefPtr<UIControl> coords;
    RefPtr<UITextField> radius;
    std::pair<RefPtr<Sprite>, RefPtr<Sprite>> areaSprites;
    std::pair<RefPtr<Sprite>, RefPtr<Sprite>> armSprites;
    RefPtr<Sprite> arrowSprite;

    UnorderedMap<eStickElements, UIControl*> stickElements;

    bool wasActive = true;

    uint32 inputHandler = 0;
};