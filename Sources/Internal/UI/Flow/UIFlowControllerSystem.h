#pragma once

#include "Base/Vector.h"
#include "UI/UISystem.h"

namespace DAVA
{
class LuaScript;
class UIFlowContext;
class UIFlowControllerComponent;
class UIFlowController;

/**
    Manage all UIFlowControllerComponents and their activated controlles.
*/
class UIFlowControllerSystem final : public UISystem
{
public:
    /** Destructor. */
    ~UIFlowControllerSystem() override;

    /** Return pointer to stored UIFlowController by specified poiter to UIFlowControllerComponent. */
    UIFlowController* GetController(UIFlowControllerComponent* component) const;

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

private:
    /** Struct that describes a link between instances of UIFlowControllerComponent and UIFlowController. */
    struct ControllerLink
    {
        UIFlowControllerComponent* component = nullptr;
        std::shared_ptr<UIFlowController> controller;
    };

    /** List of links between UIFlowControllerComponent and UIFlowController. */
    Vector<ControllerLink> links;

    /** Add new link with specified UIFlowControllerComponent to system. */
    void AddControllerLink(UIFlowControllerComponent* component);
    /** Remove link with specified UIFlowControllerComponent from system. */
    void RemoveControllerLink(UIFlowControllerComponent* component);

    /** Create instance of UIFlowController from information in specified
        UIFlowControllerComponent and calls Init method of created controller.
        Return pointer to created controller. */
    UIFlowController* InitController(UIFlowControllerComponent* component, UIFlowContext* context);
    /** Calls Release method of controller that linked with specified
        UIFlowControllerComponent and destroy the controller. */
    void ReleaseController(UIFlowControllerComponent* component, UIFlowContext* context);
    /** Calls LoadResources method of controller that linked with specified
        UIFlowControllerComponent. It method can be executed not in main thread. */
    void LoadController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view);
    /** Calls UnloadResources method of controller that linked with specified
        UIFlowControllerComponent. It method can be executed not in main thread. */
    void UnloadController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view);
    /** Calls Activate method of controller that linked with specified
        UIFlowControllerComponent. Return pointer to activated controller. */
    UIFlowController* ActivateController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view);
    /** Calls Activate method of controller that linked with specified
        UIFlowControllerComponent. */
    void DeactivateController(UIFlowControllerComponent* component, UIFlowContext* context, UIControl* view);

    friend class UIFlowStateSystem;
};
}