#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <UI/DefaultUIPackageBuilder.h>
#include <UI/Text/UITextComponent.h>
#include <UI/Text/UITextSystem.h>
#include <UI/Text/Private/UITextSystemLink.h>
#include <UI/UIControlSystem.h>
#include <UI/UIPackageLoader.h>
#include <UI/UIScreen.h>
#include "Utils/UTF8Utils.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UITextSystemTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UITextComponent.cpp")
    DECLARE_COVERED_FILES("UITextSystem.cpp")
    DECLARE_COVERED_FILES("UITextSystemLink.cpp")
    END_FILES_COVERED_BY_TESTS();

    RefPtr<UIControl> newControl;

    UITextSystemTest()
    {
        RefPtr<UIScreen> screen(new UIScreen());
        GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
        GetEngineContext()->uiControlSystem->Update();

        newControl = new UIControl();

        screen->AddControl(newControl.Get());
    }

    ~UITextSystemTest()
    {
        GetEngineContext()->uiControlSystem->Reset();
    }

    void UpdateSystem()
    {
        UITextSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UITextSystem>();
        sys->Process(0.f);
    }

    bool IsEqual(const Color& a, const Color& b)
    {
        return (FLOAT_EQUAL(a.r, b.r) && FLOAT_EQUAL(a.g, b.g) && FLOAT_EQUAL(a.b, b.b) && FLOAT_EQUAL(a.a, b.a));
    }

    bool IsEqual(const Vector2& a, const Vector2& b)
    {
        return (FLOAT_EQUAL(a.x, b.x) && FLOAT_EQUAL(a.y, b.y));
    }

    DAVA_TEST (LoadYamlTest)
    {
        DefaultUIPackageBuilder pkgBuilder;
        bool loaded = UIPackageLoader().LoadPackage("~res:/UI/UITextTest.yaml", &pkgBuilder);
        DVASSERT(loaded);

        RefPtr<UIControl> control;
        control = pkgBuilder.GetPackage()->GetControl("NewText");
        TEST_VERIFY(control.Get() != nullptr);
    }

    DAVA_TEST (LoadLegacyYamlTest)
    {
        DefaultUIPackageBuilder pkgBuilder;
        bool loaded = UIPackageLoader().LoadPackage("~res:/UI/UIStaticTextLegacyTest.yaml", &pkgBuilder);
        DVASSERT(loaded);

        RefPtr<UIControl> control;

        control = pkgBuilder.GetPackage()->GetControl("UIStaticTextSample");
        TEST_VERIFY(control.Get() != nullptr);
        UITextComponent* text = control->GetComponent<UITextComponent>();
        DVASSERT(text);

        // Check converted propeties
        TEST_VERIFY(text->GetAlign() == (eAlign::ALIGN_HCENTER | eAlign::ALIGN_BOTTOM));
        TEST_VERIFY(text->GetText() == "Test text");
        TEST_VERIFY(text->GetFitting() == UITextComponent::eTextFitting::FITTING_FILL);
        TEST_VERIFY(text->GetFontName() == "Korinna_18");
        TEST_VERIFY(IsEqual(text->GetColor(), Color(1.f, 0.f, 0.f, 1.f)));
        TEST_VERIFY(text->GetMultiline() == UITextComponent::eTextMultiline::MULTILINE_ENABLED);
        TEST_VERIFY(text->GetColorInheritType() == UIControlBackground::eColorInheritType::COLOR_IGNORE_PARENT);
        TEST_VERIFY(IsEqual(text->GetShadowColor(), Color(0.f, 0.f, 1.f, 1.f)));
        TEST_VERIFY(IsEqual(text->GetShadowOffset(), Vector2(5.f, 5.f)));
        TEST_VERIFY(text->GetPerPixelAccuracyType() == UIControlBackground::ePerPixelAccuracyType::PER_PIXEL_ACCURACY_FORCED);
        TEST_VERIFY(text->GetUseRtlAlign() == TextBlock::eUseRtlAlign::RTL_USE_BY_CONTENT);
        TEST_VERIFY(text->IsForceBiDiSupportEnabled() == true);
    }

    DAVA_TEST (UpdateTest)
    {
        const String testData = R"(
        Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc elementum lectus quis mauris molestie, sed consectetur risus dictum. Ut mattis ornare magna, nec interdum nunc interdum nec. 
        )";
        UITextComponent* c = newControl->GetOrCreateComponent<UITextComponent>();
        DVASSERT(c);
        c->SetText(testData);

        TEST_VERIFY(c->IsModified() == true);

        UpdateSystem();

        TEST_VERIFY(c->IsModified() == false);
    }

    DAVA_TEST (AddRemoveTest)
    {
        RefPtr<UITextComponent> orig;
        orig = newControl->GetComponent<UITextComponent>();

        TEST_VERIFY(orig->GetLink());

        UpdateSystem();

        UITextSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UITextSystem>();

        RefPtr<UITextComponent> copy(orig->Clone());
        TEST_VERIFY(orig->GetLink() != nullptr);
        TEST_VERIFY(copy->GetLink() != nullptr);
        TEST_VERIFY(orig->GetLink() != copy->GetLink());

        newControl->RemoveComponent(orig.Get());
        TEST_VERIFY(orig->GetLink() != nullptr);
        TEST_VERIFY(copy->GetLink() != nullptr);

        UpdateSystem();

        newControl->AddComponent(copy.Get());
        TEST_VERIFY(orig->GetLink() != nullptr);
        TEST_VERIFY(copy->GetLink() != nullptr);
        TEST_VERIFY(orig->GetLink() != copy->GetLink());

        UpdateSystem();
    }

    DAVA_TEST (CloneTest)
    {
        RefPtr<UITextComponent> orig;
        orig = newControl->GetComponent<UITextComponent>();
        orig->SetAlign(eAlign::ALIGN_HCENTER | eAlign::ALIGN_VCENTER);
        orig->SetText("123");
        orig->SetFitting(UITextComponent::eTextFitting::FITTING_FILL);
        orig->SetFontName("Korinna_18");
        orig->SetColor(Color(1.f, 0.f, 0.f, 0.f));
        orig->SetMultiline(UITextComponent::eTextMultiline::MULTILINE_ENABLED);
        orig->SetColorInheritType(UIControlBackground::eColorInheritType::COLOR_REPLACE_ALPHA_ONLY);
        orig->SetShadowOffset(Vector2(2.f, 2.f));
        orig->SetShadowColor(Color(0.f, 1.f, 0.f, 0.f));
        orig->SetPerPixelAccuracyType(UIControlBackground::ePerPixelAccuracyType::PER_PIXEL_ACCURACY_FORCED);
        orig->SetUseRtlAlign(TextBlock::eUseRtlAlign::RTL_USE_BY_SYSTEM);
        orig->SetForceBiDiSupportEnabled(true);

        UpdateSystem();

        newControl->RemoveComponent(orig.Get());
        RefPtr<UITextComponent> copy(orig->Clone());
        newControl->AddComponent(copy.Get());

        TEST_VERIFY(orig->GetAlign() == copy->GetAlign());
        TEST_VERIFY(orig->GetText() == copy->GetText());
        TEST_VERIFY(orig->GetFitting() == copy->GetFitting());
        TEST_VERIFY(orig->GetFontName() == copy->GetFontName());
        TEST_VERIFY(orig->GetColor() == copy->GetColor());
        TEST_VERIFY(orig->GetMultiline() == copy->GetMultiline());
        TEST_VERIFY(orig->GetColorInheritType() == copy->GetColorInheritType());
        TEST_VERIFY(orig->GetShadowOffset() == copy->GetShadowOffset());
        TEST_VERIFY(orig->GetShadowColor() == copy->GetShadowColor());
        TEST_VERIFY(orig->GetPerPixelAccuracyType() == copy->GetPerPixelAccuracyType());
        TEST_VERIFY(orig->GetUseRtlAlign() == copy->GetUseRtlAlign());
        TEST_VERIFY(orig->IsForceBiDiSupportEnabled() == copy->IsForceBiDiSupportEnabled());

        copy->SetText("4321");
        TEST_VERIFY(orig->GetText() != copy->GetText());
    }

    DAVA_TEST (UpdateTextBlockTest)
    {
        const String str1 = "123";
        const String str2 = "54321";

        RefPtr<UITextComponent> orig;
        orig = newControl->GetComponent<UITextComponent>();
        orig->SetText(str1);

        newControl->RemoveComponent(orig.Get());
        RefPtr<UITextComponent> copy(orig->Clone());
        newControl->AddComponent(copy.Get());

        UpdateSystem();
        TEST_VERIFY(copy->GetLink()->GetTextBlock()->GetText() == UTF8Utils::EncodeToWideString(str1));

        copy->SetText(str2);
        TEST_VERIFY(copy->GetText() == str2);
        TEST_VERIFY(copy->GetLink()->GetTextBlock()->GetText() == UTF8Utils::EncodeToWideString(str1));
        TEST_VERIFY(orig->GetText() != copy->GetText());

        UpdateSystem();
        TEST_VERIFY(copy->GetLink()->GetTextBlock()->GetText() == UTF8Utils::EncodeToWideString(str2));
    }
};
