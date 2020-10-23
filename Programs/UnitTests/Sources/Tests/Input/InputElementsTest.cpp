#include "UnitTests/UnitTests.h"

#include <Input/InputElements.h>

using namespace DAVA;

DAVA_TESTCLASS (InputElementsTestClass)
{
    DAVA_TEST (InputElementsMetaDataTest)
    {
        // Check input element info:
        //    - name is not empty
        //    - type is correct
        //    - Is*InputElement methods return correct values

        // Keyboard

        for (uint32 i = static_cast<uint32>(eInputElements::KB_FIRST); i <= static_cast<uint32>(eInputElements::KB_LAST); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::DIGITAL);
            TEST_VERIFY(IsKeyboardInputElement(element));
            TEST_VERIFY(!IsMouseInputElement(element));
            TEST_VERIFY(!IsMouseButtonInputElement(element));
            TEST_VERIFY(!IsGamepadInputElement(element));
            TEST_VERIFY(!IsGamepadButtonInputElement(element));
            TEST_VERIFY(!IsGamepadAxisInputElement(element));
            TEST_VERIFY(!IsTouchInputElement(element));
            TEST_VERIFY(!IsTouchClickInputElement(element));
            TEST_VERIFY(!IsTouchPositionInputElement(element));
        }

        // Mouse

        for (uint32 i = static_cast<uint32>(eInputElements::MOUSE_FIRST_BUTTON); i <= static_cast<uint32>(eInputElements::MOUSE_LAST_BUTTON); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::DIGITAL);
            TEST_VERIFY(!IsKeyboardInputElement(element));
            TEST_VERIFY(!IsKeyboardModifierInputElement(element));
            TEST_VERIFY(!IsKeyboardSystemInputElement(element));
            TEST_VERIFY(IsMouseInputElement(element));
            TEST_VERIFY(IsMouseButtonInputElement(element));
            TEST_VERIFY(!IsGamepadInputElement(element));
            TEST_VERIFY(!IsGamepadButtonInputElement(element));
            TEST_VERIFY(!IsGamepadAxisInputElement(element));
            TEST_VERIFY(!IsTouchInputElement(element));
            TEST_VERIFY(!IsTouchClickInputElement(element));
            TEST_VERIFY(!IsTouchPositionInputElement(element));
        }

        InputElementInfo mouseWheelInfo = GetInputElementInfo(eInputElements::MOUSE_WHEEL);
        TEST_VERIFY(!mouseWheelInfo.name.empty());
        TEST_VERIFY(mouseWheelInfo.type == eInputElementTypes::ANALOG);
        TEST_VERIFY(!IsKeyboardInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsKeyboardModifierInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsKeyboardSystemInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(IsMouseInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsMouseButtonInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsGamepadInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsGamepadButtonInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsGamepadAxisInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsTouchInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsTouchClickInputElement(eInputElements::MOUSE_WHEEL));
        TEST_VERIFY(!IsTouchPositionInputElement(eInputElements::MOUSE_WHEEL));

        InputElementInfo mousePositionInfo = GetInputElementInfo(eInputElements::MOUSE_POSITION);
        TEST_VERIFY(!mousePositionInfo.name.empty());
        TEST_VERIFY(mousePositionInfo.type == eInputElementTypes::ANALOG);
        TEST_VERIFY(!IsKeyboardInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsKeyboardModifierInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsKeyboardSystemInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(IsMouseInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsMouseButtonInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsGamepadInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsGamepadButtonInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsGamepadAxisInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsTouchInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsTouchClickInputElement(eInputElements::MOUSE_POSITION));
        TEST_VERIFY(!IsTouchPositionInputElement(eInputElements::MOUSE_POSITION));

        // Gamepad

        for (uint32 i = static_cast<uint32>(eInputElements::GAMEPAD_FIRST_BUTTON); i <= static_cast<uint32>(eInputElements::GAMEPAD_LAST_BUTTON); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::DIGITAL);
            TEST_VERIFY(!IsKeyboardInputElement(element));
            TEST_VERIFY(!IsKeyboardModifierInputElement(element));
            TEST_VERIFY(!IsKeyboardSystemInputElement(element));
            TEST_VERIFY(!IsMouseInputElement(element));
            TEST_VERIFY(!IsMouseButtonInputElement(element));
            TEST_VERIFY(IsGamepadInputElement(element));
            TEST_VERIFY(IsGamepadButtonInputElement(element));
            TEST_VERIFY(!IsGamepadAxisInputElement(element));
            TEST_VERIFY(!IsTouchInputElement(element));
            TEST_VERIFY(!IsTouchClickInputElement(element));
            TEST_VERIFY(!IsTouchPositionInputElement(element));
        }

        for (uint32 i = static_cast<uint32>(eInputElements::GAMEPAD_FIRST_AXIS); i <= static_cast<uint32>(eInputElements::GAMEPAD_LAST_AXIS); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::ANALOG);
            TEST_VERIFY(!IsKeyboardInputElement(element));
            TEST_VERIFY(!IsKeyboardModifierInputElement(element));
            TEST_VERIFY(!IsKeyboardSystemInputElement(element));
            TEST_VERIFY(!IsMouseInputElement(element));
            TEST_VERIFY(!IsMouseButtonInputElement(element));
            TEST_VERIFY(IsGamepadInputElement(element));
            TEST_VERIFY(!IsGamepadButtonInputElement(element));
            TEST_VERIFY(IsGamepadAxisInputElement(element));
            TEST_VERIFY(!IsTouchInputElement(element));
            TEST_VERIFY(!IsTouchClickInputElement(element));
            TEST_VERIFY(!IsTouchPositionInputElement(element));
        }

        // Touch screen

        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_CLICK); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_CLICK); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::DIGITAL);
            TEST_VERIFY(!IsKeyboardInputElement(element));
            TEST_VERIFY(!IsKeyboardModifierInputElement(element));
            TEST_VERIFY(!IsKeyboardSystemInputElement(element));
            TEST_VERIFY(!IsMouseInputElement(element));
            TEST_VERIFY(!IsMouseButtonInputElement(element));
            TEST_VERIFY(!IsGamepadInputElement(element));
            TEST_VERIFY(!IsGamepadButtonInputElement(element));
            TEST_VERIFY(!IsGamepadAxisInputElement(element));
            TEST_VERIFY(IsTouchInputElement(element));
            TEST_VERIFY(IsTouchClickInputElement(element));
            TEST_VERIFY(!IsTouchPositionInputElement(element));

            TEST_VERIFY(GetTouchPositionElementFromClickElement(element) == static_cast<eInputElements>(eInputElements::TOUCH_FIRST_POSITION + (i - eInputElements::TOUCH_FIRST_CLICK)));
        }

        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_POSITION); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_POSITION); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);

            InputElementInfo info = GetInputElementInfo(element);
            TEST_VERIFY(!info.name.empty());
            TEST_VERIFY(info.type == eInputElementTypes::ANALOG);
            TEST_VERIFY(!IsKeyboardInputElement(element));
            TEST_VERIFY(!IsKeyboardModifierInputElement(element));
            TEST_VERIFY(!IsKeyboardSystemInputElement(element));
            TEST_VERIFY(!IsMouseInputElement(element));
            TEST_VERIFY(!IsMouseButtonInputElement(element));
            TEST_VERIFY(!IsGamepadInputElement(element));
            TEST_VERIFY(!IsGamepadButtonInputElement(element));
            TEST_VERIFY(!IsGamepadAxisInputElement(element));
            TEST_VERIFY(IsTouchInputElement(element));
            TEST_VERIFY(!IsTouchClickInputElement(element));
            TEST_VERIFY(IsTouchPositionInputElement(element));
        }
    }
};