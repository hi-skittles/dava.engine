#include "Tests/StaticTextTest.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

namespace
{
class InputDelegate : public UITextFieldDelegate
{
public:
    InputDelegate(StaticTextTest* _test)
        : test(_test)
    {
    }

    void TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText) override
    {
        test->SetPreviewText(newText);
    }

private:
    StaticTextTest* test;
};

static const Color RED = Color(1.f, 0.f, 0.f, 1.f);
static const Color GREEN = Color(0.f, 1.f, 0.f, 1.f);

struct ButtonInfo
{
    WideString caption;
    int32 tag;
    Rect rect;
};
}

static const ButtonInfo alignButtonsInfo[] = {
    { L"Top left", ALIGN_TOP | ALIGN_LEFT, Rect(450, 30, 100, 20) },
    { L"Top center", ALIGN_TOP | ALIGN_HCENTER, Rect(560, 30, 100, 20) },
    { L"Top right", ALIGN_TOP | ALIGN_RIGHT, Rect(670, 30, 100, 20) },
    { L"Top justify", ALIGN_TOP | ALIGN_HJUSTIFY, Rect(780, 30, 100, 20) },
    { L"Middle left", ALIGN_VCENTER | ALIGN_LEFT, Rect(450, 55, 100, 20) },
    { L"Middle center", ALIGN_VCENTER | ALIGN_HCENTER, Rect(560, 55, 100, 20) },
    { L"Middle right", ALIGN_VCENTER | ALIGN_RIGHT, Rect(670, 55, 100, 20) },
    { L"Middle justify", ALIGN_VCENTER | ALIGN_HJUSTIFY, Rect(780, 55, 100, 20) },
    { L"Bottom left", ALIGN_BOTTOM | ALIGN_LEFT, Rect(450, 80, 100, 20) },
    { L"Bottom center", ALIGN_BOTTOM | ALIGN_HCENTER, Rect(560, 80, 100, 20) },
    { L"Bottom right", ALIGN_BOTTOM | ALIGN_RIGHT, Rect(670, 80, 100, 20) },
    { L"Bottom justify", ALIGN_BOTTOM | ALIGN_HJUSTIFY, Rect(780, 80, 100, 20) },
};

static const ButtonInfo fittingButtonsInfo[] = {
    { L"Disable", 0, Rect(450, 130, 100, 20) },
    { L"Points", TextBlock::FITTING_POINTS, Rect(560, 130, 100, 20) },
    { L"Enlarge", TextBlock::FITTING_ENLARGE, Rect(450, 155, 100, 20) },
    { L"Reduce", TextBlock::FITTING_REDUCE, Rect(560, 155, 100, 20) },
    { L"Enlarge/reduce", TextBlock::FITTING_ENLARGE | TextBlock::FITTING_REDUCE, Rect(450, 180, 100, 20) },
};

static const ButtonInfo multilineButtonsInfo[] = {
    { L"Disable", UIStaticText::MULTILINE_DISABLED, Rect(450, 280, 100, 20) },
    { L"By words", UIStaticText::MULTILINE_ENABLED, Rect(560, 280, 100, 20) },
    { L"By symbols", UIStaticText::MULTILINE_ENABLED_BY_SYMBOL, Rect(670, 280, 100, 20) },
};

StaticTextTest::StaticTextTest(TestBed& app)
    : BaseScreen(app, "TextAlignTest")
{
}

