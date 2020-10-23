#include "UnitTests/UnitTests.h"

#include <Engine/Engine.h>
#include <Engine/Private/EngineBackend.h>
#include <Engine/Private/Dispatcher/MainDispatcher.h>
#include <Engine/Private/Dispatcher/MainDispatcherEvent.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/ActionSystem.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <Input/TouchScreen.h>
#include <Logger/Logger.h>

using namespace DAVA;

DAVA_TESTCLASS (ActionSystemTest)
{
    void Update(float32 timeElapsed, const String& testName) override
    {
        auto testChainIter = testsChains.find(testName);

        if (testChainIter == testsChains.end())
        {
            return;
        }

        std::queue<PrepareVerify>& testChain = testChainIter->second;

        static TestFn verify;

        if (verify)
        {
            verify();
        }

        if (!testChain.empty())
        {
            TestFn prepare = testChain.front().prepare;
            if (prepare)
            {
                prepare();
            }
            verify = testChain.front().verify;

            testChain.pop();
        }
        else
        {
            testFinishedStatus[testName] = true;
            verify = TestFn();
        }
    }

    bool TestComplete(const String& testName) const override
    {
        auto status = testFinishedStatus.find(testName);

        if (status != testFinishedStatus.end())
        {
            return status->second;
        }

        return true;
    }

    void OnActionTriggered(Action action)
    {
        lastTriggeredAction.reset(new Action(action));
        ++triggeredActionsCounter;
    }

    void ResetTriggeredActionsInfo()
    {
        lastTriggeredAction.reset();
        triggeredActionsCounter = 0;
    }

    void VerifyActiveDigitalActions(const Vector<FastName>& activeActionsIds, const Vector<FastName>& allActions)
    {
        for (FastName actionId : allActions)
        {
            if (std::find(activeActionsIds.begin(), activeActionsIds.end(), actionId) == activeActionsIds.end())
            {
                TEST_VERIFY(!GetEngineContext()->actionSystem->GetDigitalActionState(actionId));
            }
            else
            {
                TEST_VERIFY(GetEngineContext()->actionSystem->GetDigitalActionState(actionId));
            }
        }
    }

    void VerifyActiveAnalogActions(const Vector<FastName>& activeActionsIds, const Vector<FastName>& allActions)
    {
        for (FastName actionId : allActions)
        {
            if (std::find(activeActionsIds.begin(), activeActionsIds.end(), actionId) == activeActionsIds.end())
            {
                TEST_VERIFY(!GetEngineContext()->actionSystem->GetAnalogActionState(actionId).active);
            }
            else
            {
                TEST_VERIFY(GetEngineContext()->actionSystem->GetAnalogActionState(actionId).active);
            }
        }
    }

    DAVA_TEST (ActionSystemActionTriggerDigitalTest)
    {
        // Check different combinations of digital elements that should trigger an action

        // Prepare test chain. We should send events on current update and check them on the next one
        // Prepare - Verify chain handled in the 'Update' method.
        const String testName = "ActionSystemActionTriggerDigitalTest";
        std::queue<PrepareVerify>& chain = testsChains[testName];

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggerDigitalTest since there is no keyboard device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        ActionSystem* actionSystem = GetEngineContext()->actionSystem;
        actionSystem->ActionTriggered.Connect(this, &ActionSystemTest::OnActionTriggered);

        // Bind test set

        ActionSet set;

        // W, triggered continuously
        DigitalBinding action1;
        action1.actionId = ACTION_1;
        action1.digitalElements[0] = eInputElements::KB_W;
        action1.digitalStates[0] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action1);

        // Space, triggered once
        DigitalBinding action2;
        action2.actionId = ACTION_2;
        action2.digitalElements[0] = eInputElements::KB_SPACE;
        action2.digitalStates[0] = DigitalElementState::JustPressed();
        set.digitalBindings.push_back(action2);

        // LCtrl + Space, triggered once
        DigitalBinding action3;
        action3.actionId = ACTION_3;
        action3.digitalElements[0] = eInputElements::KB_LCTRL;
        action3.digitalStates[0] = DigitalElementState::Pressed();
        action3.digitalElements[1] = eInputElements::KB_SPACE;
        action3.digitalStates[1] = DigitalElementState::JustPressed();
        set.digitalBindings.push_back(action3);

        // RCtrl + Space, triggered continuously
        DigitalBinding action4;
        action4.actionId = ACTION_4;
        action4.digitalElements[0] = eInputElements::KB_RCTRL;
        action4.digitalStates[0] = DigitalElementState::Pressed();
        action4.digitalElements[1] = eInputElements::KB_SPACE;
        action4.digitalStates[1] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action4);

        // O, triggered once on release
        DigitalBinding action5;
        action5.actionId = ACTION_5;
        action5.digitalElements[0] = eInputElements::KB_O;
        action5.digitalStates[0] = DigitalElementState::JustReleased();
        set.digitalBindings.push_back(action5);

        Vector<FastName> actions = { ACTION_1, ACTION_2, ACTION_3, ACTION_4, ACTION_5 };

        actionSystem->BindSet(set, kb->GetId());

        // Check action1 (W Pressed)
        {
            // Press W, action1 should be triggered once
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_1);

                    VerifyActiveDigitalActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, action1 should be triggered, because W is still pressed
            {
                TestFn prepare = [this, kb]() {
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_1);

                    VerifyActiveDigitalActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release W, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press W again, action1 should be triggered once
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_1);

                    VerifyActiveDigitalActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release W, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press A, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_A);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release A, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_A);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release W, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Check action2 (Space JustPressed)
        {
            // Press Space, action2 should be triggered once
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_2);

                    VerifyActiveDigitalActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                };
                TestFn verify = [this, kb, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press Space again, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release Space, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press Space, action2 should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_2);

                    VerifyActiveDigitalActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press R, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_R);
                };
                TestFn verify = [this, actions] {
                    TEST_VERIFY(triggeredActionsCounter == 2);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release R, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_R);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release Space, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Check action3 (LCtrl Pressed, Space JustPressed)
        {
            // Press LCtrl and Space, action3 should be triggered once
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_LCTRL);
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_3);

                    VerifyActiveDigitalActions({ ACTION_3 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press Space, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press LCtrl, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_LCTRL);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release LCtrl, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_LCTRL);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release Space, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Check action4 (RCtrl Pressed, Space Pressed)
        {
            // Press RCtrl and Space, action4 should be triggered once
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_RCTRL);
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_4);

                    VerifyActiveDigitalActions({ ACTION_4 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, action4 should be triggered, because RCtrl and Space are still pressed
            {
                TestFn prepare = [this, kb]() {
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_4);

                    VerifyActiveDigitalActions({ ACTION_4 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press Space, action4 should still be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_4);

                    VerifyActiveDigitalActions({ ACTION_4 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press RCtrl, action4 should still be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_RCTRL);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_4);

                    VerifyActiveDigitalActions({ ACTION_4 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release RCtrl, no action should be triggered
            {
                TestFn prepare = [this, kb, actions]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_RCTRL);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release Space, no action should be triggered
            {
                TestFn prepare = [this, kb, actions]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Check action5 (O just released)
        {
            // Release O, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_O);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 0);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press O, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_O);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 0);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                };
                TestFn verify = [this, kb, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 0);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release O, action5 should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_O);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_5);

                    VerifyActiveDigitalActions({ ACTION_5 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                };
                TestFn verify = [this, kb, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press O, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_O);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release O, action5 should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_O);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_5);

                    VerifyActiveDigitalActions({ ACTION_5 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Cleanup
        TestFn prepare = [this]() {
            GetEngineContext()->actionSystem->UnbindAllSets();
            GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
            GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
        };
        TestFn verify;
        chain.emplace(prepare, verify);
    }

    DAVA_TEST (ActionSystemActionTriggerAnalogTest)
    {
        // Check different combinations of digital and analog elements that should trigger an action with analog data
        // Use mouse device for that

        // Prepare test chain. We should send events on current update and check them on the next one
        // Prepare - Verify chain handled in the 'Update' method.
        const String testName = "ActionSystemActionTriggerAnalogTest";
        std::queue<PrepareVerify>& chain = testsChains[testName];

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();

        if (mouse == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggerAnalogTest test since there is no mouse device");
            return;
        }

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggerAnalogTest since there is no keyboard device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        ActionSystem* actionSystem = GetEngineContext()->actionSystem;

        actionSystem->ActionTriggered.Connect(this, &ActionSystemTest::OnActionTriggered);

        ActionSet set;

        // Mouse moved
        AnalogBinding action1;
        action1.actionId = ACTION_1;
        action1.analogElementId = eInputElements::MOUSE_POSITION;
        set.analogBindings.push_back(action1);

        // RButton Pressed + Mouse moved
        AnalogBinding action2;
        action2.actionId = ACTION_2;
        action2.digitalElements[0] = eInputElements::MOUSE_RBUTTON;
        action2.digitalStates[0] = DigitalElementState::Pressed();
        action2.analogElementId = eInputElements::MOUSE_POSITION;
        set.analogBindings.push_back(action2);

        // W Pressed + Mouse moved
        AnalogBinding action3;
        action3.actionId = ACTION_3;
        action3.digitalElements[0] = eInputElements::KB_W;
        action3.digitalStates[0] = DigitalElementState::Pressed();
        action3.analogElementId = eInputElements::MOUSE_POSITION;
        set.analogBindings.push_back(action3);

        // Q Pressed + Mouse moved
        AnalogBinding action4;
        action4.actionId = ACTION_4;
        action4.digitalElements[0] = eInputElements::KB_Q;
        action4.digitalStates[0] = DigitalElementState::Pressed();
        action4.analogElementId = eInputElements::MOUSE_POSITION;
        set.analogBindings.push_back(action4);

        // Q and W Pressed + Mouse moved
        AnalogBinding action5;
        action5.actionId = ACTION_5;
        action5.digitalElements[0] = eInputElements::KB_Q;
        action5.digitalStates[0] = DigitalElementState::Pressed();
        action5.digitalElements[1] = eInputElements::KB_W;
        action5.digitalStates[1] = DigitalElementState::Pressed();
        action5.analogElementId = eInputElements::MOUSE_POSITION;
        set.analogBindings.push_back(action5);

        actionSystem->BindSet(set, mouse->GetId(), kb->GetId());

        Vector<FastName> actions = { ACTION_1, ACTION_2, ACTION_3, ACTION_4, ACTION_5 };

        {
            float32 mousePosX = 13.38f;
            float32 mousePosY = 42.72f;

            // Move mouse, action1 should be triggered
            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Press Mouse RButton, no action should be triggered, but 'GetAnalogActionState' method should return
            // active AnalogActionState for action2 with correct coordinates
            {
                TestFn prepare = [this, mouse]() {
                    SendMouseButtonDown(mouse, eInputElements::MOUSE_RBUTTON);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_2);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Move mouse and check that action2 is triggered with correct positions

            mousePosX = 412.421f;
            mousePosY = 1.03f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_2);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 5.0f;
            mousePosY = 21.81f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_2);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 531.1f;
            mousePosY = 0.0f;

            {
                TestFn prepare = [this, actions, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_2);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 2.0f;
            mousePosY = -3.0f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 5);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_2);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 33.0f;
            mousePosY = 44.0f;

            // Release RBUTTON, move mouse, action1 should be triggered
            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseButtonUp(mouse, eInputElements::MOUSE_RBUTTON);
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 6);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 43.0f;
            mousePosY = 54.0f;

            // Press Q, W, move mouse, action5 should be triggered
            {
                TestFn prepare = [this, mouse, kb, mousePosX, mousePosY]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_Q);
                    SendKeyboardKeyDown(kb, eInputElements::KB_W);
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 7);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_5);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);
                    TEST_VERIFY(actionState.active);

                    VerifyActiveAnalogActions({ ACTION_5 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 53.0f;
            mousePosY = 64.0f;

            // Release Q, move mouse, action3 should be triggered
            {
                TestFn prepare = [this, mouse, kb, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                    SendKeyboardKeyUp(kb, eInputElements::KB_Q);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 8);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_3);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_3 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            mousePosX = 63.0f;
            mousePosY = 74.0f;

            // Release W, move mouse, action1 should be triggered
            {
                TestFn prepare = [this, mouse, kb, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions, mousePosX, mousePosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 9);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.x == mousePosX);
                    TEST_VERIFY(actionState.y == mousePosY);

                    VerifyActiveAnalogActions({ ACTION_1 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            float32 deltaX = 15.0f;
            float32 deltaY = 25.0f;

            Window* window = GetPrimaryWindow();
            // Turn on pinning mode
            {
                TestFn prepare = [this, window]() {
                    window->SetCursorCapture(eCursorCapture::PINNING);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }

            // Handle input activation
            {
                TestFn prepare = [this, mouse]() {
                    SendMouseButtonUp(mouse, eInputElements::MOUSE_LBUTTON);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }

            // Pinning mode test, no action should be triggered
            {
                TestFn prepare = [this, mouse, deltaX, deltaY]() {
                    SendMouseMove(mouse, deltaX, deltaY, true);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 9);
                };
                chain.emplace(prepare, verify);
            }

            // Reset pinning mode
            {
                TestFn prepare = [this, window]() {
                    window->SetCursorCapture(eCursorCapture::OFF);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }
        }

        // Cleanup
        TestFn prepare = [this]() {
            GetEngineContext()->actionSystem->UnbindAllSets();
            GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
            GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
            ResetTriggeredActionsInfo();
        };
        TestFn verify;
        chain.emplace(prepare, verify);
    }

    DAVA_TEST (ActionSystemActionTriggerAnalogRelativeTest)
    {
        // Prepare test chain. We should send events on current update and check them on the next one
        // Prepare - Verify chain handled in the 'Update' method.
        const String testName = "ActionSystemActionTriggerAnalogRelativeTest";
        std::queue<PrepareVerify>& chain = testsChains[testName];

        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();

        if (mouse == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggerAnalogRelativeTest test since there is no mouse device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        ActionSystem* actionSystem = GetEngineContext()->actionSystem;

        actionSystem->ActionTriggered.Connect(this, &ActionSystemTest::OnActionTriggered);

        ActionSet set;

        // Mouse moved
        AnalogBinding action1;
        action1.actionId = ACTION_1;
        action1.analogElementId = eInputElements::MOUSE_POSITION;
        action1.analogStateType = AnalogBinding::eAnalogStateType::RELATIVE_STATE;
        set.analogBindings.push_back(action1);

        actionSystem->BindSet(set, mouse->GetId());

        // action1 relative coordinates test
        {
            AnalogElementState initialPosition = mouse->GetAnalogElementState(eInputElements::MOUSE_POSITION);

            float32 prevPosX = initialPosition.x;
            float32 prevPosY = initialPosition.y;

            float32 mousePosX = 13.37f;
            float32 mousePosY = 42.72f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, mousePosX, mousePosY, prevPosX, prevPosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX - prevPosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY - prevPosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.active);
                    TEST_VERIFY(actionState.x == mousePosX - prevPosX);
                    TEST_VERIFY(actionState.y == mousePosY - prevPosY);
                };
                chain.emplace(prepare, verify);
            }

            prevPosX = mousePosX;
            prevPosY = mousePosY;

            mousePosX = 5.0f;
            mousePosY = 21.81f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, mousePosX, mousePosY, prevPosX, prevPosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 2);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX - prevPosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY - prevPosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.active);
                    TEST_VERIFY(actionState.x == mousePosX - prevPosX);
                    TEST_VERIFY(actionState.y == mousePosY - prevPosY);
                };
                chain.emplace(prepare, verify);
            }

            prevPosX = mousePosX;
            prevPosY = mousePosY;

            mousePosX = 531.1f;
            mousePosY = 0.0f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, mousePosX, mousePosY, prevPosX, prevPosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX - prevPosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY - prevPosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.active);
                    TEST_VERIFY(actionState.x == mousePosX - prevPosX);
                    TEST_VERIFY(actionState.y == mousePosY - prevPosY);
                };
                chain.emplace(prepare, verify);
            }

            prevPosX = mousePosX;
            prevPosY = mousePosY;

            mousePosX = 2.0f;
            mousePosY = -3.0f;

            {
                TestFn prepare = [this, mouse, mousePosX, mousePosY]() {
                    SendMouseMove(mouse, mousePosX, mousePosY);
                };
                TestFn verify = [this, mousePosX, mousePosY, prevPosX, prevPosY]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == mousePosX - prevPosX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == mousePosY - prevPosY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.active);
                    TEST_VERIFY(actionState.x == mousePosX - prevPosX);
                    TEST_VERIFY(actionState.y == mousePosY - prevPosY);
                };
                chain.emplace(prepare, verify);
            }

            float32 deltaX = 15.0f;
            float32 deltaY = 25.0f;

            Window* window = GetPrimaryWindow();
            // Turn on pinning mode
            {
                TestFn prepare = [this, window]() {
                    window->SetCursorCapture(eCursorCapture::PINNING);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }

            // Handle input activation
            {
                TestFn prepare = [this, mouse]() {
                    SendMouseButtonUp(mouse, eInputElements::MOUSE_LBUTTON);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }

            // Pinning mode test
            {
                TestFn prepare = [this, mouse, deltaX, deltaY]() {
                    SendMouseMove(mouse, deltaX, deltaY, true);
                };
                TestFn verify = [this, mouse, deltaX, deltaY]() {
                    TEST_VERIFY(triggeredActionsCounter == 5);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == deltaX);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == deltaY);

                    AnalogActionState actionState = GetEngineContext()->actionSystem->GetAnalogActionState(ACTION_1);
                    TEST_VERIFY(actionState.active);
                    TEST_VERIFY(actionState.x == deltaX);
                    TEST_VERIFY(actionState.y == deltaY);
                };
                chain.emplace(prepare, verify);
            }

            // Reset pinning mode
            {
                TestFn prepare = [this, window]() {
                    window->SetCursorCapture(eCursorCapture::OFF);
                };
                TestFn verify;
                chain.emplace(prepare, verify);
            }
        }

        // Cleanup
        TestFn prepare = [this]() {
            GetEngineContext()->actionSystem->UnbindAllSets();
            GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
            GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
            ResetTriggeredActionsInfo();
        };
        TestFn verify;
        chain.emplace(prepare, verify);
    }

    DAVA_TEST (ActionSystemActionTriggerDigitalAnalogStateTest)
    {
        // Check that digital binding correctly outputs overrided analog state

        // Prepare test chain. We should send events on current update and check them on the next one
        // Prepare - Verify chain handled in the 'Update' method.
        const String testName = "ActionSystemActionTriggerDigitalAnalogStateTest";
        std::queue<PrepareVerify>& chain = testsChains[testName];

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping ActionSystemActionTriggerDigitalAnalogStateTest since there is no keyboard device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        ActionSystem* actionSystem = GetEngineContext()->actionSystem;
        actionSystem->ActionTriggered.Connect(this, &ActionSystemTest::OnActionTriggered);

        // Bind test set

        ActionSet set;

        const AnalogElementState overridenAnalogState(14.3f, -3.04f, 12.12f);

        // W, triggered continuously, analog data overriden
        DigitalBinding action1;
        action1.actionId = ACTION_1;
        action1.digitalElements[0] = eInputElements::KB_W;
        action1.digitalStates[0] = DigitalElementState::Pressed();
        action1.outputAnalogState = overridenAnalogState;
        set.digitalBindings.push_back(action1);

        actionSystem->BindSet(set, kb->GetId());

        // Check action1 (W Pressed)
        {
            // Press W, action1 should be triggered once, analog data should be equal to outputAnalogState
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, overridenAnalogState]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_1);
                    TEST_VERIFY(lastTriggeredAction->analogState.x == overridenAnalogState.x);
                    TEST_VERIFY(lastTriggeredAction->analogState.y == overridenAnalogState.y);
                    TEST_VERIFY(lastTriggeredAction->analogState.z == overridenAnalogState.z);
                };
                chain.emplace(prepare, verify);
            }

            // Release W, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Cleanup
        TestFn prepare = [this]() {
            GetEngineContext()->actionSystem->UnbindAllSets();
            GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
            GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
        };
        TestFn verify;
        chain.emplace(prepare, verify);
    }

    DAVA_TEST (ActionSystemComplexBindingsTest)
    {
        // Prepare test chain. We should send events on current update and check them on the next one
        // Prepare - Verify chain handled in the 'Update' method.
        const String testName = "ActionSystemComplexBindingsTest";
        std::queue<PrepareVerify>& chain = testsChains[testName];

        Keyboard* kb = GetEngineContext()->deviceManager->GetKeyboard();
        if (kb == nullptr)
        {
            Logger::Info("Skipping ActionSystemComplexBindingsTest since there is no keyboard device");
            return;
        }

        GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ALWAYS);

        GetEngineContext()->actionSystem->ActionTriggered.Connect(this, &ActionSystemTest::OnActionTriggered);

        // Bind test set

        ActionSet set;

        // W, triggered continuously
        DigitalBinding action1;
        action1.actionId = ACTION_1;
        action1.digitalElements[0] = eInputElements::KB_W;
        action1.digitalStates[0] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action1);

        // X, triggered continuously
        DigitalBinding action2;
        action2.actionId = ACTION_2;
        action2.digitalElements[0] = eInputElements::KB_X;
        action2.digitalStates[0] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action2);

        // Space, W triggered once
        DigitalBinding action3;
        action3.actionId = ACTION_3;
        action3.digitalElements[0] = eInputElements::KB_SPACE;
        action3.digitalStates[0] = DigitalElementState::JustPressed();
        action3.digitalElements[1] = eInputElements::KB_W;
        action3.digitalStates[1] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action3);

        // Space, W, X triggered once
        DigitalBinding action4;
        action4.actionId = ACTION_4;
        action4.digitalElements[0] = eInputElements::KB_SPACE;
        action4.digitalStates[0] = DigitalElementState::JustPressed();
        action4.digitalElements[1] = eInputElements::KB_W;
        action4.digitalStates[1] = DigitalElementState::Pressed();
        action4.digitalElements[2] = eInputElements::KB_X;
        action4.digitalStates[2] = DigitalElementState::Pressed();
        set.digitalBindings.push_back(action4);

        GetEngineContext()->actionSystem->BindSet(set, kb->GetId());

        Vector<FastName> actions = { ACTION_1, ACTION_2, ACTION_3, ACTION_4 };

        {
            // W, SPACE, X are just pressed, action4 should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyDown(kb, eInputElements::KB_W);
                    SendKeyboardKeyDown(kb, eInputElements::KB_SPACE);
                    SendKeyboardKeyDown(kb, eInputElements::KB_X);
                };
                TestFn verify = [this, set, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 1);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_4);

                    VerifyActiveDigitalActions({ ACTION_4 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Do nothing, action1 and action2 should be triggered
            {
                TestFn prepare = [this]() {
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 3);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_1 || lastTriggeredAction->actionId == ACTION_2);

                    VerifyActiveDigitalActions({ ACTION_1, ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release W, action2 should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_W);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);
                    TEST_VERIFY(lastTriggeredAction != nullptr);
                    TEST_VERIFY(lastTriggeredAction->actionId == ACTION_2);

                    VerifyActiveDigitalActions({ ACTION_2 }, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release X, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_X);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Release Space, no action should be triggered
            {
                TestFn prepare = [this, kb]() {
                    SendKeyboardKeyUp(kb, eInputElements::KB_SPACE);
                };
                TestFn verify = [this, actions]() {
                    TEST_VERIFY(triggeredActionsCounter == 4);

                    VerifyActiveDigitalActions({}, actions);
                };
                chain.emplace(prepare, verify);
            }

            // Reset triggered info
            TestFn prepare = [this]() {
                ResetTriggeredActionsInfo();
            };
            TestFn verify;
            chain.emplace(prepare, verify);
        }

        // Cleanup
        TestFn prepare = [this]() {
            GetEngineContext()->actionSystem->UnbindAllSets();
            GetEngineContext()->actionSystem->ActionTriggered.Disconnect(this);
            GetPrimaryWindow()->SetInputHandlingMode(eInputHandlingModes::HANDLE_ONLY_WHEN_FOCUSED);
        };
        TestFn verify;
        chain.emplace(prepare, verify);
    }

    void SendKeyboardKeyDown(Keyboard * kb, eInputElements key)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(primaryWindow, MainDispatcherEvent::KEY_DOWN, kb->GetKeyNativeScancode(key), kb->GetKeyNativeScancode(key), DAVA::eModifierKeys::NONE, false));
    }

    void SendKeyboardKeyUp(Keyboard * kb, eInputElements key)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(primaryWindow, MainDispatcherEvent::KEY_UP, kb->GetKeyNativeScancode(key), kb->GetKeyNativeScancode(key), DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseButtonDown(Mouse * mouse, eInputElements button)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        AnalogElementState pos = mouse->GetPosition();

        dispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_DOWN, static_cast<eMouseButtons>(button - eInputElements::MOUSE_FIRST_BUTTON + 1), pos.x, pos.y, 0, DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseButtonUp(Mouse * mouse, eInputElements button)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        AnalogElementState pos = mouse->GetPosition();

        dispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseClickEvent(primaryWindow, MainDispatcherEvent::MOUSE_BUTTON_UP, static_cast<eMouseButtons>(button - eInputElements::MOUSE_FIRST_BUTTON + 1), pos.x, pos.y, 0, DAVA::eModifierKeys::NONE, false));
    }

    void SendMouseMove(Mouse * mouse, float toX, float toY, bool relative = false)
    {
        using namespace DAVA::Private;

        Window* primaryWindow = GetPrimaryWindow();
        MainDispatcher* dispatcher = EngineBackend::Instance()->GetDispatcher();

        dispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(primaryWindow, toX, toY, eModifierKeys::NONE, relative));
    }

private:
    std::shared_ptr<Action> lastTriggeredAction;
    int triggeredActionsCounter = 0;

    using TestFn = std::function<void()>;
    struct PrepareVerify
    {
        TestFn prepare;
        TestFn verify;

        PrepareVerify(TestFn prepare, TestFn verify)
            : prepare(prepare)
            , verify(verify)
        {
        }
    };

    UnorderedMap<String, bool> testFinishedStatus = UnorderedMap<String, bool>{
        { "ActionSystemActionTriggerDigitalTest", false },
        { "ActionSystemActionTriggerAnalogTest", false },
        { "ActionSystemActionTriggerDigitalAnalogStateTest", false },
        { "ActionSystemComplexBindingsTest", false },
        { "ActionSystemActionTriggerAnalogRelativeTest", false }
    };

    UnorderedMap<String, std::queue<PrepareVerify>> testsChains;

    const FastName ACTION_1 = FastName("Action 1");
    const FastName ACTION_2 = FastName("Action 2");
    const FastName ACTION_3 = FastName("Action 3");
    const FastName ACTION_4 = FastName("Action 4");
    const FastName ACTION_5 = FastName("Action 5");
};
