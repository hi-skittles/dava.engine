#include "Tests/MultilineTest.h"
#include "UI/Focus/UIFocusComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

namespace MultilineTestDatails
{
class TextDelegate1 : public UITextFieldDelegate
{
public:
    virtual ~TextDelegate1() = default;
    bool TextFieldKeyPressed(UITextField* /*textField*/, int32 replacementLocation, int32 replacementLength, WideString& replacementString) override
    {
        // Allow only small caps letters
        bool accept = true;
        for (size_t i = 0, n = replacementString.length(); i < n && accept; ++i)
        {
            accept = ('a' <= replacementString[i] && replacementString[i] <= 'z') || replacementString[i] == ' ';
        }
        Logger::Debug("****** TextDelegate1::TextFieldKeyPressed: accepted=%d", accept);
        return accept;
    }
    void TextFieldOnTextChanged(UITextField* /*textField*/, const WideString& newText, const WideString& oldText) override
    {
        Logger::Debug("****** TextDelegate1::TextFieldOnTextChanged: new=%s, old=%s", UTF8Utils::EncodeToUTF8(newText).c_str(), UTF8Utils::EncodeToUTF8(oldText).c_str());
    }
    void TextFieldShouldReturn(UITextField* textField) override
    {
        textField->StopEdit();
    }
};

class TextDelegate2 : public UITextFieldDelegate
{
public:
    virtual ~TextDelegate2() = default;
    bool TextFieldKeyPressed(UITextField* /*textField*/, int32 replacementLocation, int32 replacementLength, WideString& replacementString) override
    {
        // Allow only numbers
        bool accept = true;
        for (size_t i = 0, n = replacementString.length(); i < n && accept; ++i)
        {
            accept = '0' <= replacementString[i] && replacementString[i] <= '9';
        }
        Logger::Debug("****** TextDelegate2::TextFieldKeyPressed: accepted=%d", accept);
        return accept;
    }
    void TextFieldOnTextChanged(UITextField* /*textField*/, const WideString& newText, const WideString& oldText) override
    {
        Logger::Debug("****** TextDelegate2::TextFieldOnTextChanged: new=%s, old=%s", UTF8Utils::EncodeToUTF8(newText).c_str(), UTF8Utils::EncodeToUTF8(oldText).c_str());
    }
};

class TextDelegateMulti : public UITextFieldDelegate
{
public:
    virtual ~TextDelegateMulti() = default;
};

const float32 FONT_SIZE = 20.f;
}

MultilineTest::MultilineTest(TestBed& app)
    : BaseScreen(app, "MultilineTest")
{
}

