#pragma once

#include "Base/BaseTypes.h"
#include "Engine/EngineTypes.h"

#include "Input/InputElements.h"

namespace DAVA
{
class KeyboardShortcut final
{
public:
    KeyboardShortcut();
    KeyboardShortcut(const KeyboardShortcut& shortcut);
    KeyboardShortcut(eInputElements key_, eModifierKeys modifiers_ = eModifierKeys::NONE);

    ~KeyboardShortcut();

    KeyboardShortcut& operator=(const KeyboardShortcut& shortcut);
    bool operator==(const KeyboardShortcut& other) const;
    bool operator!=(const KeyboardShortcut& other) const;

    eInputElements GetKey() const;
    eModifierKeys GetModifiers() const;
    String ToString() const;

    static KeyboardShortcut ParseFromString(const String& str);

private:
    eInputElements key = eInputElements::NONE;
    eModifierKeys modifiers = eModifierKeys::NONE;
};
}

namespace std
{
template <>
struct hash<DAVA::KeyboardShortcut>
{
    size_t operator()(const DAVA::KeyboardShortcut& shortcut) const
    {
        return static_cast<size_t>(shortcut.GetKey()) | (static_cast<size_t>(shortcut.GetModifiers()) << 16);
    }
};
}
