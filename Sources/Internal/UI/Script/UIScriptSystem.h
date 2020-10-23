#pragma once

#include "Base/FastName.h"
#include "Base/Vector.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIScriptComponent;
class UIScriptComponentController;

/**
    Manage all UIScriptComponent internals. Creates, handles and destroys component controllers. 
*/
class UIScriptSystem : public UISystem
{
public:
    ~UIScriptSystem() override;

    /** Processing mode for editor */
    void SetPauseProcessing(bool value);
    bool IsPauseProcessing() const;

    /** Send message to controller of this control. */
    bool ProcessEvent(UIControl* control, const FastName& event, const Any& data);

    /** Process system*/
    void Process(float32 elapsedTime) override;

    /** Get internal controller instance. */
    UIScriptComponentController* GetController(UIScriptComponent* component) const;

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

private:
    struct ScriptLink
    {
        UIScriptComponent* component = nullptr;
        std::shared_ptr<UIScriptComponentController> controller;
    };

    void AddScriptLink(UIScriptComponent* component);

    /** Update controller instance. */
    void UpdateController(DAVA::UIScriptSystem::ScriptLink& l);
    void RemoveScriptLink(UIScriptComponent* component);

    Vector<ScriptLink> links;
    bool pauseProcessing = false;
};

inline bool UIScriptSystem::IsPauseProcessing() const
{
    return pauseProcessing;
}
}
