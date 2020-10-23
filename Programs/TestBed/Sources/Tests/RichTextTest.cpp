#include "Tests/RichTextTest.h"

#include "Infrastructure/TestBed.h"
#include <FileSystem/XMLParser.h>
#include <Logger/Logger.h>
#include <Utils/Utils.h>
#include <Utils/UTF8Utils.h>
#include <UI/RichContent/UIRichContentComponent.h>

using namespace DAVA;

class RichInputDelegate : public UITextFieldDelegate
{
public:
    RichInputDelegate(UIControl* richControl)
        : richControl(richControl)
    {
    }

    void TextFieldOnTextChanged(UITextField* /*textField*/, const WideString& newText, const WideString& /*oldText*/) override
    {
        UIRichContentComponent* rich = richControl->GetOrCreateComponent<UIRichContentComponent>();
        rich->SetText(UTF8Utils::EncodeToUTF8(newText));
    }

private:
    UIControl* richControl = nullptr;
};

RichTextTest::RichTextTest(TestBed& app)
    : BaseScreen(app, "RichTextTest")
{
}

void RichTextTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/RichTextTest.yaml", &pkgBuilder);
    UIControl* dialog = pkgBuilder.GetPackage()->GetControl("Root");
    AddControl(dialog);
    richText = dialog->FindByPath("RichText");
    inputField = dialog->FindByPath<UITextField*>("Input");

    inputDelegate = new RichInputDelegate(richText.Get());
    inputField->SetDelegate(inputDelegate);

    UIRichContentComponent* rich = richText->GetOrCreateComponent<UIRichContentComponent>();
    rich->SetBaseClasses("text");
    rich->SetText(inputField->GetUtf8Text());
}

void RichTextTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    inputField->SetDelegate(nullptr);
    SafeDelete(inputDelegate);
}
