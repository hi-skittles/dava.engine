#pragma once

#include "Math/Vector.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIJoypadComponent;

class UIJoypadSystem : public UISystem
{
public:
    UIJoypadSystem() = default;
    ~UIJoypadSystem() override = default;

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void OnControlVisible(UIControl* control) override;
    void OnControlInvisible(UIControl* control) override;
    void Process(float32 elapsedTime) override;

private:
    void AddComponent(UIJoypadComponent* ptr);
    void RemoveComponent(UIJoypadComponent* ptr);

    void UpdateComponents();

private:
    struct ComponentState
    {
        Vector2 initialTouch = { -1.f, -1.f };
        Vector2 currentTouch = { -1.f, -1.f };
        bool isCanceled = false;
    };

    UnorderedMap<UIJoypadComponent*, ComponentState> componentsStates;
    UnorderedSet<UIJoypadComponent*> needUpdate;
};
}