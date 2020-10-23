#pragma once

#include "Base/BaseTypes.h"
#include "Input/KeyboardShortcut.h"
#include "UI/Events/UIActionMap.h"
#include "UI/Events/UIInputMap.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIControl;
class UIBehaviorSystem;
class UIEvent;

/**
*/
class UIEventsSystem : public UISystem
{
public:
    const static DAVA::FastName ACTION_COMPONENT_SELF_ENTITY_NAME;

    UIEventsSystem();
    ~UIEventsSystem() override;

    void RegisterSystem() override;
    void UnregisterSystem() override;

    void Process(float32 elapsedTime) override;

    /** Register internal commands */
    void RegisterCommands();

    /** Get control by relative path */
    UIControl* GetTargetEntity(const FastName& name, UIControl* control);

    /** Bind global shortcut to global event */
    void BindGlobalShortcut(const KeyboardShortcut& shortcut, const FastName& eventName);

    /** Bind global event to global handler */
    void BindGlobalAction(const FastName& eventName, const UIActionMap::SimpleAction& action);

    /** Find global shortcut event and execute associated global event handler. */
    void PerformGlobalShortcut(const KeyboardShortcut& shortcut);

    /** Process all control events for UIInputEventComponent */
    void ProcessControlEvent(int32 eventType, const UIEvent* uiEvent, UIControl* control);

private:
    /** Send event from child to root controls. */
    bool SendEvent(UIControl* control, const FastName& event, const Any& data);

    /** Broadcast event to all children controls. */
    bool SendBroadcastEvent(UIControl* control, const FastName& event, const Any& data);

    /** Process event internal*/
    bool ProcessEventOnContol(UIControl* sceneControl, const FastName& event, const Any& data, bool checkFlow);

    /** Process eventsfor UIEventBindingComponent */
    bool ProcessEventOnBinding(UIControl* sceneControl, const FastName& event, const Any& data);

    // bool ProcessEventOnController(UIControl* control, const FastName& event);

    UIActionMap globalActions;
    UIInputMap globalShortcutToEvent;
};
}
