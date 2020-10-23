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

#include <UI/Script/UIScriptSystem.h>
#include <UI/Script/UIScriptComponent.h>
#include <UI/Script/Private/UILuaScriptComponentController.h>

#include "UnitTests/UnitTests.h"

using namespace DAVA;

class UIDemoController : public UIScriptComponentController
{
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(UIDemoController, UIScriptComponentController)
    {
        ReflectionRegistrator<UIDemoController>::Begin()
        .ConstructorByPointer()
        .DestructorByPointer([](UIDemoController* c) { SafeDelete(c); })
        .End();
    }

    int count = 0;

public:
    UIDemoController() = default;
    ~UIDemoController() override = default;

    void Init(UIScriptComponent* component) override
    {
        component->GetControl()->GetOrCreateComponent<UITextComponent>()->SetText("TextFromCpp:init");
    }
    void Release(UIScriptComponent* component) override
    {
        component->GetControl()->GetOrCreateComponent<UITextComponent>()->SetText("TextFromCpp:release");
    }
    void ParametersChanged(UIScriptComponent* component) override
    {
        component->GetControl()->GetOrCreateComponent<UITextComponent>()->SetText("TextFromCpp:parametersChanged:" + component->GetParameters());
    }
    void Process(UIScriptComponent* component, float32 elapsedTime) override
    {
        if (count == 1)
        {
            component->GetControl()->GetOrCreateComponent<UITextComponent>()->SetText("TextFromCpp:process:1");
        }
        count++;
    }
    bool ProcessEvent(UIScriptComponent* component, const FastName& eventName, const Vector<Any>& params = Vector<Any>()) override
    {
        component->GetControl()->GetOrCreateComponent<UITextComponent>()->SetText(String("TextFromCpp:processEvent:") + eventName.c_str());
        return true;
    }
};

DAVA_TESTCLASS (UIScriptTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UIScriptSystem.cpp")
    DECLARE_COVERED_FILES("UIScriptComponent.cpp")
    DECLARE_COVERED_FILES("UILuaScriptComponentController.cpp")
    END_FILES_COVERED_BY_TESTS();

    RefPtr<UIScreen> screen;
    RefPtr<UIStaticText> text;

    UIScriptTest()
    {
        DAVA_REFLECTION_REGISTER_PERMANENT_NAME(UIDemoController);
        screen = MakeRef<UIScreen>();
        GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
        GetEngineContext()->uiControlSystem->Update();
    }

    ~UIScriptTest()
    {
        GetEngineContext()->uiControlSystem->Reset();
    }

    void SetUp(const String& testName) override
    {
        text = MakeRef<UIStaticText>();
        screen->AddControl(text.Get());
    }

    void TearDown(const String& testName) override
    {
        screen->RemoveControl(text.Get());
        text = nullptr;
    }

    DAVA_TEST (CornerCasesTest)
    {
        UIScriptSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIScriptSystem>();
        RefPtr<UIStaticText> dummyText = MakeRef<UIStaticText>();
        TEST_VERIFY(sys->ProcessEvent(dummyText.Get(), FastName("TestEvent"), Any()) == false);
        UIScriptComponent* scriptComp = dummyText->GetOrCreateComponent<UIScriptComponent>();
        screen->AddControl(dummyText.Get());
        TEST_VERIFY(sys->ProcessEvent(dummyText.Get(), FastName("TestEvent"), Any()) == false);
        screen->RemoveControl(dummyText.Get());

        RefPtr<UIScriptComponent> ref = MakeRef<UIScriptComponent>();
        TEST_VERIFY(sys->GetController(ref.Get()) == nullptr);
    }

    DAVA_TEST (PauseProcessingTest)
    {
        UIScriptSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIScriptSystem>();

        text->SetUtf8Text("");

        UIScriptComponent* scriptComp = text->GetOrCreateComponent<UIScriptComponent>();
        scriptComp->SetReflectionTypeName("UIDemoController");

        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:init");

        sys->SetPauseProcessing(true);
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:init");

        sys->SetPauseProcessing(false);
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:process:1");
    }

    DAVA_TEST (NativeTest)
    {
        UIScriptSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIScriptSystem>();

        text->SetUtf8Text("");

        UIScriptComponent* scriptComp = text->GetOrCreateComponent<UIScriptComponent>();
        scriptComp->SetReflectionTypeName("UIDemoController");

        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:init");

        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:process:1");

        scriptComp->SetParameters("qwe");
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:parametersChanged:qwe");

        TEST_VERIFY(sys->ProcessEvent(text.Get(), FastName("TestEvent"), Any()));
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:processEvent:TestEvent")
        TEST_VERIFY(sys->GetController(scriptComp) != nullptr);

        text->RemoveComponent<UIScriptComponent>(0);
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromCpp:release");
    }

    DAVA_TEST (LuaScriptTest)
    {
        UIScriptSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIScriptSystem>();

        text->SetUtf8Text("");

        UIScriptComponent* scriptComp = text->GetOrCreateComponent<UIScriptComponent>();
        scriptComp->SetLuaScriptPath("~res:/UI/Flow/Lua/TestComponent.lua");

        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromLua:init");

        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromLua:process:1");

        scriptComp->SetParameters("qwe");
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromLua:parametersChanged:qwe");

        TEST_VERIFY(sys->ProcessEvent(text.Get(), FastName("TestEvent"), Any()));
        TEST_VERIFY(text->GetUtf8Text() == "TextFromLua:processEvent:TestEvent")
        TEST_VERIFY(sys->GetController(scriptComp) != nullptr);

        text->RemoveComponent<UIScriptComponent>(0);
        sys->Process(0.0f);
        TEST_VERIFY(text->GetUtf8Text() == "TextFromLua:release");
    }

    DAVA_TEST (LuaScriptSwitchTest)
    {
        UIScriptSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIScriptSystem>();

        text->SetUtf8Text("");

        UIScriptComponent* scriptComp = text->GetOrCreateComponent<UIScriptComponent>();
        scriptComp->SetLuaScriptPath("~res:/UI/Flow/Lua/EmptyComponent.lua");

        // call not existing functions
        sys->Process(0.0f);
        scriptComp->SetParameters("qwe");
        sys->Process(0.0f);
        sys->ProcessEvent(text.Get(), FastName("TestEvent"), Any());

        // switch controller
        scriptComp->SetLuaScriptPath("~res:/UI/Flow/Lua/TestComponent.lua");
        sys->Process(0.0f);

        TEST_VERIFY(sys->GetController(scriptComp) != nullptr);
        scriptComp->SetReflectionTypeName("Invalid Name");
        sys->Process(0.0f);
        TEST_VERIFY(!scriptComp->GetLuaScriptPath().Exists());
        TEST_VERIFY(sys->GetController(scriptComp) == nullptr);

        // release
        text->RemoveComponent<UIScriptComponent>(0);
        sys->Process(0.0f);
    }

    DAVA_TEST (RemoveComponentTest)
    {
        text->GetOrCreateComponent<UIScriptComponent>();

        RefPtr<UIStaticText> text2 = MakeRef<UIStaticText>();
        screen->AddControl(text2.Get());
        text2->GetOrCreateComponent<UIScriptComponent>();

        text->RemoveComponent<UIScriptComponent>();
        text2->RemoveComponent<UIScriptComponent>();
    }
};
