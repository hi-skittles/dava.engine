#include "UnitTests/UnitTests.h"

#include <Engine/Engine.h>
#include <Engine/Private/EngineBackend.h>
#include <Engine/Private/Dispatcher/MainDispatcher.h>
#include <Engine/Private/Dispatcher/MainDispatcherEvent.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/TouchScreen.h>
#include <Logger/Logger.h>

using namespace DAVA;

DAVA_TESTCLASS (TouchScreenTestClass)
{
    DAVA_TEST (TouchScreenSupportedElementsTest)
    {
        // Check touch screen supported elements:
        //   - all touch screen elements are supported
        //   - all non-touch screen elements are not supported

        TouchScreen* touchscreen = GetEngineContext()->deviceManager->GetTouchScreen();
        if (touchscreen == nullptr)
        {
            Logger::Info("Skipping TouchScreenSupportedElementsTest since there is no touch screen");
            return;
        }

        for (uint32 i = static_cast<uint32>(eInputElements::FIRST); i <= static_cast<uint32>(eInputElements::LAST); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            TEST_VERIFY(IsTouchInputElement(element) ? touchscreen->IsElementSupported(element) : !touchscreen->IsElementSupported(element));
        }
    }

    DAVA_TEST (TouchScreenDefaultStateTest)
    {
        // Check touch screen default state: equal to released for each click, {0, 0, 0} for each position

        TouchScreen* touchscreen = GetEngineContext()->deviceManager->GetTouchScreen();
        if (touchscreen == nullptr)
        {
            Logger::Info("Skipping TouchScreenDefaultStateTest since there is no touch screen");
            return;
        }

        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_CLICK); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_CLICK); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            DigitalElementState state = touchscreen->GetDigitalElementState(element);
            TEST_VERIFY(state.IsReleased() && !state.IsJustReleased());
        }

        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_POSITION); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_POSITION); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            AnalogElementState state = touchscreen->GetAnalogElementState(element);
            TEST_VERIFY(state.x == 0.0f && state.y == 0.0f && state.z == 0.0f);
        }
    }

    DAVA_TEST (TouchScreenSingleTouchEventHandlingTest)
    {
        // Check event handling by the touch screen, using single touch:
        //   - Check that initial state is released
        //   - Imitate platform sending TOUCH_DOWN event
        //   - Check that click state has changed to just pressed, position has changed to the correct one
        //   - Imitate platform sending TOUCH_MOVE event
        //   - Check that position has changed to the new one
        //   - Wait for the next frame, check it has changed to pressed
        //   - Imititate platform sending TOUCH_UP event
        //   - Check that state has changed to just released
        //   - Wait for the next frame, check that it has changed to released, position has changed to {0, 0, 0}
        //
        // Handled in Update()

        TouchScreen* touchscreen = GetEngineContext()->deviceManager->GetTouchScreen();
        if (touchscreen == nullptr)
        {
            Logger::Info("Skipping TouchScreenSingleTouchEventHandlingTest since there is no touch screen");
            eventHandlingTestState = EventHandlingTestState::FINISHED;
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);
    }

    DAVA_TEST (TouchScreenMultiTouchEventHandlingTest)
    {
        // Check event handling by the touch screen, using multiple touches:
        //   - Check that initial state is released for each touch
        //   - Send TOUCH_DOWN for each touch, check that state has changed to just pressed
        //   - Send TOUCH_UP for one touch, check that state has changed to just released
        //   - Send TOUCH_DOWN for this touch again, check that all are just pressed again

        using namespace DAVA::Private;

        TouchScreen* touchscreen = GetEngineContext()->deviceManager->GetTouchScreen();
        if (touchscreen == nullptr)
        {
            Logger::Info("Skipping TouchScreenMultiTouchEventHandlingTest since there is no touch screen");
            return;
        }

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        std::vector<eInputElements> elementsThatShouldBePressed;

        // Press all ten touches and check their states
        for (uint32 i = eInputElements::TOUCH_CLICK0; i <= eInputElements::TOUCH_CLICK9; ++i)
        {
            eInputElements currentTouch = static_cast<eInputElements>(i);
            dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_DOWN, i, 0.0f, 0.0f, eModifierKeys::NONE));
            elementsThatShouldBePressed.push_back(currentTouch);
            CheckMultipleState(touchscreen, elementsThatShouldBePressed, DigitalElementState::JustPressed());
        }

        // Release fifth touch and check that it's state changed to JustReleased, and others haven't changed
        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_UP, static_cast<uint32>(eInputElements::TOUCH_CLICK5), 0.0f, 0.0f, eModifierKeys::NONE));
        elementsThatShouldBePressed.erase(elementsThatShouldBePressed.begin() + 5);
        CheckMultipleState(touchscreen, elementsThatShouldBePressed, DigitalElementState::JustPressed(), { eInputElements::TOUCH_CLICK5 }, DigitalElementState::JustReleased());

        // Send fifth touch press again and check that all elements are pressed again
        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_DOWN, static_cast<uint32>(eInputElements::TOUCH_CLICK5), 0.0f, 0.0f, eModifierKeys::NONE));
        elementsThatShouldBePressed.push_back(eInputElements::TOUCH_CLICK5);
        CheckMultipleState(touchscreen, elementsThatShouldBePressed, DigitalElementState::JustPressed());

        // Release all
        for (uint32 i = eInputElements::TOUCH_CLICK0; i <= eInputElements::TOUCH_CLICK9; ++i)
        {
            eInputElements currentTouch = static_cast<eInputElements>(i);
            dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_UP, i, 0.0f, 0.0f, eModifierKeys::NONE));
        }

        primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
    }

    // Check that all elements are in released state, except `requiredElement` that should be in `requiredState`
    void CheckSingleState(TouchScreen * touchScreen, eInputElements requiredElement, DigitalElementState requiredState)
    {
        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_CLICK); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_CLICK); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            DigitalElementState state = touchScreen->GetDigitalElementState(element);

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

    // Check that all elements are in released state,
    // except `requiredElements1` and `requiredElements2` that should be in `requiredState1` and `requiredState2` accordingly
    void CheckMultipleState(TouchScreen * touchScreen, std::vector<eInputElements> requiredElements1, DigitalElementState requiredState1, std::vector<eInputElements> requiredElements2 = {}, DigitalElementState requiredState2 = DigitalElementState::Released())
    {
        for (uint32 i = static_cast<uint32>(eInputElements::TOUCH_FIRST_CLICK); i <= static_cast<uint32>(eInputElements::TOUCH_LAST_CLICK); ++i)
        {
            eInputElements element = static_cast<eInputElements>(i);
            DigitalElementState state = touchScreen->GetDigitalElementState(element);

            if (std::find(requiredElements1.begin(), requiredElements1.end(), element) != requiredElements1.end())
            {
                TEST_VERIFY(state == requiredState1);
            }
            else if (std::find(requiredElements2.begin(), requiredElements2.end(), element) != requiredElements2.end())
            {
                TEST_VERIFY(state == requiredState2);
            }
            else
            {
                TEST_VERIFY(state.IsReleased() && !state.IsJustReleased());
            }
        }
    }

    enum class EventHandlingTestState
    {
        INITIAL,
        SENT_TOUCH_DOWN,
        SENT_TOUCH_UP,
        FINISHED
    };

    void Update(float32 timeElapsed, const String& testName) override
    {
        if (testName == "TouchScreenSingleTouchEventHandlingTest" && eventHandlingTestState != EventHandlingTestState::FINISHED)
        {
            using namespace DAVA::Private;

            TouchScreen* touchscreen = GetEngineContext()->deviceManager->GetTouchScreen();
            Window* primaryWindow = GetPrimaryWindow();
            MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

            if (eventHandlingTestState == EventHandlingTestState::INITIAL)
            {
                // Check that click is released
                // Send TOUCH_DOWN and check that button is just pressed, position is correct
                // Send TOUCH_MOVE and check that position is correct
                // Wait for the next frame

                Logger::Info("TouchScreenEventHandlingTest: testing element '%s'", GetInputElementInfo(eInputElements::TOUCH_CLICK0).name.c_str());

                DigitalElementState currentElementStateBeforeEvent = touchscreen->GetDigitalElementState(eInputElements::TOUCH_CLICK0);
                CheckSingleState(touchscreen, eInputElements::TOUCH_CLICK0, DigitalElementState::Released());
                AnalogElementState currentElementPositionBeforeEvent = touchscreen->GetAnalogElementState(eInputElements::TOUCH_POSITION0);
                TEST_VERIFY(currentElementPositionBeforeEvent.x == 0.0f && currentElementPositionBeforeEvent.y == 0.0f && currentElementPositionBeforeEvent.z == 0.0f);

                const float32 initialTouchX = 32.0f;
                const float32 initialTouchY = 51.3f;

                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_DOWN, static_cast<uint32>(eInputElements::TOUCH_CLICK0), initialTouchX, initialTouchY, eModifierKeys::NONE));

                DigitalElementState currentElementStateAfterEvent = touchscreen->GetDigitalElementState(eInputElements::TOUCH_CLICK0);
                CheckSingleState(touchscreen, eInputElements::TOUCH_CLICK0, DigitalElementState::JustPressed());
                AnalogElementState currentElementPositionAfterEvent = touchscreen->GetAnalogElementState(eInputElements::TOUCH_POSITION0);
                TEST_VERIFY(currentElementPositionAfterEvent.x == initialTouchX && currentElementPositionAfterEvent.y == initialTouchY && currentElementPositionAfterEvent.z == 0.0f);

                const float32 moveTouchX = 390.21f;
                const float32 moveTouchY = 124.03f;
                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_MOVE, static_cast<uint32>(eInputElements::TOUCH_CLICK0), moveTouchX, moveTouchY, eModifierKeys::NONE));
                currentElementPositionAfterEvent = touchscreen->GetAnalogElementState(eInputElements::TOUCH_POSITION0);
                TEST_VERIFY(currentElementPositionAfterEvent.x == moveTouchX && currentElementPositionAfterEvent.y == moveTouchY && currentElementPositionAfterEvent.z == 0.0f);

                eventHandlingTestState = EventHandlingTestState::SENT_TOUCH_DOWN;
            }
            else if (eventHandlingTestState == EventHandlingTestState::SENT_TOUCH_DOWN)
            {
                // Check that click is pressed
                // Send TOUCH_UP and check that click is just released
                // Wait for the next frame

                DigitalElementState currentElementStateBeforeEvent = touchscreen->GetDigitalElementState(eInputElements::TOUCH_CLICK0);
                CheckSingleState(touchscreen, eInputElements::TOUCH_CLICK0, DigitalElementState::Pressed());

                dispatcher->SendEvent(MainDispatcherEvent::CreateWindowTouchEvent(primaryWindow, MainDispatcherEvent::TOUCH_UP, static_cast<uint32>(eInputElements::TOUCH_CLICK0), 0.0f, 0.0f, eModifierKeys::NONE));

                DigitalElementState currentElementStateAfterEvent = touchscreen->GetDigitalElementState(eInputElements::TOUCH_CLICK0);
                CheckSingleState(touchscreen, eInputElements::TOUCH_CLICK0, DigitalElementState::JustReleased());
                AnalogElementState currentElementPositionAfterEvent = touchscreen->GetAnalogElementState(eInputElements::TOUCH_POSITION0);
                TEST_VERIFY(currentElementPositionAfterEvent.x == 0.0f && currentElementPositionAfterEvent.y == 0.0f && currentElementPositionAfterEvent.z == 0.0f);

                eventHandlingTestState = EventHandlingTestState::SENT_TOUCH_UP;
            }
            else if (eventHandlingTestState == EventHandlingTestState::SENT_TOUCH_UP)
            {
                // Check that click is released
                // Finish

                DigitalElementState currentElementState = touchscreen->GetDigitalElementState(eInputElements::TOUCH_CLICK0);
                CheckSingleState(touchscreen, eInputElements::TOUCH_CLICK0, DigitalElementState::Released());

                eventHandlingTestState = EventHandlingTestState::FINISHED;

                primaryWindow->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
            }
        }
    }

    bool TestComplete(const String& testName) const override
    {
        if (testName != "TouchScreenSingleTouchEventHandlingTest")
        {
            return true;
        }
        else
        {
            return eventHandlingTestState == EventHandlingTestState::FINISHED;
        }
    }

private:
    // TouchScreenEventHandlingTest variables
    EventHandlingTestState eventHandlingTestState = EventHandlingTestState::INITIAL;
};
