#include "UnitTests/UnitTests.h"

#include <Engine/Engine.h>
#include <Engine/Private/EngineBackend.h>
#include <Engine/Private/Dispatcher/MainDispatcher.h>
#include <Engine/Private/Dispatcher/MainDispatcherEvent.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <Input/InputBindingListener.h>
#include <Logger/Logger.h>

using namespace DAVA;

DAVA_TESTCLASS (InputBindingListenerTestClass)
{
    void OnListeningEnded(bool cancelled, const Vector<InputEvent>& result)
    {
        ++callbackCounter;
        lastCallbackCancelled = cancelled;
        lastCallbackResult = result;
    }

    void ResetCallbaksState()
    {
        callbackCounter = 0;
        lastCallbackCancelled = false;
        lastCallbackResult.clear();
    }

    DAVA_TEST (InputBindingListenerDigitalSingleWithoutModifiersTest)
    {
        InputBindingListener* listener = GetEngineContext()->inputListener;

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping InputListenerDigitalSingleWithoutModifiersTest since there is no keyboard device");
            return;
        }

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggeredDeviceTest since there is no mouse device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        {
            // All devices

            // Test single non-modifer key press
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_O);
            SendKeyboardKeyUp(kb, eInputElements::KB_O);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test modifier + non-modifier key press, only second key should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendKeyboardKeyDown(kb, eInputElements::KB_Z);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_Z);
            SendKeyboardKeyUp(kb, eInputElements::KB_Z);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test mouse move + modifier + mouse button key press, only mouse button should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendMouseMove(mouse, 0.0f, 0.0f);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::MOUSE_RBUTTON);
            SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());
        }

        {
            // Only keyboard device, by id

            // Test single non-modifer key press
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded), kb->GetId());
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_O);
            SendKeyboardKeyUp(kb, eInputElements::KB_O);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test mouse move + modifier + mouse button key press + non-modifier keyboard key press, only last key should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded), kb->GetId());
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendMouseMove(mouse, 0.0f, 0.0f);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyDown(kb, eInputElements::KB_H);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_H);
            SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            SendKeyboardKeyUp(kb, eInputElements::KB_H);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());
        }

        {
            // Only keyboard device, by type

            // Test single non-modifer key press
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded), eInputDeviceTypes::CLASS_KEYBOARD);
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_O);
            SendKeyboardKeyUp(kb, eInputElements::KB_O);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test mouse move + modifier + mouse button key press + non-modifier keyboard key press, only last key should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITHOUT_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded), eInputDeviceTypes::CLASS_KEYBOARD);
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendMouseMove(mouse, 0.0f, 0.0f);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyDown(kb, eInputElements::KB_H);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_H);
            SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            SendKeyboardKeyUp(kb, eInputElements::KB_H);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
    }

    DAVA_TEST (InputBindingListenerDigitalSingleWithModifiersTest)
    {
        InputBindingListener* listener = GetEngineContext()->inputListener;

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping InputListenerDigitalSingleWithModifiersTest since there is no keyboard device");
            return;
        }

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping InputListenerDigitalSingleWithModifiersTest since there is no mouse device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        {
            // All devices

            // Test single non-modifer key press
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITH_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_O);
            SendKeyboardKeyUp(kb, eInputElements::KB_O);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test modifier + non-modifier key press, both should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITH_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendKeyboardKeyDown(kb, eInputElements::KB_Z);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 2);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_LALT);
            TEST_VERIFY(lastCallbackResult[1].elementId == eInputElements::KB_Z);
            SendKeyboardKeyUp(kb, eInputElements::KB_Z);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test mouse move + modifier + mouse button key press, move should be ignored
            listener->Listen(eInputBindingListenerModes::DIGITAL_SINGLE_WITH_MODIFIERS, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendMouseMove(mouse, 0.0f, 0.0f);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 2);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_LALT);
            TEST_VERIFY(lastCallbackResult[1].elementId == eInputElements::MOUSE_RBUTTON);
            SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
    }

    DAVA_TEST (InputBindingListenerDigitalAnyTest)
    {
        InputBindingListener* listener = GetEngineContext()->inputListener;

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping InputListenerDigitalAnyTest since there is no keyboard device");
            return;
        }

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();
        if (mouse == nullptr)
        {
            Logger::Info("Skipping InputListenerDigitalAnyTest since there is no mouse device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        {
            // All devices

            // Test single non-modifer key press
            listener->Listen(eInputBindingListenerModes::DIGITAL_MULTIPLE_ANY, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 0); // Since we're listening for any number of keys, callback shouldn't be invoked yet
            SendKeyboardKeyUp(kb, eInputElements::KB_O);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 1);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_O);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test modifier + non-modifier key press, both should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_MULTIPLE_ANY, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendKeyboardKeyDown(kb, eInputElements::KB_Z);
            TEST_VERIFY(callbackCounter == 0); // Since we're listening for any number of keys, callback shouldn't be invoked yet
            SendKeyboardKeyUp(kb, eInputElements::KB_Z);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 2);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_LALT);
            TEST_VERIFY(lastCallbackResult[1].elementId == eInputElements::KB_Z);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());

            // Test mouse move + modifier + two mouse button key presses, move should be ignored, others should be registered
            listener->Listen(eInputBindingListenerModes::DIGITAL_MULTIPLE_ANY, MakeFunction(this, &InputBindingListenerTestClass::OnListeningEnded));
            TEST_VERIFY(listener->IsListening());
            SendKeyboardKeyDown(kb, eInputElements::KB_LALT);
            SendMouseMove(mouse, 0.0f, 0.0f);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
            SendMouseButtonDown(mouse, eInputElements::MOUSE_EXT1BUTTON);
            TEST_VERIFY(callbackCounter == 0); // Since we're listening for any number of keys, callback shouldn't be invoked yet
            SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
            TEST_VERIFY(callbackCounter == 1);
            TEST_VERIFY(lastCallbackCancelled == false);
            TEST_VERIFY(lastCallbackResult.size() == 3);
            TEST_VERIFY(lastCallbackResult[0].elementId == eInputElements::KB_LALT);
            TEST_VERIFY(lastCallbackResult[1].elementId == eInputElements::MOUSE_RBUTTON);
            TEST_VERIFY(lastCallbackResult[2].elementId == eInputElements::MOUSE_EXT1BUTTON);
            SendMouseButtonUp(mouse, eInputElements::MOUSE_EXT1BUTTON);
            SendKeyboardKeyUp(kb, eInputElements::KB_LALT);
            ResetCallbaksState();
            TEST_VERIFY(!listener->IsListening());
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
    }

    // TODO: other modes

    void SendKeyboardKeyDown(Keyboard * kb, eInputElements key)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(primaryWindow, MainDispatcherEvent::KEY_DOWN, kb->GetKeyNativeScancode(key), kb->GetKeyNativeScancode(key), DAVA::eModifierKeys::NONE, false));
    }

    void SendKeyboardKeyUp(Keyboard * kb, eInputElements key)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(primaryWindow, MainDispatcherEvent::KEY_UP, kb->GetKeyNativeScancode(key), kb->GetKeyNativeScancode(key), DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseButtonDown(Mouse * mouse, eInputElements button)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_DOWN, static_cast<eMouseButtons>(button - eInputElements::MOUSE_FIRST_BUTTON + 1), 0.0f, 0.0f, 0, DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseButtonUp(Mouse * mouse, eInputElements button)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_UP, static_cast<eMouseButtons>(button - eInputElements::MOUSE_FIRST_BUTTON + 1), 0.0f, 0.0f, 0, DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseMove(Mouse * mouse, float toX, float toY, bool relative = false)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->SendEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(primaryWindow, toX, toY, eModifierKeys::NONE, relative));
    }

private:
    int callbackCounter = 0;
    bool lastCallbackCancelled = false;
    Vector<InputEvent> lastCallbackResult;
};
