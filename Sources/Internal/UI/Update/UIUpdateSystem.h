#pragma once

#include "Base/BaseTypes.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIControl;
class UIComponent;
class UIUpdateComponent;
class UICustomUpdateDeltaComponent;

/**
Send update events for all visible UIControl with UIUpdateCompoent component.
Temporary system for backward compatibility with existing code.
**WILL BE REMOVED** after refactoring all `UIControl::Update` logic.
*/
class UIUpdateSystem : public UISystem
{
public:
    ~UIUpdateSystem() override;

private:
    struct UpdateBind
    {
        UpdateBind(UIUpdateComponent* uc, UICustomUpdateDeltaComponent* cdc);
        const UIUpdateComponent* updateComponent = nullptr;
        UICustomUpdateDeltaComponent* customDeltaComponent = nullptr;
        bool updated = false;
    };

    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void OnControlVisible(UIControl* control) override;
    void OnControlInvisible(UIControl* control) override;

    void Process(float32 elapsedTime) override;
    void ForceProcessControl(float32 elapsedTime, UIControl* control) override;

    void ProcessControlHierarchy(float32 elapsedTime, UIControl* control);

    List<UpdateBind> binds;
    bool modified = false;
};
}
