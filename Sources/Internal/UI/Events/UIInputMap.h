#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Input/KeyboardShortcut.h"

namespace DAVA
{
class UIInputMap final
{
public:
    UIInputMap() = default;
    ~UIInputMap() = default;

    void BindEvent(const KeyboardShortcut& shortcut, const FastName& event);
    FastName FindEvent(const KeyboardShortcut& shortcut) const;

    void RemoveShortcut(const KeyboardShortcut& shortcut);
    void RemoveEvent(const FastName& event);
    void Clear();

private:
    UnorderedMap<KeyboardShortcut, FastName> inputMap;
};
}
