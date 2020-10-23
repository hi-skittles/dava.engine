#include "UI/Events/UIShortcutEventComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Utils/StringUtils.h"
#include "Utils/Utils.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include <UI/UIControlHelpers.h>

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIShortcutEventComponent)
{
    ReflectionRegistrator<UIShortcutEventComponent>::Begin()[M::DisplayName("Shortcut Event"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIShortcutEventComponent* o) { o->Release(); })
    .Field("shortcuts", &UIShortcutEventComponent::GetShortcutsAsString, &UIShortcutEventComponent::SetShortcutsFromString)[M::DisplayName("Shortcuts")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIShortcutEventComponent);

UIShortcutEventComponent::UIShortcutEventComponent()
{
}

UIShortcutEventComponent::UIShortcutEventComponent(const UIShortcutEventComponent& src)
{
    // TODO
    SetShortcutsFromString(src.GetShortcutsAsString());
}

UIShortcutEventComponent::~UIShortcutEventComponent()
{
}

UIShortcutEventComponent* UIShortcutEventComponent::Clone() const
{
    return new UIShortcutEventComponent(*this);
}

UIInputMap& UIShortcutEventComponent::GetInputMap()
{
    return inputMap;
}

void UIShortcutEventComponent::BindShortcut(const KeyboardShortcut& shortcut, const FastName& eventName)
{
    eventShortcuts.push_back(KeyBinding(eventName, shortcut));
    inputMap.BindEvent(shortcut, eventName);
}

void UIShortcutEventComponent::UnbindShortcut(const KeyboardShortcut& shortcut)
{
    auto it = find_if(eventShortcuts.begin(), eventShortcuts.end(), [shortcut](const KeyBinding& a) { return a.shortcut1 == shortcut; });
    if (it != eventShortcuts.end())
    {
        eventShortcuts.erase(it);
    }
    inputMap.RemoveShortcut(shortcut);
}

void UIShortcutEventComponent::UnbindEvent(const FastName& eventName)
{
    auto it = find_if(eventShortcuts.begin(), eventShortcuts.end(), [eventName](const KeyBinding& a) { return a.eventName == eventName; });
    if (it != eventShortcuts.end())
    {
        eventShortcuts.erase(it);
    }
    inputMap.RemoveEvent(eventName);
}

String UIShortcutEventComponent::GetShortcutsAsString() const
{
    StringStream stream;
    bool first = true;
    for (const KeyBinding& action : eventShortcuts)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            stream << "; ";
        }

        if (action.shortcut1.GetKey() != eInputElements::NONE)
        {
            stream << action.eventName.c_str() << ", " << action.shortcut1.ToString();
        }
        else
        {
            stream << action.eventName.c_str();
        }
    }
    return stream.str();
}

void UIShortcutEventComponent::SetShortcutsFromString(const String& value)
{
    eventShortcuts.clear();
    inputMap.Clear();
    Vector<String> actionsStr;
    Split(value, ";", actionsStr);
    for (const String& actionStr : actionsStr)
    {
        Vector<String> str;
        Split(actionStr, ",", str);
        if (str.size() > 0 && !str[0].empty())
        {
            FastName eventName(StringUtils::Trim(str[0]));

            if (UIControlHelpers::IsEventNameValid(eventName))
            {
                KeyboardShortcut shortcut;
                if (str.size() > 1)
                {
                    shortcut = KeyboardShortcut::ParseFromString(StringUtils::Trim(str[1]));
                }

                eventShortcuts.push_back(KeyBinding(eventName, shortcut));
                if (shortcut.GetKey() != eInputElements::NONE)
                {
                    inputMap.BindEvent(shortcut, eventName);
                }
            }
        }
    }
}
}
