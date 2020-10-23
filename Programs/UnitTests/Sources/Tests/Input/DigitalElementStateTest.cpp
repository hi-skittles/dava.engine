#include "UnitTests/UnitTests.h"

#include <Input/InputSystemTypes.h>

using namespace DAVA;

DAVA_TESTCLASS (DigitalElementStateTestClass)
{
    DAVA_TEST (DigitalElementStateTest)
    {
        // Check creation

        DigitalElementState stateReleased;
        TEST_VERIFY(stateReleased.IsReleased());
        TEST_VERIFY(!stateReleased.IsJustReleased());
        TEST_VERIFY(!stateReleased.IsPressed());
        TEST_VERIFY(!stateReleased.IsJustPressed());

        DigitalElementState stateJustReleased = DigitalElementState::JustReleased();
        TEST_VERIFY(stateJustReleased.IsReleased());
        TEST_VERIFY(stateJustReleased.IsJustReleased());
        TEST_VERIFY(!stateJustReleased.IsPressed());
        TEST_VERIFY(!stateJustReleased.IsJustPressed());

        DigitalElementState statePressed = DigitalElementState::Pressed();
        TEST_VERIFY(!statePressed.IsReleased());
        TEST_VERIFY(!statePressed.IsJustReleased());
        TEST_VERIFY(statePressed.IsPressed());
        TEST_VERIFY(!statePressed.IsJustPressed());

        DigitalElementState stateJustPressed = DigitalElementState::JustPressed();
        TEST_VERIFY(!stateJustPressed.IsReleased());
        TEST_VERIFY(!stateJustPressed.IsJustReleased());
        TEST_VERIFY(stateJustPressed.IsPressed());
        TEST_VERIFY(stateJustPressed.IsJustPressed());

        // Check Press, Release, OnEndFrame

        DigitalElementState state;

        // OnEndFrame -> no changes for released state
        state.OnEndFrame();
        TEST_VERIFY(state.IsReleased());
        TEST_VERIFY(!state.IsJustReleased());
        TEST_VERIFY(!state.IsPressed());
        TEST_VERIFY(!state.IsJustPressed());

        // Press -> just pressed | pressed
        state.Press();
        TEST_VERIFY(!state.IsReleased());
        TEST_VERIFY(!state.IsJustReleased());
        TEST_VERIFY(state.IsPressed());
        TEST_VERIFY(state.IsJustPressed());

        // OnEndFrame -> pressed
        state.OnEndFrame();
        TEST_VERIFY(!state.IsReleased());
        TEST_VERIFY(!state.IsJustReleased());
        TEST_VERIFY(state.IsPressed());
        TEST_VERIFY(!state.IsJustPressed());

        // OnEndFrame second time -> no changes
        state.OnEndFrame();
        TEST_VERIFY(!state.IsReleased());
        TEST_VERIFY(!state.IsJustReleased());
        TEST_VERIFY(state.IsPressed());
        TEST_VERIFY(!state.IsJustPressed());

        // Release -> released | just released
        state.Release();
        TEST_VERIFY(state.IsReleased());
        TEST_VERIFY(state.IsJustReleased());
        TEST_VERIFY(!state.IsPressed());
        TEST_VERIFY(!state.IsJustPressed());

        // OnEndFrame -> released
        state.OnEndFrame();
        TEST_VERIFY(state.IsReleased());
        TEST_VERIFY(!state.IsJustReleased());
        TEST_VERIFY(!state.IsPressed());
        TEST_VERIFY(!state.IsJustPressed());

        // Check comparisons

        TEST_VERIFY(stateReleased == stateReleased); //-V501
        TEST_VERIFY(stateJustReleased == stateJustReleased); //-V501
        TEST_VERIFY(statePressed == statePressed); //-V501
        TEST_VERIFY(stateJustPressed == stateJustPressed); //-V501

        TEST_VERIFY(stateReleased != statePressed);
        TEST_VERIFY(statePressed != stateReleased);
        TEST_VERIFY(stateReleased != stateJustReleased);
        TEST_VERIFY(statePressed != stateJustPressed);
    }
};
