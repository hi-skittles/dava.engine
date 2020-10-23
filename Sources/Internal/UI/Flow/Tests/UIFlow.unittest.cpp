#include "UnitTests/UnitTests.h"

#include "Base/FastName.h"
#include "Base/RefPtr.h"
#include "Concurrency/Thread.h"
#include "Engine/EngineContext.h"
#include "FileSystem/KeyedArchive.h"
#include "Job/JobManager.h"
#include "Reflection/Reflection.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/DataBinding/UIDataSourceComponent.h"
#include "UI/DefaultUIPackageBuilder.h"
#include "UI/Events/UIEventsSingleComponent.h"
#include "UI/Events/UIEventsSystem.h"
#include "UI/Flow/Private/UIFlowTransitionTransaction.h"
#include "UI/Flow/Services/UIFlowSystemService.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/Flow/UIFlowController.h"
#include "UI/Flow/UIFlowControllerComponent.h"
#include "UI/Flow/UIFlowControllerSystem.h"
#include "UI/Flow/UIFlowStateComponent.h"
#include "UI/Flow/UIFlowStateSystem.h"
#include "UI/Flow/UIFlowTransitionComponent.h"
#include "UI/Flow/UIFlowTransitionEffectConfig.h"
#include "UI/Flow/UIFlowViewComponent.h"
#include "UI/Flow/UIFlowViewSystem.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIGeometricData.h"
#include "UI/UIPackage.h"
#include "UI/UIPackageLoader.h"
#include "UI/UIScreen.h"

using namespace DAVA;

class DemoFlowController : public UIFlowController
{
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DemoFlowController, UIFlowController)
    {
        ReflectionRegistrator<DemoFlowController>::Begin()
        .ConstructorByPointer()
        .DestructorByPointer([](DemoFlowController* c) { SafeDelete(c); })
        .End();
    }

public:
    uint32 testStep = 0;
    bool first = true;

    void Init(UIFlowContext* context) override
    {
        UIFlowController::Init(context);
        testStep = context->GetData()->GetInt32("testStep");
        testStep++;
    }

    void LoadResources(UIFlowContext* context, UIControl* view) override
    {
        UIFlowController::LoadResources(context, view);
        testStep++;
    }

    void Activate(UIFlowContext* context, UIControl* view) override
    {
        UIFlowController::Activate(context, view);
        testStep++;
    }

    void Process(float32 elapsedTime) override
    {
        UIFlowController::Process(elapsedTime);
        if (first)
        {
            testStep++;
            first = false;
        }
    }

    bool ProcessEvent(const FastName& eventName, const Vector<Any>& params = Vector<Any>()) override
    {
        static const FastName incA("INC_A");
        if (eventName == incA)
        {
            testStep++;
            return true;
        }
        return UIFlowController::ProcessEvent(eventName, params);
    }

    void Deactivate(UIFlowContext* context, UIControl* view) override
    {
        testStep++;
        UIFlowController::Deactivate(context, view);
    }

    void UnloadResources(UIFlowContext* context, UIControl* view) override
    {
        testStep++;
        UIFlowController::UnloadResources(context, view);
    }

    void Release(UIFlowContext* context) override
    {
        testStep++;
        context->GetData()->SetInt32("testStep", testStep);
        UIFlowController::Release(context);
    }
};

