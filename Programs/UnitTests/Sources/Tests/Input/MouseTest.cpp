#include "UnitTests/UnitTests.h"

#include <Engine/Engine.h>
#include <Engine/Private/EngineBackend.h>
#include <Engine/Private/Dispatcher/MainDispatcher.h>
#include <Engine/Private/Dispatcher/MainDispatcherEvent.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/Mouse.h>
#include <Logger/Logger.h>

using namespace DAVA;

DAVA_TESTCLASS (MouseTestClass)
{
    DAVA_TEST (MouseSupportedElementsTest)
    {
        // Check mouse supported elements:
        //   - all mouse elements are supported
        //   - all non-mouse elements are not supported

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping MouseSupportedElementsTest since there is no mouse device");
            return;
        }

        for (uint32 i = static_cast<uint32>(eInputElements::FIRST); i <= static_cast<uint32>(eInputElements::LAST); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            TEST_VERIFY(IsMouseInputElement(element) ? mouse->IsElementSupported(element) : !mouse->IsElementSupported(element));
        }
    }

    DAVA_TEST (MouseDefaultStateTest)
    {
        // Check mouse default state: all buttons are released, wheel delta is zero

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping MouseDefaultStateTest since there is no mouse device");
            return;
        }

        for (uint32 i = static_cast<uint32>(eInputElements::MOUSE_FIRST_BUTTON); i <= static_cast<uint32>(eInputElements::MOUSE_LAST_BUTTON); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            DigitalElementState state = mouse->GetDigitalElementState(element);
            TEST_VERIFY(state.IsReleased() && !state.IsJustReleased());
        }

        AnalogElementState wheelState = mouse->GetAnalogElementState(eInputElements::MOUSE_WHEEL);
        TEST_VERIFY(wheelState.x == 0.0f && wheelState.y == 0.0f && wheelState.z == 0.0f);
    }

    DAVA_TEST (MouseButtonsEventHandlingTest)
    {
        // Check event handling by the mouse, for each button:
        //   - Check that initial state is released
        //   - Imitate platform sending MOUSE_BUTTON_DOWN event
        //   - Check that state has changed to just pressed, position has changed to the correct one
        //   - Wait for the next frame, check that state has changed to pressed
        //   - Imititate platform sending MOUSE_BUTTON_UP event
        //   - Check that state has changed to just released, position has changed to the correct one
        //   - Wait for the next frame, check that state has changed to released
        //
        // Handled in Update()

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping MouseButtonsEventHandlingTest since there is no mouse device");
            buttonsEventHandlingTestState = ButtonsEventHandlingTestState::FINISHED;
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);
    }

    DAVA_TEST (MouseWheelEventHandlingTest)
    {
        // Check event handling by the mouse, for the wheel element:
        //   - Check that intitial state is {0, 0, 0}
        //   - Imitate platform sending MOUSE_WHEEL event
        //   - Check that state has changed to the correct one
        //   - Wait for the next frame and make sure wheel state has been set to {0, 0, 0} again
        // Handled in Update()

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping MouseWheelEventHandlingTest since there is no mouse device");
            wheelEventHandlingTestState = WheelEventHandlingTestState::FINISHED;
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);
    }

    DAVA_TEST (MousePositionEventHandlingTest)
    {
        // Check event handling by the mouse, for position

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping MousePositionEventHandlingTest since there is no mouse device");
            return;
        }

        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        float32 newX = 241.01f;
        float32 newY = 7.05f;
        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(primaryWindow, newX, newY, eModifierKeys::NONE, false));
        AnalogElementState mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
        TEST_VERIFY(mousePos.x == newX && mousePos.y == newY);

        newX = 11.34f;
        newY = 513.51f;
        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(primaryWindow, newX, newY, eModifierKeys::NONE, false));
        mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
        TEST_VERIFY(mousePos.x == newX && mousePos.y == newY);

        newX = 0.03f;
        newY = 1021.3f;
        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(primaryWindow, newX, newY, eModifierKeys::NONE, false));
        mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
        TEST_VERIFY(mousePos.x == newX && mousePos.y == newY);

        primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
    }

    void CheckSingleState(Mouse * mouse, eInputElements requiredElement, DigitalElementState requiredState)
    {
        // All buttons should be in released state, `requiredElement` must be in `requiredState`
        for (uint32 i = static_cast<uint32>(eInputElements::MOUSE_FIRST_BUTTON); i <= static_cast<uint32>(eInputElements::MOUSE_LAST_BUTTON); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            DigitalElementState state = mouse->GetDigitalElementState(element);

            if (element == requiredElement)
            {
                TEST_VERIFY(state == requiredState);
            }
            else
            {
                TEST_VERIFY(state.IsReleased() && !state.IsJustReleased());
            }
        }
    }

    enum class ButtonsEventHandlingTestState
    {
        INITIAL,
        SENT_MOUSE_BUTTON_DOWN,
        SENT_MOUSE_BUTTON_UP,
        FINISHED
    };

    enum class WheelEventHandlingTestState
    {
        INITIAL,
        SENT_MOUSE_WHEEL,
        FINISHED
    };

    void Update(float32 timeElapsed, const String& testName) override
    {
        using namespace DAVA::Private;

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        if (testName == "MouseButtonsEventHandlingTest" && buttonsEventHandlingTestState != ButtonsEventHandlingTestState::FINISHED)
        {
            if (buttonsEventHandlingTestState == ButtonsEventHandlingTestState::INITIAL)
            {
                // Check that a button is released
                // Send MOUSE_BUTTON_DOWN and check that button state changed to JustPressed and position changed
                // Wait for the next frame

                Logger::Info("MouseButtonsEventHandlingTest: testing element '%s'", GetInputElementInfo(currentElement).name.c_str());

                DigitalElementState currentElementStateBeforeEvent = mouse->GetDigitalElementState(currentElement);
                CheckSingleState(mouse, currentElement, DigitalElementState::Released());

                const float32 clickX = 412.1f;
                const float32 clickY = 3.3f;

                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_DOWN, static_cast<eMouseButtons>(currentElement - eInputElements::MOUSE_FIRST_BUTTON + 1), clickX, clickY, 0, DAVA::eModifierKeys::NONE, false));

                DigitalElementState currentElementStateAfterEvent = mouse->GetDigitalElementState(currentElement);
                CheckSingleState(mouse, currentElement, DigitalElementState::JustPressed());

                AnalogElementState mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
                TEST_VERIFY(mousePos.x == clickX && mousePos.y == clickY);

                buttonsEventHandlingTestState = ButtonsEventHandlingTestState::SENT_MOUSE_BUTTON_DOWN;
            }
            else if (buttonsEventHandlingTestState == ButtonsEventHandlingTestState::SENT_MOUSE_BUTTON_DOWN)
            {
                // Check that button is pressed
                // Send MOUSE_BUTTON_UP and check that button state changed to JustReleased and position changed
                // Wait for the next frame

                DigitalElementState currentElementStateBeforeEvent = mouse->GetDigitalElementState(currentElement);
                CheckSingleState(mouse, currentElement, DigitalElementState::Pressed());

                const float32 releaseX = 412.1f;
                const float32 releaseY = 3.3f;

                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_UP, static_cast<eMouseButtons>(currentElement - eInputElements::MOUSE_FIRST_BUTTON + 1), releaseX, releaseY, 0, DAVA::eModifierKeys::NONE, false));

                DigitalElementState currentElementStateAfterEvent = mouse->GetDigitalElementState(currentElement);
                CheckSingleState(mouse, currentElement, DigitalElementState::JustReleased());

                AnalogElementState mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
                TEST_VERIFY(mousePos.x == releaseX && mousePos.y == releaseY);

                buttonsEventHandlingTestState = ButtonsEventHandlingTestState::SENT_MOUSE_BUTTON_UP;
            }
            else if (buttonsEventHandlingTestState == ButtonsEventHandlingTestState::SENT_MOUSE_BUTTON_UP)
            {
                // Check that button is released
                // Go to the next element (if not finished yet)

                DigitalElementState currentElementState = mouse->GetDigitalElementState(currentElement);
                CheckSingleState(mouse, currentElement, DigitalElementState::Released());

                currentElement = static_cast<eInputElements>(currentElement + 1);

                if (currentElement > eInputElements::MOUSE_LAST_BUTTON)
                {
                    buttonsEventHandlingTestState = ButtonsEventHandlingTestState::FINISHED;
                    primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
                }
                else
                {
                    buttonsEventHandlingTestState = ButtonsEventHandlingTestState::INITIAL;
                }
            }
        }
        else if (testName == "MouseWheelEventHandlingTest" && wheelEventHandlingTestState != WheelEventHandlingTestState::FINISHED)
        {
            if (wheelEventHandlingTestState == WheelEventHandlingTestState::INITIAL)
            {
                // Check initial wheel state
                // Send MOUSE_WHEEL event
                // Check new wheel state, check new position
                // Wait for the next frame

                AnalogElementState initialState = mouse->GetAnalogElementState(eInputElements::MOUSE_WHEEL);
                TEST_VERIFY(initialState.x == 0.0f && initialState.y == 0.0f);

                const float32 posX = 251.11f;
                const float32 posY = 8.23f;
                const float32 wheelDeltaX = 0.17f;
                const float32 wheelDeltaY = 5.79f;

                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseWheelEvent(primaryWindow, posX, posY, wheelDeltaX, wheelDeltaY, eModifierKeys::NONE, false));

                AnalogElementState newState = mouse->GetAnalogElementState(eInputElements::MOUSE_WHEEL);
                TEST_VERIFY(newState.x == wheelDeltaX && newState.y == wheelDeltaY);

                AnalogElementState mousePos = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);
                TEST_VERIFY(mousePos.x == posX && mousePos.y == posY);

                wheelEventHandlingTestState = WheelEventHandlingTestState::SENT_MOUSE_WHEEL;
            }
            else if (wheelEventHandlingTestState == WheelEventHandlingTestState::SENT_MOUSE_WHEEL)
            {
                AnalogElementState currentState = mouse->GetAnalogElementState(eInputElements::MOUSE_WHEEL);
                TEST_VERIFY(currentState.x == 0.0f && currentState.y == 0.0f);

                wheelEventHandlingTestState = WheelEventHandlingTestState::FINISHED;

                primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
            }
        }
    }

    bool TestComplete(const String& testName) const override
    {
        if (testName == "MouseButtonsEventHandlingTest")
        {
            return buttonsEventHandlingTestState == ButtonsEventHandlingTestState::FINISHED;
        }
        else if (testName == "MouseWheelEventHandlingTest")
        {
            return wheelEventHandlingTestState == WheelEventHandlingTestState::FINISHED;
        }
        else
        {
            return true;
        }
    }

private:
    // MouseButtonsEventHandlingTest variables
    eInputElements currentElement = eInputElements::MOUSE_FIRST_BUTTON;
    ButtonsEventHandlingTestState buttonsEventHandlingTestState = ButtonsEventHandlingTestState::INITIAL;

    // MouseWheelEventHandlingTest
    WheelEventHandlingTestState wheelEventHandlingTestState = WheelEventHandlingTestState::INITIAL;
};