void MultilineTest::LoadResources()
{
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    textDelegate1 = new MultilineTestDatails::TextDelegate1();
    textDelegate2 = new MultilineTestDatails::TextDelegate2();

    textField1 = new UITextField(Rect(5, 10, 400, 60));
    textField1->SetFont(font);
    textField1->SetFontSize(MultilineTestDatails::FONT_SIZE);
    textField1->SetText(L"hello world");
    textField1->GetOrCreateComponent<UIDebugRenderComponent>();
    textField1->SetTextColor(Color(0.0, 1.0, 0.0, 1.0));
    textField1->SetDelegate(textDelegate1);
    textField1->GetOrCreateComponent<UIFocusComponent>();
    textField1->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    textField1->SetMaxLength(10);
    AddControl(textField1);

    textField2 = new UITextField(Rect(5, 80, 400, 60));
    textField2->SetIsPassword(true);
    textField2->SetFont(font);
    textField2->SetFontSize(MultilineTestDatails::FONT_SIZE);
    textField2->SetText(L"123456");
    textField2->GetOrCreateComponent<UIDebugRenderComponent>();
    textField2->SetTextColor(Color(0.0, 0.0, 1.0, 1.0));
    textField2->SetKeyboardType(UITextField::eKeyboardType::KEYBOARD_TYPE_NUMBER_PAD);
    textField2->SetDelegate(textDelegate2);
    textField2->GetOrCreateComponent<UIFocusComponent>();
    textField2->SetTextAlign(ALIGN_RIGHT | ALIGN_TOP);
    AddControl(textField2);

    RefPtr<UITextField> textField3(new UITextField(Rect(5, 150, 400, 60)));
    textField3->SetFont(font);
    textField3->SetFontSize(MultilineTestDatails::FONT_SIZE);
    textField3->SetText(L"field w/o options");
    textField3->GetOrCreateComponent<UIDebugRenderComponent>();
    textField3->GetOrCreateComponent<UIFocusComponent>();
    AddControl(textField3.Get());

    textFieldMulti = new UITextField(Rect(450, 10, 400, 120));
    textFieldMulti->GetOrCreateComponent<UIFocusComponent>();
    textFieldMulti->SetFont(font);
    textFieldMulti->SetFontSize(MultilineTestDatails::FONT_SIZE);
    textFieldMulti->SetText(L"Multiline text field");
    textFieldMulti->GetOrCreateComponent<UIDebugRenderComponent>();
    textFieldMulti->SetTextColor(Color(0.0, 0.0, 1.0, 1.0));
    textFieldMulti->SetMultiline(true);
    textFieldMulti->SetTextAlign(ALIGN_HCENTER | ALIGN_TOP);
    AddControl(textFieldMulti);

    const float32 Y_OFFSET = 500;
    const float32 CONTROL_LENGHT = 400;
    const float32 CONTROL_HEIGTH = 70;

    UIButton* button = new UIButton(Rect(0, Y_OFFSET, CONTROL_LENGHT, CONTROL_HEIGTH));
    button->SetStateFont(0xFF, font);
    button->SetStateFontSize(0xFF, MultilineTestDatails::FONT_SIZE);
    button->SetStateFontColor(0xFF, Color::White);
    button->SetStateText(0xFF, L"Show/Hide");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &MultilineTest::OnShowHideClick));
    AddControl(button);
    SafeRelease(button);

    UITextField* field = new UITextField(Rect(0, Y_OFFSET + CONTROL_HEIGTH + 10, CONTROL_LENGHT, CONTROL_HEIGTH));
    field->SetFont(font);
    field->SetFontSize(MultilineTestDatails::FONT_SIZE);
    field->GetOrCreateComponent<UIDebugRenderComponent>();
    field->SetText(L"Test text inside UITextField used for test");
    field->SetDelegate(this);
    AddControl(field);
    SafeRelease(field);

    field = new UITextField(Rect(0, Y_OFFSET + 2 * (CONTROL_HEIGTH + 10), CONTROL_LENGHT, CONTROL_HEIGTH));
    field->GetOrCreateComponent<UIFocusComponent>();
    field->SetFont(font);
    field->SetFontSize(MultilineTestDatails::FONT_SIZE);
    field->GetOrCreateComponent<UIDebugRenderComponent>();
    field->SetText(L"Test text inside UITextField used for test");
    field->SetDelegate(this);

    AddControl(field);
    SafeRelease(field);

    topLayerControl = new UIControl(Rect(CONTROL_LENGHT / 3, Y_OFFSET, CONTROL_LENGHT / 3, 3 * (Y_OFFSET + CONTROL_HEIGTH + 10)));
    UIControlBackground* topLayerControlBg = topLayerControl->GetOrCreateComponent<UIControlBackground>();
    topLayerControlBg->SetColor(Color(1.0f, 0.0f, 0.0f, 0.5f));
    topLayerControlBg->SetDrawType(UIControlBackground::DRAW_FILL);
    topLayerControlBg->SetColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
    topLayerControl->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(topLayerControl);

    BaseScreen::LoadResources();
}

void MultilineTest::UnloadResources()
{
    RemoveAllControls();

    SafeRelease(textField1);
    SafeRelease(textField2);
    SafeRelease(textFieldMulti);

    SafeDelete(textDelegate1);
    SafeDelete(textDelegate2);
    SafeRelease(topLayerControl);
}

void MultilineTest::OnShowHideClick(BaseObject* sender, void* data, void* callerData)
{
    if (nullptr != topLayerControl)
    {
        static bool isVisible = true;
        topLayerControl->SetVisibilityFlag(isVisible);
        isVisible = !isVisible;
    }
}

UIButton* MultilineTest::CreateUIButton(Font* font, const Rect& rect, const String& text,
                                        void (MultilineTest::*onClick)(BaseObject*, void*, void*))
{
    UIButton* button = new UIButton(rect);
    button->SetStateFont(0xFF, font);
    button->SetStateText(0xFF, UTF8Utils::EncodeToWideString(text));
    button->SetStateFontColor(0xFF, Color::White);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, onClick));
    AddControl(button);
    return button;
}