DAVA_TESTCLASS (UIFlowTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UIFlowContext.cpp")
    DECLARE_COVERED_FILES("UIFlowController.cpp")
    DECLARE_COVERED_FILES("UIFlowControllerComponent.cpp")
    DECLARE_COVERED_FILES("UIFlowControllerSystem.cpp")
    DECLARE_COVERED_FILES("UIFlowLuaController.cpp")
    DECLARE_COVERED_FILES("UIFlowService.cpp")
    DECLARE_COVERED_FILES("UIFlowStateComponent.cpp")
    DECLARE_COVERED_FILES("UIFlowStateSystem.cpp")
    DECLARE_COVERED_FILES("UIFlowTransitionComponent.cpp")
    DECLARE_COVERED_FILES("UIFlowTransitionTransaction.cpp")
    DECLARE_COVERED_FILES("UIFlowUtils.cpp")
    DECLARE_COVERED_FILES("UIFlowViewComponent.cpp")
    DECLARE_COVERED_FILES("UIFlowViewSystem.cpp")
    //TODO: Uncomment after we start test render in unit tests
    //DECLARE_COVERED_FILES("UIFlowTransitionAnimationSystem.cpp")
    //DECLARE_COVERED_FILES("UIFlowTransitionEffect.cpp")
    //DECLARE_COVERED_FILES("UIFlowTransitionEffectConfig.cpp")
    END_FILES_COVERED_BY_TESTS();

    static const int32 CONTROLLER_STEPS_COUNT = 8;

    RefPtr<UIControl> flowProto;
    RefPtr<UIControl> uiRoot;
    UIControlSystem* controlSys = nullptr;
    UIFlowStateSystem* stateSys = nullptr;
    UIEventsSingleComponent* eventComp = nullptr;

    UIFlowTest()
    {
        DAVA_REFLECTION_REGISTER_PERMANENT_NAME(DemoFlowController);

        controlSys = GetEngineContext()->uiControlSystem;

        RefPtr<UIScreen> screen(new UIScreen());
        controlSys->SetScreen(screen.Get());
        controlSys->Update();

        {
            DAVA::DefaultUIPackageBuilder pkgBuilder;
            DAVA::UIPackageLoader().LoadPackage("~res:/UI/Flow/Flow.yaml", &pkgBuilder);
            flowProto = pkgBuilder.GetPackage()->GetControls().at(0);
        }

        {
            DAVA::DefaultUIPackageBuilder pkgBuilder;
            DAVA::UIPackageLoader().LoadPackage("~res:/UI/Flow/Screen.yaml", &pkgBuilder);
            uiRoot = pkgBuilder.GetPackage()->GetControls().at(0);
        }

        stateSys = controlSys->GetSystem<UIFlowStateSystem>();
        eventComp = controlSys->GetSingleComponent<UIEventsSingleComponent>();
    }

    ~UIFlowTest()
    {
        controlSys->Reset();
    }

    void SystemsUpdate(float32 time = 0.f)
    {
        if (time > 0.f)
        {
            controlSys->UpdateWithCustomTime(time);
        }
        else
        {
            controlSys->Update();
        }
    }

    void WaitBackgroundTransactions()
    {
        while (stateSys->HasTransitions())
        {
            GetEngineContext()->jobManager->Update();
            Thread::Sleep(10);
        }
    }

    void SendEvent(const String& e)
    {
        eventComp->SendEvent(controlSys->GetScreen(), FastName(e), Any());
    }

    bool HasControl(const String& path)
    {
        return controlSys->GetScreen()->FindByPath(path) != nullptr;
    }

    void SetUp(const String& testName) override
    {
        stateSys->SetFlowRoot(flowProto->SafeClone().Get());
        controlSys->GetScreen()->AddControl(uiRoot->SafeClone().Get());
    }

    void TearDown(const String& testName) override
    {
        stateSys->SetFlowRoot(nullptr);
        stateSys->GetContext()->GetData()->DeleteAllKeys();
        controlSys->GetScreen()->RemoveAllControls();
    }

    DAVA_TEST (SyncActivation)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state);
        TEST_VERIFY(HasControl("**/View1"));
        TEST_VERIFY(stateSys->IsStateActive(stateSys->FindStateByPath("^")));
        TEST_VERIFY(stateSys->IsStateActive(stateSys->FindStateByPath("^/BasicGroup")));
        TEST_VERIFY(stateSys->IsStateActive(stateSys->FindStateByPath("^/BasicGroup/State1")));
    }

    DAVA_TEST (AsyncActivation)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, true);

        UIFlowTransitionTransaction* transaction = stateSys->GetTopTransitionTransaction();
        TEST_VERIFY(transaction != nullptr);
        TEST_VERIFY(transaction->IsBackgroundLoading());

        SystemsUpdate();
        WaitBackgroundTransactions();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state);
        TEST_VERIFY(HasControl("**/View1"));
    }

    DAVA_TEST (SyncSwitchingState)
    {
        UIFlowStateComponent* state1 = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state1 != nullptr);
        stateSys->ActivateState(state1, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);
        TEST_VERIFY(HasControl("**/View1"));

        UIFlowStateComponent* state2 = stateSys->FindStateByPath("^/**/State2");
        TEST_VERIFY(state2 != nullptr);
        stateSys->ActivateState(state2, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state2);
        TEST_VERIFY(HasControl("**/View2"));

        TEST_VERIFY(!HasControl("**/View1"));
        TEST_VERIFY(stateSys->IsStateActive(state2));
        TEST_VERIFY(!stateSys->IsStateActive(state1));
    }

    DAVA_TEST (AsyncSwitchingState)
    {
        UIFlowStateComponent* state1 = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state1 != nullptr);
        stateSys->ActivateState(state1, true);
        SystemsUpdate();
        WaitBackgroundTransactions();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);
        TEST_VERIFY(HasControl("**/View1"));

        UIFlowStateComponent* state2 = stateSys->FindStateByPath("^/**/State2");
        TEST_VERIFY(state2 != nullptr);
        stateSys->ActivateState(state2, true);
        SystemsUpdate();
        WaitBackgroundTransactions();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state2);
        TEST_VERIFY(HasControl("**/View2"));

        TEST_VERIFY(!HasControl("**/View1"));
        TEST_VERIFY(stateSys->IsStateActive(state2));
        TEST_VERIFY(!stateSys->IsStateActive(state1));
    }

    DAVA_TEST (HistoryBack)
    {
        UIFlowStateComponent* state1 = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state1 != nullptr);
        stateSys->ActivateState(state1, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);

        UIFlowStateComponent* state2 = stateSys->FindStateByPath("^/**/State2");
        TEST_VERIFY(state2 != nullptr);
        stateSys->ActivateState(state2, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state2);

        // By system method
        stateSys->HistoryBack();
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);

        stateSys->ActivateState(state2, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state2);

        // By event
        SendEvent("BACK");
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);
    }

    DAVA_TEST (HistorySiblings)
    {
        UIFlowStateComponent* state1 = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state1 != nullptr);
        stateSys->ActivateState(state1, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);

        UIFlowStateComponent* sibling1 = stateSys->FindStateByPath("^/**/Sibling1");
        TEST_VERIFY(sibling1 != nullptr);
        stateSys->ActivateState(sibling1, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == sibling1);

        UIFlowStateComponent* sibling2 = stateSys->FindStateByPath("^/**/Sibling2");
        TEST_VERIFY(sibling2 != nullptr);
        stateSys->ActivateState(sibling2, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == sibling2);

        stateSys->HistoryBack();
        SystemsUpdate();

        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);
    }

    DAVA_TEST (HistoryTop)
    {
        UIFlowStateComponent* state1 = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state1 != nullptr);
        stateSys->ActivateState(state1, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);

        // Back from OnlyTop state
        UIFlowStateComponent* stateTop = stateSys->FindStateByPath("^/**/StateTop");
        TEST_VERIFY(stateTop != nullptr);
        stateSys->ActivateState(stateTop, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateTop);

        stateSys->HistoryBack();
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);

        // Switch OnlyTop state and go back
        stateSys->ActivateState(stateTop, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateTop);

        UIFlowStateComponent* state2 = stateSys->FindStateByPath("^/**/State2");
        TEST_VERIFY(state2 != nullptr);
        stateSys->ActivateState(state2, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state2);

        stateSys->HistoryBack();
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state1);
    }

    DAVA_TEST (FlowService)
    {
        static const FastName serviceName("flow");

        UIFlowContext* context = stateSys->GetContext();
        TEST_VERIFY(context != nullptr);

        UIFlowStateComponent* state = stateSys->FindStateByPath(".");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();

        TEST_VERIFY(context->GetService<UIFlowSystemService>(serviceName) != nullptr);

        stateSys->DeactivateAllStates();
        SystemsUpdate();

        TEST_VERIFY(context->GetService<UIFlowSystemService>(serviceName) == nullptr);
    }

    DAVA_TEST (LuaController)
    {
        UIFlowContext* context = stateSys->GetContext();
        TEST_VERIFY(context != nullptr);
        KeyedArchive* data = context->GetData();
        TEST_VERIFY(data != nullptr);

        data->SetInt32("testStep", 0);

        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/LuaCtrlTest");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();

        KeyedArchive* model = data->GetArchive("model");
        TEST_VERIFY(model != nullptr);
        TEST_VERIFY(model->GetInt32("a") == 1);

        SendEvent("INC_A");
        SystemsUpdate();
        TEST_VERIFY(model->GetInt32("a") == 2);

        stateSys->DeactivateAllStates();
        SystemsUpdate();

        model = data->GetArchive("model");
        TEST_VERIFY(model->Count() == 0);

        TEST_VERIFY(data->GetInt32("testStep") == CONTROLLER_STEPS_COUNT);
    }

    DAVA_TEST (NativeController)
    {
        UIFlowContext* context = stateSys->GetContext();
        TEST_VERIFY(context != nullptr);
        KeyedArchive* data = context->GetData();
        TEST_VERIFY(data != nullptr);

        data->SetInt32("testStep", 0);

        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/NativeCtrlTest");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();

        SendEvent("INC_A");
        SystemsUpdate();

        stateSys->DeactivateAllStates();
        SystemsUpdate();

        TEST_VERIFY(data->GetInt32("testStep") == CONTROLLER_STEPS_COUNT);
    }

    DAVA_TEST (MultipleStates)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath(".");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("./DefaultState"));

        state = stateSys->FindStateByPath("^/SubState");
        TEST_VERIFY(state != nullptr);

        stateSys->PreloadState(state, false);

        UIFlowTransitionTransaction* transaction = stateSys->GetTopTransitionTransaction();
        TEST_VERIFY(transaction != nullptr);
        TEST_VERIFY(transaction->IsOnlyLoad());

        SystemsUpdate();
        TEST_VERIFY(stateSys->IsStateLoaded(state));

        stateSys->ActivateState(state, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->IsStateActive(state));

        stateSys->DeactivateState(state, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->IsStateActive(state) == false);
    }

    DAVA_TEST (TransitionsByEvents)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath(".");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("./DefaultState"));

        SendEvent("STATE1");
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("^/**/State1"));

        SendEvent("STATE2");
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("^/**/State2"));

        SendEvent("BASIC_GROUP");
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("^/**/State1"));

        SendEvent("STATE2_BG");
        SystemsUpdate();
        WaitBackgroundTransactions();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("^/**/State2"));

        SendEvent("FWD_STATE1");
        SystemsUpdate();
        SystemsUpdate();
        TEST_VERIFY(stateSys->GetCurrentSingleState() == stateSys->FindStateByPath("^/**/State1"));

        SendEvent("SUBSTATE");
        SystemsUpdate();
        TEST_VERIFY(stateSys->IsStateActive(stateSys->FindStateByPath("^/SubState")));

        SendEvent("SUBSTATE_OFF");
        SystemsUpdate();
        TEST_VERIFY(!stateSys->IsStateActive(stateSys->FindStateByPath("^/SubState")));
    }

    DAVA_TEST (ControllerComponent)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);

        UIControl* stateControl = state->GetControl();
        UIFlowControllerComponent* ctrlCom = stateControl->GetOrCreateComponent<UIFlowControllerComponent>();
        ctrlCom->SetReflectionTypeName("SOME_WRONG_CONTROLLER_TYPE_NAME");
        ctrlCom->SetLuaScriptPath("~res:/SOME_WRONG_LUA_CONTROLLER_FILE_PATH.lua");

        stateSys->ActivateState(state, false);
        SystemsUpdate();

        UIFlowControllerSystem* ctrlSys = controlSys->GetSystem<UIFlowControllerSystem>();
        TEST_VERIFY(ctrlSys->GetController(ctrlCom) == nullptr);

        stateControl->RemoveComponent<UIFlowControllerComponent>();
    }

    DAVA_TEST (ViewComponent)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);

        UIControl* stateControl = state->GetControl();
        UIFlowViewComponent* viewCom = stateControl->GetOrCreateComponent<UIFlowViewComponent>();
        viewCom->SetViewYaml("~res:/SOME_WRONG_VIEW_YAML_FILE_PATH.yaml");
        viewCom->SetControlName("WRONG_CONTROL_NAME");
        viewCom->SetContainerPath("^/WRONG_CONTAINER_PATH");

        stateSys->ActivateState(state, false);
        SystemsUpdate();

        UIFlowViewSystem* viewSys = controlSys->GetSystem<UIFlowViewSystem>();
        TEST_VERIFY(viewSys->GetLinkedView(viewCom) == nullptr);

        stateControl->RemoveComponent<UIFlowViewComponent>();
    }

    DAVA_TEST (TransitionsComponent)
    {
        static const String TRANSITION_STR("EVENT,Activate,^;EVENT2,SendEvent,EVENT;");

        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);

        UIControl* stateControl = state->GetControl();
        UIFlowTransitionComponent* transCom = stateControl->GetOrCreateComponent<UIFlowTransitionComponent>();
        transCom->SetTransitionMapFromString(TRANSITION_STR);

        stateSys->ActivateState(state, false);
        SystemsUpdate();

        const UIFlowTransitionComponent::TransitionMap& transMap = transCom->GetTransitionMap();
        TEST_VERIFY(transMap.size() == 2);
        TEST_VERIFY(transMap[0].event == FastName("EVENT"));
        TEST_VERIFY(transMap[0].operation == UIFlowTransitionComponent::ACTIVATE_STATE);
        TEST_VERIFY(transMap[0].statePath == "^");
        TEST_VERIFY(transMap[1].event == FastName("EVENT2"));
        TEST_VERIFY(transMap[1].operation == UIFlowTransitionComponent::SEND_EVENT);
        TEST_VERIFY(transMap[1].sendEvent == FastName("EVENT"));
        transCom->SetTransitionMap(transMap);

        TEST_VERIFY(transCom->GetTransitionMapAsString() == TRANSITION_STR);
    }

    DAVA_TEST (StateComponent)
    {
        static const String EVENTS_STR("EVENT1;EVENT2;");
        static const String SERVICES_STR("flow,UIFlowSystemService;wrong,WRONG_SERVICE;");

        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);

        state->SetActivateEventsFromString(EVENTS_STR);
        state->SetDeactivateEventsFromString(EVENTS_STR);
        state->SetServicesFromString(SERVICES_STR);

        stateSys->ActivateState(state, false);
        SystemsUpdate();

        const Vector<FastName>& activateEvents = state->GetActivateEvents();
        TEST_VERIFY(activateEvents.size() == 2);
        TEST_VERIFY(activateEvents[0] == FastName("EVENT1"));
        TEST_VERIFY(activateEvents[1] == FastName("EVENT2"));
        state->SetActivateEvents(activateEvents);
        TEST_VERIFY(state->GetActivateEventsAsString() == EVENTS_STR);

        const Vector<FastName>& deactivateEvents = state->GetDeactivateEvents();
        TEST_VERIFY(deactivateEvents.size() == 2);
        TEST_VERIFY(deactivateEvents[0] == FastName("EVENT1"));
        TEST_VERIFY(deactivateEvents[1] == FastName("EVENT2"));
        state->SetDeactivateEvents(deactivateEvents);
        TEST_VERIFY(state->GetDeactivateEventsAsString() == EVENTS_STR);

        const Vector<UIFlowStateComponent::ServiceDescriptor>& services = state->GetServices();
        TEST_VERIFY(services.size() == 2);
        TEST_VERIFY(services[0].name == "flow");
        TEST_VERIFY(services[0].nativeType == "UIFlowSystemService");
        TEST_VERIFY(services[1].name == "wrong");
        TEST_VERIFY(services[1].nativeType == "WRONG_SERVICE");
        state->SetServices(services);
        TEST_VERIFY(state->GetServicesAsString() == SERVICES_STR);
    }

    DAVA_TEST (DataBinding)
    {
        UIFlowContext* context = stateSys->GetContext();
        TEST_VERIFY(context != nullptr);
        KeyedArchive* data = context->GetData();
        TEST_VERIFY(data != nullptr);

        RefPtr<KeyedArchive> model(new KeyedArchive());
        model->SetInt32("a", 1);
        data->SetArchive("model", model.Get());

        UIFlowStateComponent* state = stateSys->FindStateByPath("./**/State1");
        TEST_VERIFY(state != nullptr);
        UIControl* stateControl = state->GetControl();
        UIFlowViewComponent* viewCom = stateControl->GetComponent<UIFlowViewComponent>();
        TEST_VERIFY(viewCom != nullptr);

        viewCom->SetModelName("model");
        viewCom->SetModelScope("{b = 2}");

        stateSys->ActivateState(state, false);
        SystemsUpdate();

        UIControl* view = controlSys->GetScreen()->FindByPath("**/View1");
        UIDataSourceComponent* dataCom = view->GetComponent<UIDataSourceComponent>(0);
        TEST_VERIFY(dataCom != nullptr);
        TEST_VERIFY(dataCom->GetSourceType() == UIDataSourceComponent::FROM_REFLECTION);
        TEST_VERIFY(dataCom->GetData().IsValid());
        TEST_VERIFY(dataCom->GetData().GetField("a").IsValid());

        UIDataSourceComponent* scopeCom = view->GetComponent<UIDataSourceComponent>(1);
        TEST_VERIFY(scopeCom != nullptr);
        TEST_VERIFY(scopeCom->GetSourceType() == UIDataSourceComponent::FROM_EXPRESSION);
        TEST_VERIFY(scopeCom->GetSource() == "{b = 2}");
    }

    DAVA_TEST (TransitionEffects)
    {
        UIFlowStateComponent* state = stateSys->FindStateByPath("./DefaultState");
        TEST_VERIFY(state != nullptr);
        stateSys->ActivateState(state, false);

        UIFlowTransitionTransaction* transaction = stateSys->GetTopTransitionTransaction();
        TEST_VERIFY(transaction != nullptr);

        UIFlowTransitionEffectConfig effectCfg;
        transaction->SetEffectConfig(effectCfg);

        SystemsUpdate(); // Apply transaction
        SystemsUpdate(); // Apply transition effect
        TEST_VERIFY(stateSys->GetCurrentSingleState() == state);

        // TODO: Make test for rendering in separate application (TestBed)
        //         const float32 HALF_TIME = 0.051f;
        //         const Array<String, 8> STATES = {
        //             "./Effects/Static",
        //             "^/Effects/FadeAlpha",
        //             "^/Effects/Fade",
        //             "^/Effects/Scale",
        //             "^/Effects/Flip",
        //             "^/Effects/MoveLeft",
        //             "^/Effects/MoveRight",
        //             "^/Effects/Static"
        //         };
        //         UIFlowStateComponent* state = nullptr;
        //
        //         for (const String& statePath : STATES)
        //         {
        //             state = stateSys->FindStateByPath(statePath);
        //             TEST_VERIFY(state != nullptr);
        //             stateSys->ActivateState(state, false);
        //             SystemsUpdate(HALF_TIME);
        //             controlSys->Draw();
        //             SystemsUpdate(HALF_TIME);
        //             controlSys->Draw();
        //             TEST_VERIFY(stateSys->GetCurrentSingleState() == state);
        //         }
        //
        //         // Deactivate all states with transitions and make sure that transitions complete
        //         stateSys->DeactivateAllStates();
        //         SystemsUpdate(HALF_TIME);
        //         controlSys->Draw();
        //         TEST_VERIFY(stateSys->GetCurrentSingleState() == nullptr);
    }
};
