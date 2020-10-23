#include "Tests/FontTest.h"

#include <UI/Focus/UIFocusComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <Engine/Engine.h>
#include <Engine/Window.h>

using namespace DAVA;

namespace FontTestDetails
{
class InputDelegate : public UITextFieldDelegate
{
public:
    InputDelegate(UIStaticText* text)
        : staticText(SafeRetain(text))
    {
    }

    ~InputDelegate()
    {
        SafeRelease(staticText);
    }

    void TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText) override
    {
        DVASSERT(staticText);
        staticText->SetText(newText);
    }

private:
    UIStaticText* staticText;
};

enum Tags
{
    INCREASE_SIZE_TAG = 1,
    DECREASE_SIZE_TAG
};

const float32 FONT_SIZE = 14.f;
}

FontTest::FontTest(TestBed& app)
    : BaseScreen(app, "FontTest")
{
}

void FontTest::LoadResources()
{
    BaseScreen::LoadResources();

    ftFont = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    dfFont = GraphicFont::Create("~res:/TestBed/Fonts/korinna_df.fnt", "~res:/TestBed/Fonts/korinna_df.tex");
    graphicFont = GraphicFont::Create("~res:/TestBed/Fonts/korinna_graphic.fnt", "~res:/TestBed/Fonts/korinna_graphic.tex");

    ScopedPtr<Font> uiFont(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    ScopedPtr<UIStaticText> label(new UIStaticText(Rect(10, 10, 200, 20)));
    label->SetFont(uiFont);
    label->SetFontSize(FontTestDetails::FONT_SIZE);
    label->SetTextColor(Color::White);
    label->SetText(L"Preview:");
    label->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    AddControl(label);

    previewText = new UIStaticText(Rect(10, 40, 400, 200));
    previewText->SetFont(ftFont);
    previewText->SetFontSize(FontTestDetails::FONT_SIZE);
    previewText->SetTextColor(Color::White);
    previewText->GetOrCreateComponent<UIDebugRenderComponent>();
    previewText->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    previewText->SetMultiline(true);
    AddControl(previewText);

    label = new UIStaticText(Rect(10, 250, 200, 20));
    label->SetFont(uiFont);
    label->SetFontSize(FontTestDetails::FONT_SIZE);
    label->SetTextColor(Color::White);
    label->SetText(L"Input:");
    label->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    AddControl(label);

    inputText = new UITextField(Rect(10, 280, 400, 200));
    inputText->GetOrCreateComponent<UIFocusComponent>();
    inputText->SetFont(uiFont);
    inputText->SetFontSize(FontTestDetails::FONT_SIZE);
    inputText->SetTextColor(Color::White);
    inputText->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    inputText->GetOrCreateComponent<UIDebugRenderComponent>();
    inputText->SetDelegate(inputDelegate = new FontTestDetails::InputDelegate(previewText));
    inputText->SetMultiline(true);
    AddControl(inputText);

    label = new UIStaticText(Rect(420, 10, 200, 20));
    label->SetFont(uiFont);
    label->SetFontSize(FontTestDetails::FONT_SIZE);
    label->SetTextColor(Color::White);
    label->SetText(L"Font type:");
    label->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    AddControl(label);

    ScopedPtr<UIButton> button(new UIButton(Rect(420, 40, 100, 20)));
    button->SetStateFont(0xFF, uiFont);
    button->SetStateFontSize(0xFF, FontTestDetails::FONT_SIZE);
    button->SetStateText(0xFF, L"FreeType");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FontTest::OnFontSelectClick));
    button->SetTag(Font::TYPE_FT);
    AddControl(button);

    button = new UIButton(Rect(530, 40, 100, 20));
    button->SetStateFont(0xFF, uiFont);
    button->SetStateFontSize(0xFF, FontTestDetails::FONT_SIZE);
    button->SetStateText(0xFF, L"Distance");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FontTest::OnFontSelectClick));
    button->SetTag(Font::TYPE_DISTANCE);
    AddControl(button);

    button = new UIButton(Rect(640, 40, 100, 20));
    button->SetStateFont(0xFF, uiFont);
    button->SetStateFontSize(0xFF, FontTestDetails::FONT_SIZE);
    button->SetStateText(0xFF, L"Graphic");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FontTest::OnFontSelectClick));
    button->SetTag(Font::TYPE_GRAPHIC);
    AddControl(button);

    label = new UIStaticText(Rect(420, 70, 100, 20));
    label->SetFont(uiFont);
    label->SetFontSize(FontTestDetails::FONT_SIZE);
    label->SetTextColor(Color::White);
    label->SetText(L"Font size:");
    label->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    AddControl(label);

    sizeText = new UIStaticText(Rect(520, 70, 100, 20));
    sizeText->SetFont(uiFont);
    label->SetFontSize(FontTestDetails::FONT_SIZE);
    sizeText->SetTextColor(Color::White);
    sizeText->SetText(L"00");
    sizeText->SetTextAlign(ALIGN_TOP | ALIGN_LEFT);
    AddControl(sizeText);

    button = new UIButton(Rect(420, 100, 100, 20));
    button->SetStateFont(0xFF, uiFont);
    button->SetStateFontSize(0xFF, FontTestDetails::FONT_SIZE);
    button->SetStateText(0xFF, L"Increase");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FontTest::OnFontSizeClick));
    button->SetTag(FontTestDetails::INCREASE_SIZE_TAG);
    AddControl(button);

    button = new UIButton(Rect(530, 100, 100, 20));
    button->SetStateFont(0xFF, uiFont);
    button->SetStateFontSize(0xFF, FontTestDetails::FONT_SIZE);
    button->SetStateText(0xFF, L"Decrease");
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->AddEvent(UIButton::EVENT_TOUCH_DOWN, Message(this, &FontTest::OnFontSizeClick));
    button->SetTag(FontTestDetails::DECREASE_SIZE_TAG);
    AddControl(button);

    UpdateFontSizeText();
}

void FontTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    SafeRelease(ftFont);
    SafeRelease(dfFont);
    SafeRelease(graphicFont);
    SafeDelete(inputDelegate);
    SafeRelease(inputText);
    SafeRelease(previewText);
    SafeRelease(sizeText);
}

void FontTest::OnFontSelectClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = DynamicTypeCheck<UIButton*>(sender);
    switch (btn->GetTag())
    {
    case Font::TYPE_FT:
        previewText->SetFont(ftFont);
        inputText->SetFont(ftFont);
        break;
    case Font::TYPE_DISTANCE:
        previewText->SetFont(dfFont);
        inputText->SetFont(dfFont);
        break;
    case Font::TYPE_GRAPHIC:
        previewText->SetFont(graphicFont);
        inputText->SetFont(graphicFont);
        break;
    }
    UpdateFontSizeText();
}

void FontTest::OnFontSizeClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* btn = DynamicTypeCheck<UIButton*>(sender);
    float32 size = previewText->GetFontSize();
    switch (btn->GetTag())
    {
    case FontTestDetails::INCREASE_SIZE_TAG:
        size += 1.f;
        break;
    case FontTestDetails::DECREASE_SIZE_TAG:
        size -= 1.f;
        break;
    }
    previewText->SetFontSize(size);
    UpdateFontSizeText();
}

void FontTest::UpdateFontSizeText()
{
    sizeText->SetUtf8Text(std::to_string(previewText->GetFontSize()));
}