void StaticTextTest::LoadResources()
{
    BaseScreen::LoadResources();

    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/DejaVuSans.ttf"));

    ScopedPtr<UIStaticText> label(new UIStaticText(Rect(20, 5, 400, 20)));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Preview:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    previewText = new UIStaticText(Rect(20, 30, 400, 200));
    previewText->SetFont(font);
    previewText->SetFontSize(24.f);
    previewText->SetTextColor(Color::White);
    previewText->SetText(L"");
    previewText->GetOrCreateComponent<UIDebugRenderComponent>();
    previewText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    previewText->SetTextUseRtlAlign(TextBlock::RTL_USE_BY_CONTENT);
    previewText->SetForceBiDiSupportEnabled(true);
    AddControl(previewText);

    label = new UIStaticText(Rect(20, 235, 400, 20));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Input:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    inputText = new UITextField(Rect(20, 260, 400, 200));
    inputText->SetFont(font);
    inputText->SetFontSize(24.f);
    inputText->SetTextColor(Color::White);
    inputText->SetText(L"");
    inputText->GetOrCreateComponent<UIDebugRenderComponent>();
    inputText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    inputText->SetTextUseRtlAlign(TextBlock::RTL_USE_BY_CONTENT);
    inputDelegate = new InputDelegate(this);
    inputText->SetDelegate(inputDelegate);
    inputText->SetMultiline(true);
    inputText->GetOrCreateComponent<UIFocusComponent>();
    AddControl(inputText);

    debugLabel = new UIStaticText(Rect(20, 470, 400, 70));
    debugLabel->SetFont(font);
    debugLabel->SetFontSize(14.f);
    debugLabel->SetTextColor(Color::White);
    debugLabel->SetText(L"TextBlock Debug:\n");
    debugLabel->SetTextAlign(ALIGN_LEFT);
    debugLabel->SetMultiline(true);
    AddControl(debugLabel);

    label = new UIStaticText(Rect(450, 5, 200, 20));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Aligns:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    for (auto info : alignButtonsInfo)
    {
        UIButton* btn = CreateButton(info.caption, info.rect, info.tag, font, Message(this, &StaticTextTest::OnAlignButtonClick));
        alignButtons.push_back(btn);
    }

    SetPreviewAlign(ALIGN_TOP | ALIGN_LEFT);

    label = new UIStaticText(Rect(450, 105, 200, 20));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Fitting:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    for (auto info : fittingButtonsInfo)
    {
        UIButton* btn = CreateButton(info.caption, info.rect, info.tag, font, Message(this, &StaticTextTest::OnFittingButtonClick));
        fittingButtons.push_back(btn);
    }

    label = new UIStaticText(Rect(450, 205, 200, 20));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Required text size:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    requireTextSizeButton = new UIButton(Rect(450, 230, 100, 20));
    requireTextSizeButton->SetStateFont(0xFF, font);
    requireTextSizeButton->SetStateFontSize(0xFF, 14.f);
    requireTextSizeButton->SetStateFontColor(0xFF, Color::White);
    requireTextSizeButton->SetStateText(0xFF, L"On / Off");
    requireTextSizeButton->GetOrCreateComponent<UIDebugRenderComponent>();
    requireTextSizeButton->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &StaticTextTest::OnRequireTextSizeButtonClick));
    AddControl(requireTextSizeButton);

    label = new UIStaticText(Rect(450, 255, 200, 20));
    label->SetFont(font);
    label->SetFontSize(14.f);
    label->SetTextColor(Color::White);
    label->SetText(L"Multiline:");
    label->SetTextAlign(ALIGN_LEFT);
    AddControl(label);

    for (auto info : multilineButtonsInfo)
    {
        UIButton* btn = CreateButton(info.caption, info.rect, info.tag, font, Message(this, &StaticTextTest::OnMultilineButtonClick));
        multilineButtons.push_back(btn);
    }

    SetPreviewMultiline(UIStaticText::MULTILINE_DISABLED);

    SetPreviewRequiredTextSize(true);
}

void StaticTextTest::UnloadResources()
{
    if (inputText)
        inputText->SetDelegate(nullptr);
    SafeDelete(inputDelegate);
    SafeRelease(previewText);
    SafeRelease(inputText);
    SafeRelease(requireTextSizeButton);
    for (auto btn : alignButtons)
    {
        SafeRelease(btn);
    }
    alignButtons.clear();
    for (auto btn : fittingButtons)
    {
        SafeRelease(btn);
    }
    fittingButtons.clear();
    for (auto btn : multilineButtons)
    {
        SafeRelease(btn);
    }
    multilineButtons.clear();

    BaseScreen::UnloadResources();
}

