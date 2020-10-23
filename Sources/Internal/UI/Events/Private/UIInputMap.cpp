#include "UI/Events/UIInputMap.h"

namespace DAVA
{
void UIInputMap::BindEvent(const KeyboardShortcut& shortcut, const FastName& event)
{
    inputMap[shortcut] = event;
}

FastName UIInputMap::FindEvent(const KeyboardShortcut& shortcut) const
{
    auto it = inputMap.find(shortcut);
    if (it != inputMap.end())
    {
        return it->second;
    }

    return FastName();
}

void UIInputMap::RemoveShortcut(const KeyboardShortcut& shortcut)
{
    auto it = inputMap.find(shortcut);
    if (it != inputMap.end())
    {
        inputMap.erase(it);
    }
}

void UIInputMap::RemoveEvent(const FastName& event)
{
    auto it = inputMap.begin();
    while (it != inputMap.end())
    {
        if (it->second == event)
        {
            it = inputMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void UIInputMap::Clear()
{
    inputMap.clear();
}
}
