#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <Base/RefPtrUtils.h>
#include <UI/DefaultUIPackageBuilder.h>
#include <UI/UIPackageLoader.h>
#include <UI/UIControlSystem.h>
#include <UI/UIScreen.h>
#include <UI/UIStaticText.h>
#include <UI/UITextField.h>
#include <UI/Text/UITextComponent.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectionRegistrator.h>

#include <UI/Events/UIEventsSystem.h>
#include <UI/Events/UIEventBindingComponent.h>
#include <UI/Events/UIInputEventComponent.h>
#include <UI/Events/UIShortcutEventComponent.h>
#include <UI/Events/UIEventsSingleComponent.h>
#include <UI/Events/UIMovieEventComponent.h>

#include "UnitTests/UnitTests.h"

using namespace DAVA;

namespace UIEventsTestDetails
{
const FastName COMMAND_SET_TEXT_NAME("SetText");
const FastName COMMAND_DISPATCH_EVENT_NAME("SendEvent");
const FastName COMMAND_BROADCAST_EVENT_NAME("SendBroadcastEvent");
}

DAVA_TESTCLASS (UIEventsTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UIEventsSystem.cpp")
    DECLARE_COVERED_FILES("UIEventsSingleComponent.cpp")
    DECLARE_COVERED_FILES("UIEventBindingComponent.cpp")
    DECLARE_COVERED_FILES("UIInputEventComponent.cpp")
    DECLARE_COVERED_FILES("UIMovieEventComponent.cpp")
    DECLARE_COVERED_FILES("UIShortcutEventComponent.cpp")
    DECLARE_COVERED_FILES("UIActionMap.cpp")
    DECLARE_COVERED_FILES("UIInputMap.cpp")
    END_FILES_COVERED_BY_TESTS();

    RefPtr<UIScreen> screen;
    RefPtr<UIStaticText> text;
    RefPtr<UIStaticText> childText;

    UIEventsTest()
    {
        using namespace UIEventsTestDetails;

        screen = MakeRef<UIScreen>();
        GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
        GetEngineContext()->uiControlSystem->Update();
    }

    ~UIEventsTest()
    {
        GetEngineContext()->uiControlSystem->Reset();
    }

    void SetUp(const String& testName) override
    {
        text = MakeRef<UIStaticText>();
        childText = MakeRef<UIStaticText>();
        screen->AddControl(text.Get());
        text->AddControl(childText.Get());
        text->SetName("text");
        childText->SetName("childText");
    }

    void TearDown(const String& testName) override
    {
        screen->RemoveControl(text.Get());
        text = nullptr;
        childText = nullptr;
    }

    DAVA_TEST (HelpersTest)
    {
        UIEventsSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIEventsSystem>();
        TEST_VERIFY(sys->GetTargetEntity(FastName("childText"), text.Get()) == childText.Get());
        TEST_VERIFY(sys->GetTargetEntity(UIEventsSystem::ACTION_COMPONENT_SELF_ENTITY_NAME, text.Get()) == text.Get());
        TEST_VERIFY(sys->GetTargetEntity(FastName("BAD_NAME"), nullptr) == nullptr);
        TEST_VERIFY(sys->GetTargetEntity(FastName("BAD_NAME"), text.Get()) == nullptr);
    }

    DAVA_TEST (EventBindingTest)
    {
        using namespace UIEventsTestDetails;

        UIEventsSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIEventsSystem>();
        UIEventsSingleComponent* eventsSingle = GetEngineContext()->uiControlSystem->GetSingleComponent<UIEventsSingleComponent>();

        RefPtr<UIStaticText> childStubText = MakeRef<UIStaticText>();
        childStubText->SetName("childText");
        text->AddControl(childStubText.Get());

        auto BROADCAST_TEST_EVENT = FastName("BROADCAST_TEST");
        auto DISPATCH_TEST_EVENT = FastName("DISPATCH_TEST");

        bool dispatchTest = false;
        bool broadcastTest = false;

        // Bind action
        auto actions = text->GetOrCreateComponent<UIEventBindingComponent>();
        actions->BindAction(DISPATCH_TEST_EVENT, [&](const Any& data) {
            dispatchTest = true;
        });

        auto actionsChild = childText->GetOrCreateComponent<UIEventBindingComponent>();
        actionsChild->BindAction(BROADCAST_TEST_EVENT, [&](const Any& data) {
            broadcastTest = true;
        });

        TEST_VERIFY(!dispatchTest);
        TEST_VERIFY(!broadcastTest);

        TEST_VERIFY(eventsSingle->SendEvent(childText.Get(), DISPATCH_TEST_EVENT, Any()));
        sys->Process(0.f);

        TEST_VERIFY(dispatchTest);
        TEST_VERIFY(!broadcastTest);

        TEST_VERIFY(eventsSingle->SendBroadcastEvent(text.Get(), BROADCAST_TEST_EVENT, Any()));
        sys->Process(0.f);

        TEST_VERIFY(dispatchTest);
        TEST_VERIFY(broadcastTest);

        // Unbind action
        dispatchTest = false;

        actions->UnbindAction(DISPATCH_TEST_EVENT);
        TEST_VERIFY(!dispatchTest);

        eventsSingle->SendEvent(childText.Get(), DISPATCH_TEST_EVENT, Any());
        sys->Process(0.f);

        TEST_VERIFY(!dispatchTest);

        auto clone = actions->Clone();
        TEST_VERIFY(clone != nullptr);
        clone->Release();

        FastName empty;
        TEST_VERIFY(!eventsSingle->SendEvent(childText.Get(), empty, Any()));
        TEST_VERIFY(!eventsSingle->SendBroadcastEvent(text.Get(), empty, Any()));
    }

    DAVA_TEST (ShortcutsTest)
    {
        UIEventsSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIEventsSystem>();

        // Component action bind
        KeyboardShortcut shortcutF1 = KeyboardShortcut::ParseFromString(String("F1"));
        KeyboardShortcut shortcutF2 = KeyboardShortcut::ParseFromString(String("F2"));
        KeyboardShortcut shortcutF3 = KeyboardShortcut::ParseFromString(String("F3"));
        auto TEST_EVENT = FastName("TEST");

        auto shortcuts = childText->GetOrCreateComponent<UIShortcutEventComponent>();
        shortcuts->BindShortcut(shortcutF1, TEST_EVENT);
        shortcuts->BindShortcut(shortcutF3, TEST_EVENT);

        TEST_VERIFY(shortcuts->GetInputMap().FindEvent(shortcutF1) == TEST_EVENT);
        TEST_VERIFY(shortcuts->GetInputMap().FindEvent(shortcutF3) == TEST_EVENT);

        auto copy = shortcuts->Clone();
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF1) == TEST_EVENT);
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF3) == TEST_EVENT);

        copy->BindShortcut(shortcutF2, TEST_EVENT);
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF2) == TEST_EVENT);

        copy->UnbindShortcut(shortcutF1);
        TEST_VERIFY(!copy->GetInputMap().FindEvent(shortcutF1).IsValid());
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF2) == TEST_EVENT);
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF3) == TEST_EVENT);

        copy->BindShortcut(shortcutF1, TEST_EVENT);
        TEST_VERIFY(copy->GetInputMap().FindEvent(shortcutF1) == TEST_EVENT);

        copy->UnbindEvent(FastName("UNKNOWN_EVENT"));
        copy->UnbindEvent(TEST_EVENT);
        TEST_VERIFY(!copy->GetInputMap().FindEvent(shortcutF1).IsValid());
        TEST_VERIFY(!copy->GetInputMap().FindEvent(shortcutF2).IsValid());
        TEST_VERIFY(!copy->GetInputMap().FindEvent(shortcutF3).IsValid());
        copy->Release();

        // Global action bind
        auto TEST_GLOBAL_F2_EVENT = FastName("TEST_GLOBAL_F2");

        int32 count = 0;
        sys->BindGlobalAction(TEST_GLOBAL_F2_EVENT, [&](const Any& data) {
            count++;
        });

        sys->BindGlobalShortcut(shortcutF2, TEST_GLOBAL_F2_EVENT);

        TEST_VERIFY(count == 0);
        sys->PerformGlobalShortcut(shortcutF2);
        TEST_VERIFY(count == 1);
    }

    DAVA_TEST (InputEventsTest)
    {
        UIEventsSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIEventsSystem>();
        auto actions = text->GetOrCreateComponent<UIEventBindingComponent>();

        Vector<int32> eventTypes = {
            UIControl::eEventType::EVENT_TOUCH_DOWN,
            UIControl::eEventType::EVENT_TOUCH_UP_INSIDE,
            UIControl::eEventType::EVENT_TOUCH_UP_OUTSIDE,
            UIControl::eEventType::EVENT_VALUE_CHANGED,
            UIControl::eEventType::EVENT_HOVERED_SET,
            UIControl::eEventType::EVENT_HOVERED_REMOVED,
        };
        Vector<bool> results;
        Vector<FastName> eventNames;

        for (size_t idx = 0; idx < eventTypes.size(); idx++)
        {
            FastName name(Format("TEST_EVENT_%u", idx));
            eventNames.push_back(name);
            results.push_back(false);

            actions->BindAction(name, [&results, idx](const Any& data) {
                results[idx] = true;
            });
        }

        auto input = childText->GetOrCreateComponent<UIInputEventComponent>();
        input->SetOnTouchDownEvent(eventNames[0]);
        input->SetOnTouchUpInsideEvent(eventNames[1]);
        input->SetOnTouchUpOutsideEvent(eventNames[2]);
        input->SetOnValueChangedEvent(eventNames[3]);
        input->SetOnHoverSetEvent(eventNames[4]);
        input->SetOnHoverRemovedEvent(eventNames[5]);

        for (size_t idx = 0; idx < eventTypes.size(); idx++)
        {
            TEST_VERIFY(!results[idx]);
            sys->ProcessControlEvent(eventTypes[idx], nullptr, childText.Get());
            sys->Process(0.f);
            TEST_VERIFY(results[idx]);
        }

        auto clone = input->Clone();
        TEST_VERIFY(input->GetOnTouchDownEvent() == clone->GetOnTouchDownEvent());
        TEST_VERIFY(input->GetOnTouchUpInsideEvent() == clone->GetOnTouchUpInsideEvent());
        TEST_VERIFY(input->GetOnTouchUpOutsideEvent() == clone->GetOnTouchUpOutsideEvent());
        TEST_VERIFY(input->GetOnValueChangedEvent() == clone->GetOnValueChangedEvent());
        TEST_VERIFY(input->GetOnHoverSetEvent() == clone->GetOnHoverSetEvent());
        TEST_VERIFY(input->GetOnHoverRemovedEvent() == clone->GetOnHoverRemovedEvent());
        clone->Release();
    }

    DAVA_TEST (MovieEventsTest)
    {
        UIEventsSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIEventsSystem>();
        auto movieEvents = childText->GetOrCreateComponent<UIMovieEventComponent>();
        movieEvents->SetStartEvent(FastName("START"));
        movieEvents->SetStopEvent(FastName("STOP"));

        auto clone = movieEvents->Clone();
        TEST_VERIFY(movieEvents->GetStartEvent() == clone->GetStartEvent());
        TEST_VERIFY(movieEvents->GetStopEvent() == clone->GetStopEvent());
        clone->Release();
    }
};