void StaticTextTest::SetPreviewText(const DAVA::WideString& text)
{
    static const Vector2 NO_REQUIRED_SIZE = Vector2(-1.f, -1.f);
    if (needRequiredSize)
        previewText->SetText(text);
    else
        previewText->SetText(text, NO_REQUIRED_SIZE);
    UpdateDebugLabel();
}

void StaticTextTest::SetPreviewAlign(DAVA::int32 align)
{
    previewText->SetTextAlign(align);
    inputText->SetTextAlign(align);
    for (auto btn : alignButtons)
    {
        btn->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(btn->GetTag() == align ? GREEN : RED);
    }
    UpdateDebugLabel();
}

void StaticTextTest::SetPreviewFitting(DAVA::int32 fitting)
{
    previewText->SetFittingOption(fitting);
    for (auto btn : fittingButtons)
    {
        btn->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(btn->GetTag() == fitting ? GREEN : RED);
    }
    UpdateDebugLabel();
}

void StaticTextTest::SetPreviewRequiredTextSize(bool enable)
{
    needRequiredSize = enable;
    SetPreviewText(previewText->GetText());
    requireTextSizeButton->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(needRequiredSize ? GREEN : RED);
    requireTextSizeButton->SetStateText(0xFF, needRequiredSize ? L"On" : L"Off");
    UpdateDebugLabel();
}

void StaticTextTest::SetPreviewMultiline(int32 multilineType)
{
    previewText->SetMultilineType(multilineType);
    for (auto btn : multilineButtons)
    {
        btn->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(btn->GetTag() == multilineType ? GREEN : RED);
    }
    UpdateDebugLabel();
}

UIButton* StaticTextTest::CreateButton(const WideString& caption, const Rect& rect, int32 tag, DAVA::Font* font, const DAVA::Message& msg)
{
    UIButton* button = new UIButton(rect);
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, 14.f);
    button->SetStateFontColor(0xFF, Color::White);
    button->SetStateText(0xFF, caption);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, msg);
    button->SetTag(tag);
    AddControl(button);
    return button;
}

void StaticTextTest::OnAlignButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = DynamicTypeCheck<UIButton*>(sender);
    SetPreviewAlign(btn->GetTag());
}

void StaticTextTest::OnFittingButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = DynamicTypeCheck<UIButton*>(sender);
    SetPreviewFitting(btn->GetTag());
}

void StaticTextTest::OnRequireTextSizeButtonClick(BaseObject* sender, void* data, void* callerData)
{
    SetPreviewRequiredTextSize(!needRequiredSize);
}

void StaticTextTest::OnMultilineButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = DynamicTypeCheck<UIButton*>(sender);
    SetPreviewMultiline(btn->GetTag());
}

void StaticTextTest::UpdateDebugLabel()
{
    String fitting = "NONE";
    switch (previewText->GetTextBlock()->GetFittingOptionUsed())
    {
    case TextBlock::eFitType::FITTING_ENLARGE:
        fitting = "ENLARGE";
        break;
    case TextBlock::eFitType::FITTING_REDUCE:
        fitting = "REDUCE";
        break;
    case TextBlock::eFitType::FITTING_REDUCE | TextBlock::eFitType::FITTING_ENLARGE:
        fitting = "ENLARGE|REDUCE";
        break;
    case TextBlock::eFitType::FITTING_POINTS:
        fitting = "POINTS";
        break;
    }

    String croped = previewText->GetTextBlock()->IsVisualTextCroped() ? "true " : "false";

    debugLabel->SetUtf8Text(Format("TextBlock Debug:\nIsVisualTextCroped: %s\nFittingOptionUsed: %s", croped.c_str(), fitting.c_str()));
}
