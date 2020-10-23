#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Input/KeyboardShortcut.h"
#include "Reflection/Reflection.h"
#include "UI/Events/UIInputMap.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIControl;

/** Keyboard shortcuts component.
    Map keyboard shortcuts to events.
*/
class UIShortcutEventComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIShortcutEventComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIShortcutEventComponent);

public:
    UIShortcutEventComponent();
    UIShortcutEventComponent(const UIShortcutEventComponent& src);

protected:
    virtual ~UIShortcutEventComponent();

private:
    UIShortcutEventComponent& operator=(const UIShortcutEventComponent&) = delete;

public:
    UIShortcutEventComponent* Clone() const override;

    /** Bind keyboard shortcut to event name. */
    void BindShortcut(const KeyboardShortcut& shortcut, const FastName& eventName);
    /** Unbind shortcut. */
    void UnbindShortcut(const KeyboardShortcut& shortcut);
    /** Unbind shortcuts by event.*/
    void UnbindEvent(const FastName& eventName);

    UIInputMap& GetInputMap();

private:
    String GetShortcutsAsString() const;
    void SetShortcutsFromString(const String& value);

    struct KeyBinding
    {
        FastName eventName;
        KeyboardShortcut shortcut1;

        KeyBinding(const FastName& eventName_, const KeyboardShortcut& shortcut_)
            : eventName(eventName_)
            , shortcut1(shortcut_)
        {
        }
    };

    Vector<KeyBinding> eventShortcuts;
    UIInputMap inputMap;
};
}
