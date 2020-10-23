#include "Input/InputElements.h"

namespace DAVA
{
const InputElementInfo& GetInputElementInfo(eInputElements element)
{
    // Use multiple arrays to avoid using extra space for reserved ranges
    // Orders of elements inside initializers must match enum

    static InputElementInfo infoKeyboard[eInputElements::KB_LAST - eInputElements::KB_FIRST + 1]
    {
      { "Escape", eInputElementTypes::DIGITAL },
      { "Backspace", eInputElementTypes::DIGITAL },
      { "Tab", eInputElementTypes::DIGITAL },
      { "Enter", eInputElementTypes::DIGITAL },
      { "Space", eInputElementTypes::DIGITAL },
      { "LShift", eInputElementTypes::DIGITAL },
      { "LCtrl", eInputElementTypes::DIGITAL },
      { "LAlt", eInputElementTypes::DIGITAL },
      { "LCmd", eInputElementTypes::DIGITAL },
      { "RCmd", eInputElementTypes::DIGITAL },
      { "Menu", eInputElementTypes::DIGITAL },
      { "Pause", eInputElementTypes::DIGITAL },
      { "Capslock", eInputElementTypes::DIGITAL },
      { "Numlock", eInputElementTypes::DIGITAL },
      { "Scrolllock", eInputElementTypes::DIGITAL },
      { "Pageup", eInputElementTypes::DIGITAL },
      { "Pagedown", eInputElementTypes::DIGITAL },
      { "Home", eInputElementTypes::DIGITAL },
      { "End", eInputElementTypes::DIGITAL },
      { "Insert", eInputElementTypes::DIGITAL },
      { "Del", eInputElementTypes::DIGITAL },
      { "Left", eInputElementTypes::DIGITAL },
      { "Up", eInputElementTypes::DIGITAL },
      { "Right", eInputElementTypes::DIGITAL },
      { "Down", eInputElementTypes::DIGITAL },
      { "0", eInputElementTypes::DIGITAL },
      { "1", eInputElementTypes::DIGITAL },
      { "2", eInputElementTypes::DIGITAL },
      { "3", eInputElementTypes::DIGITAL },
      { "4", eInputElementTypes::DIGITAL },
      { "5", eInputElementTypes::DIGITAL },
      { "6", eInputElementTypes::DIGITAL },
      { "7", eInputElementTypes::DIGITAL },
      { "8", eInputElementTypes::DIGITAL },
      { "9", eInputElementTypes::DIGITAL },
      { "A", eInputElementTypes::DIGITAL },
      { "B", eInputElementTypes::DIGITAL },
      { "C", eInputElementTypes::DIGITAL },
      { "D", eInputElementTypes::DIGITAL },
      { "E", eInputElementTypes::DIGITAL },
      { "F", eInputElementTypes::DIGITAL },
      { "G", eInputElementTypes::DIGITAL },
      { "H", eInputElementTypes::DIGITAL },
      { "I", eInputElementTypes::DIGITAL },
      { "J", eInputElementTypes::DIGITAL },
      { "K", eInputElementTypes::DIGITAL },
      { "L", eInputElementTypes::DIGITAL },
      { "M", eInputElementTypes::DIGITAL },
      { "N", eInputElementTypes::DIGITAL },
      { "O", eInputElementTypes::DIGITAL },
      { "P", eInputElementTypes::DIGITAL },
      { "Q", eInputElementTypes::DIGITAL },
      { "R", eInputElementTypes::DIGITAL },
      { "S", eInputElementTypes::DIGITAL },
      { "T", eInputElementTypes::DIGITAL },
      { "U", eInputElementTypes::DIGITAL },
      { "V", eInputElementTypes::DIGITAL },
      { "W", eInputElementTypes::DIGITAL },
      { "X", eInputElementTypes::DIGITAL },
      { "Y", eInputElementTypes::DIGITAL },
      { "Z", eInputElementTypes::DIGITAL },
      { "`", eInputElementTypes::DIGITAL },
      { "-", eInputElementTypes::DIGITAL },
      { "=", eInputElementTypes::DIGITAL },
      { "\\", eInputElementTypes::DIGITAL },
      { "[", eInputElementTypes::DIGITAL },
      { "]", eInputElementTypes::DIGITAL },
      { ";", eInputElementTypes::DIGITAL },
      { "'", eInputElementTypes::DIGITAL },
      { ",", eInputElementTypes::DIGITAL },
      { ".", eInputElementTypes::DIGITAL },
      { "/", eInputElementTypes::DIGITAL },
      { "0 (Numpad)", eInputElementTypes::DIGITAL },
      { "1 (Numpad)", eInputElementTypes::DIGITAL },
      { "2 (Numpad)", eInputElementTypes::DIGITAL },
      { "3 (Numpad)", eInputElementTypes::DIGITAL },
      { "4 (Numpad)", eInputElementTypes::DIGITAL },
      { "5 (Numpad)", eInputElementTypes::DIGITAL },
      { "6 (Numpad)", eInputElementTypes::DIGITAL },
      { "7 (Numpad)", eInputElementTypes::DIGITAL },
      { "8 (Numpad)", eInputElementTypes::DIGITAL },
      { "9 (Numpad)", eInputElementTypes::DIGITAL },
      { "*", eInputElementTypes::DIGITAL },
      { "/", eInputElementTypes::DIGITAL },
      { "+", eInputElementTypes::DIGITAL },
      { "-", eInputElementTypes::DIGITAL },
      { "Del (Numpad)", eInputElementTypes::DIGITAL },
      { "F1", eInputElementTypes::DIGITAL },
      { "F2", eInputElementTypes::DIGITAL },
      { "F3", eInputElementTypes::DIGITAL },
      { "F4", eInputElementTypes::DIGITAL },
      { "F5", eInputElementTypes::DIGITAL },
      { "F6", eInputElementTypes::DIGITAL },
      { "F7", eInputElementTypes::DIGITAL },
      { "F8", eInputElementTypes::DIGITAL },
      { "F9", eInputElementTypes::DIGITAL },
      { "F10", eInputElementTypes::DIGITAL },
      { "F11", eInputElementTypes::DIGITAL },
      { "F12", eInputElementTypes::DIGITAL },
      { "UNUSED", eInputElementTypes::DIGITAL },
      { "UNUSED", eInputElementTypes::DIGITAL },
      { "\\", eInputElementTypes::DIGITAL },
      { "Enter (Numpad)", eInputElementTypes::DIGITAL },
      { "PrintScreen", eInputElementTypes::DIGITAL },
      { "RShift", eInputElementTypes::DIGITAL },
      { "RCtrl", eInputElementTypes::DIGITAL },
      { "RAlt", eInputElementTypes::DIGITAL },
      { "F13", eInputElementTypes::DIGITAL },
      { "F14", eInputElementTypes::DIGITAL },
      { "F15", eInputElementTypes::DIGITAL },
      { "F16", eInputElementTypes::DIGITAL },
      { "F17", eInputElementTypes::DIGITAL },
      { "F18", eInputElementTypes::DIGITAL },
      { "F19", eInputElementTypes::DIGITAL },
      { "LWin", eInputElementTypes::DIGITAL },
      { "RWin", eInputElementTypes::DIGITAL },
      { "Camera Focus", eInputElementTypes::DIGITAL },
      { "Fn", eInputElementTypes::DIGITAL },
      { "Previous media", eInputElementTypes::DIGITAL },
      { "Next media", eInputElementTypes::DIGITAL },
      { "Play/pause media", eInputElementTypes::DIGITAL },
      { "Eject media", eInputElementTypes::DIGITAL },
      { "Volume mute", eInputElementTypes::DIGITAL },
      { "Volume up", eInputElementTypes::DIGITAL },
      { "Volume down", eInputElementTypes::DIGITAL }
    };

    static InputElementInfo infoMouse[eInputElements::MOUSE_LAST - eInputElements::MOUSE_FIRST + 1]
    {
      { "Mouse left button", eInputElementTypes::DIGITAL },
      { "Mouse right button", eInputElementTypes::DIGITAL },
      { "Mouse middle button", eInputElementTypes::DIGITAL },
      { "Mouse extended button 1", eInputElementTypes::DIGITAL },
      { "Mouse extended button 2", eInputElementTypes::DIGITAL },
      { "Mouse wheel", eInputElementTypes::ANALOG },
      { "Mouse cursor", eInputElementTypes::ANALOG }
    };

    static InputElementInfo infoGamepad[eInputElements::GAMEPAD_LAST - eInputElements::GAMEPAD_FIRST + 1]
    {
      { "Gamepad start", eInputElementTypes::DIGITAL },
      { "Gamepad A", eInputElementTypes::DIGITAL },
      { "Gamepad B", eInputElementTypes::DIGITAL },
      { "Gamepad X", eInputElementTypes::DIGITAL },
      { "Gamepad Y", eInputElementTypes::DIGITAL },
      { "Gamepad dpad left", eInputElementTypes::DIGITAL },
      { "Gamepad dpad right", eInputElementTypes::DIGITAL },
      { "Gamepad dpad up", eInputElementTypes::DIGITAL },
      { "Gamepad dpad down", eInputElementTypes::DIGITAL },
      { "Gamepad left thumbstick", eInputElementTypes::DIGITAL },
      { "Gamepad right thumbstick", eInputElementTypes::DIGITAL },
      { "Gamepad lelft shoulder", eInputElementTypes::DIGITAL },
      { "Gamepad right shoulder", eInputElementTypes::DIGITAL },
      { "Gamepad left trigger", eInputElementTypes::ANALOG },
      { "Gamepad right trigger", eInputElementTypes::ANALOG },
      { "Gamepad left thumbstick axis", eInputElementTypes::ANALOG },
      { "Gamepad right thumbstick axis", eInputElementTypes::ANALOG }

    };

    static InputElementInfo infoTouch[eInputElements::TOUCH_LAST - eInputElements::TOUCH_FIRST + 1]
    {
      { "Touch 0 click", eInputElementTypes::DIGITAL },
      { "Touch 1 click", eInputElementTypes::DIGITAL },
      { "Touch 2 click", eInputElementTypes::DIGITAL },
      { "Touch 3 click", eInputElementTypes::DIGITAL },
      { "Touch 4 click", eInputElementTypes::DIGITAL },
      { "Touch 5 click", eInputElementTypes::DIGITAL },
      { "Touch 6 click", eInputElementTypes::DIGITAL },
      { "Touch 7 click", eInputElementTypes::DIGITAL },
      { "Touch 8 click", eInputElementTypes::DIGITAL },
      { "Touch 9 click", eInputElementTypes::DIGITAL },

      { "Touch 0 position", eInputElementTypes::ANALOG },
      { "Touch 1 position", eInputElementTypes::ANALOG },
      { "Touch 2 position", eInputElementTypes::ANALOG },
      { "Touch 3 position", eInputElementTypes::ANALOG },
      { "Touch 4 position", eInputElementTypes::ANALOG },
      { "Touch 5 position", eInputElementTypes::ANALOG },
      { "Touch 6 position", eInputElementTypes::ANALOG },
      { "Touch 7 position", eInputElementTypes::ANALOG },
      { "Touch 8 position", eInputElementTypes::ANALOG },
      { "Touch 9 position", eInputElementTypes::ANALOG }
    };

    if (IsKeyboardInputElement(element))
    {
        return infoKeyboard[element - eInputElements::KB_FIRST];
    }
    else if (IsMouseInputElement(element))
    {
        return infoMouse[element - eInputElements::MOUSE_FIRST];
    }
    else if (IsGamepadInputElement(element))
    {
        return infoGamepad[element - eInputElements::GAMEPAD_FIRST];
    }
    else if (IsTouchInputElement(element))
    {
        return infoTouch[element - eInputElements::TOUCH_FIRST];
    }
    else
    {
        DVASSERT(false);
        static InputElementInfo emptyInfo;
        return emptyInfo;
    }
}
}
