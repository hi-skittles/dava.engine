#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <UI/DefaultUIPackageBuilder.h>
#include <UI/UIPackageLoader.h>
#include <UI/UIControlSystem.h>
#include <UI/UIScreen.h>
#include <UI/RichContent/UIRichContentAliasesComponent.h>
#include <UI/RichContent/UIRichContentComponent.h>
#include <UI/RichContent/UIRichContentSystem.h>

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UIRichContentTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("RichStructs.cpp")
    DECLARE_COVERED_FILES("XMLAliasesBuilder.cpp")
    DECLARE_COVERED_FILES("XMLRichContentBuilder.cpp")
    DECLARE_COVERED_FILES("UIRichContentAliasesComponent.cpp")
    DECLARE_COVERED_FILES("UIRichContentComponent.cpp")
    DECLARE_COVERED_FILES("UIRichContentSystem.cpp")
    END_FILES_COVERED_BY_TESTS();

    RefPtr<UIControl> richControl;

    UIRichContentTest()
    {
        RefPtr<UIScreen> screen(new UIScreen());
        GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
        GetEngineContext()->uiControlSystem->Update();

        DefaultUIPackageBuilder pkgBuilder;
        UIPackageLoader().LoadPackage("~res:/UI/UIRichContentTest.yaml", &pkgBuilder);
        richControl = pkgBuilder.GetPackage()->GetControl("RichControl");

        screen->AddControl(richControl.Get());
    }

    ~UIRichContentTest()
    {
        GetEngineContext()->uiControlSystem->Reset();
    }

    void UpdateRichContentSystem()
    {
        UIRichContentSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIRichContentSystem>();
        sys->Process(0.f);
    }

    DAVA_TEST (BasicTest)
    {
        const String testData = R"(
            <p class="text">Header</p><span class="text">Simple text<br /> with </span><img src="~res:/UI/Images/GoldPin.png" /><span class="text">image.</span>
            <ul><li>Item 1</li><li>Item 2</li></ul>
            <object path="~res:/UI/UIRichContentTest.yaml" prototype="ProtoControl" />
            <object path="~res:/UI/UIRichContentTest.yaml" control="CustomControl" />
        )";
        UIRichContentComponent* c = richControl->GetOrCreateComponent<UIRichContentComponent>();
        DVASSERT(c);
        c->SetText(testData);

        UpdateRichContentSystem();
        TEST_VERIFY(richControl->GetChildren().size() == 12);
    }

    DAVA_TEST (BaseClassesTest)
    {
        const String testData = R"(
            Simple <span class="text-green">green</span> text
        )";
        UIRichContentComponent* c = richControl->GetOrCreateComponent<UIRichContentComponent>();
        DVASSERT(c);
        c->SetBaseClasses("text");
        c->SetText(testData);
        c->SetClassesInheritance(true);

        UpdateRichContentSystem();
        TEST_VERIFY(richControl->GetChildren().size() == 3);

        c->SetBaseClasses("");
        c->SetClassesInheritance(false);
    }

    DAVA_TEST (RichComponentTest)
    {
        RefPtr<UIRichContentComponent> orig;
        orig = richControl->GetComponent<UIRichContentComponent>();
        richControl->RemoveComponent(orig.Get());
        RefPtr<UIRichContentComponent> copy(orig->Clone());
        richControl->AddComponent(copy.Get());

        TEST_VERIFY(orig->GetText() == copy->GetText());
        TEST_VERIFY(orig->GetBaseClasses() == copy->GetBaseClasses());
    }

    DAVA_TEST (RichAliasesComponentTest)
    {
        const String testData = R"(
        <h1>Header</h1><text>Simple text<nl/> with </text><GoldPin/><text>image.</text>
        )";
        UIRichContentComponent* c = richControl->GetOrCreateComponent<UIRichContentComponent>();
        UIRichContentAliasesComponent* ca = richControl->GetOrCreateComponent<UIRichContentAliasesComponent>();
        DVASSERT(c);
        DVASSERT(ca);

        UIRichContentAliasesComponent::Aliases aliases;
        aliases.push_back({ "h1", "<p class=\"dejavu\" />" });
        aliases.push_back({ "test", "<span class=\"test\" />" });
        aliases.push_back({ "nl", "<br />" });
        aliases.push_back({ "GoldPin", "<img src=\"~res:/UI/Images/GoldPin.png\" />" });

        c->SetText(testData);
        ca->SetAliases(aliases);

        String aliasesAsString = ca->GetAliasesAsString();
        ca->SetAliasesFromString(aliasesAsString);
        TEST_VERIFY(ca->GetAliases() == aliases);

        UpdateRichContentSystem();
        TEST_VERIFY(richControl->GetChildren().size() == 6);

        // Remove aliases
        aliases.clear();

        ca->SetAliases(aliases);

        // Append, remove and clone new aliases compoent
        UIRichContentAliasesComponent* ca2 = new UIRichContentAliasesComponent(*ca);
        richControl->AddComponent(ca2);
        TEST_VERIFY(richControl->GetComponentCount<UIRichContentAliasesComponent>() == 2);
        richControl->RemoveComponent(ca2);
        TEST_VERIFY(richControl->GetComponentCount<UIRichContentAliasesComponent>() == 1);
    }
};
