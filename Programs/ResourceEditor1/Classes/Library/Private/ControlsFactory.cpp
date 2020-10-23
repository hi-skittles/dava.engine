#include "Classes/Library/Private/ControlsFactory.h"

DAVA::Font* ControlsFactory::font12 = NULL;
DAVA::Font* ControlsFactory::font20 = NULL;

DAVA::UIButton* ControlsFactory::CreateButton(const DAVA::Rect& rect, const DAVA::WideString& buttonText, bool designers)
{
    DAVA::UIButton* btn = new DAVA::UIButton(rect);
    CustomizeButton(btn, buttonText, designers);
    return btn;
}

void ControlsFactory::CustomizeButton(DAVA::UIButton* btn, const DAVA::WideString& buttonText, bool designers)
{
    DAVA::Font* font = GetFont12();

    btn->SetStateDrawType(DAVA::UIControl::STATE_NORMAL, DAVA::UIControlBackground::DRAW_FILL);
    btn->SetStateDrawType(DAVA::UIControl::STATE_PRESSED_INSIDE, DAVA::UIControlBackground::DRAW_FILL);
    btn->SetStateDrawType(DAVA::UIControl::STATE_DISABLED, DAVA::UIControlBackground::DRAW_FILL);
    btn->SetStateDrawType(DAVA::UIControl::STATE_SELECTED, DAVA::UIControlBackground::DRAW_FILL);

    if (designers)
    {
        btn->GetStateBackground(DAVA::UIControl::STATE_NORMAL)->SetColor(DAVA::Color(97.f / 255.f, 69.f / 255.f, 68.f / 255.f, 1.f));
    }
    else
    {
        btn->GetStateBackground(DAVA::UIControl::STATE_NORMAL)->SetColor(DAVA::Color(0.0f, 0.0f, 0.0f, 0.5f));
    }
    btn->GetStateBackground(DAVA::UIControl::STATE_PRESSED_INSIDE)->SetColor(DAVA::Color(0.5f, 0.5f, 0.5f, 0.5f));
    btn->GetStateBackground(DAVA::UIControl::STATE_DISABLED)->SetColor(DAVA::Color(0.2f, 0.2f, 0.2f, 0.2f));
    btn->GetStateBackground(DAVA::UIControl::STATE_SELECTED)->SetColor(DAVA::Color(0.0f, 0.0f, 1.0f, 0.2f));

    btn->SetStateFont(DAVA::UIControl::STATE_PRESSED_INSIDE, font);
    btn->SetStateFont(DAVA::UIControl::STATE_DISABLED, font);
    btn->SetStateFont(DAVA::UIControl::STATE_NORMAL, font);
    btn->SetStateFont(DAVA::UIControl::STATE_SELECTED, font);

    btn->SetStateText(DAVA::UIControl::STATE_PRESSED_INSIDE, buttonText);
    btn->SetStateText(DAVA::UIControl::STATE_DISABLED, buttonText);
    btn->SetStateText(DAVA::UIControl::STATE_NORMAL, buttonText);
    btn->SetStateText(DAVA::UIControl::STATE_SELECTED, buttonText);

    btn->SetStateFontColor(DAVA::UIControl::STATE_PRESSED_INSIDE, DAVA::Color(1.0f, 1.0f, 1.0f, 0.85f));
    btn->SetStateFontColor(DAVA::UIControl::STATE_DISABLED, DAVA::Color(1.0f, 1.0f, 1.0f, 0.5f));
    btn->SetStateFontColor(DAVA::UIControl::STATE_NORMAL, DAVA::Color(1.0f, 1.0f, 1.0f, 1.0f));
    btn->SetStateFontColor(DAVA::UIControl::STATE_SELECTED, DAVA::Color(1.0f, 1.0f, 1.0f, 1.0f));

    btn->SetStateTextColorInheritType(DAVA::UIControl::STATE_PRESSED_INSIDE, DAVA::UIControlBackground::COLOR_IGNORE_PARENT);
    btn->SetStateTextColorInheritType(DAVA::UIControl::STATE_DISABLED, DAVA::UIControlBackground::COLOR_IGNORE_PARENT);
    btn->SetStateTextColorInheritType(DAVA::UIControl::STATE_NORMAL, DAVA::UIControlBackground::COLOR_IGNORE_PARENT);
    btn->SetStateTextColorInheritType(DAVA::UIControl::STATE_SELECTED, DAVA::UIControlBackground::COLOR_IGNORE_PARENT);

    AddBorder(btn);
}

DAVA::Font* ControlsFactory::GetFont12()
{
    if (!font12)
    {
        font12 = DAVA::FTFont::Create("~res:/Fonts/MyriadPro-Regular.otf");
        font12->SetSize(12);
    }
    return font12;
}

DAVA::Font* ControlsFactory::GetFont20()
{
    if (!font20)
    {
        font20 = DAVA::FTFont::Create("~res:/Fonts/MyriadPro-Regular.otf");
        font20->SetSize(20);
    }
    return font12;
}

DAVA::Color ControlsFactory::GetColorError()
{
    return DAVA::Color(1.0f, 0.0f, 0.0f, 0.8f);
}

DAVA::UIControl* ControlsFactory::CreateLine(const DAVA::Rect& rect, DAVA::Color color)
{
    DAVA::UIControl* lineControl = new DAVA::UIControl(rect);
    DAVA::UIControlBackground* bg = lineControl->GetOrCreateComponent<DAVA::UIControlBackground>();
    bg->color = color;
    bg->SetDrawType(DAVA::UIControlBackground::DRAW_FILL);
    return lineControl;
}

void ControlsFactory::CustomizeDialogFreeSpace(DAVA::UIControl* c)
{
    DAVA::UIControlBackground* bg = c->GetOrCreateComponent<DAVA::UIControlBackground>();
    bg->color = DAVA::Color(0.0f, 0.0f, 0.0f, 0.3f);
    bg->SetDrawType(DAVA::UIControlBackground::DRAW_FILL);
}

void ControlsFactory::CustomizeDialog(DAVA::UIControl* c)
{
    DAVA::UIControlBackground* bg = c->GetOrCreateComponent<DAVA::UIControlBackground>();
    bg->color = DAVA::Color(0.0f, 0.0f, 0.0f, 0.5f);
    bg->SetDrawType(DAVA::UIControlBackground::DRAW_FILL);
}

void ControlsFactory::AddBorder(DAVA::UIControl* c)
{
    DAVA::Rect fullRect = c->GetRect();

    DAVA::Color lineColor(1.f, 1.f, 1.f, 0.5f);

    DAVA::UIControl* leftLine = c->FindByName("LeftLine", false);
    if (!leftLine)
    {
        leftLine = ControlsFactory::CreateLine(DAVA::Rect(0, 1, 1, fullRect.dy - 2), lineColor);
        leftLine->SetName("LeftLine");
        c->AddControl(leftLine);
        SafeRelease(leftLine);
    }

    DAVA::UIControl* rightLine = c->FindByName("RightLine", false);
    if (!rightLine)
    {
        rightLine = ControlsFactory::CreateLine(DAVA::Rect(fullRect.dx - 1, 1, 1, fullRect.dy - 2), lineColor);
        rightLine->SetName("RightLine");
        c->AddControl(rightLine);
        SafeRelease(rightLine);
    }

    DAVA::UIControl* topLine = c->FindByName("TopLine", false);
    if (!topLine)
    {
        topLine = ControlsFactory::CreateLine(DAVA::Rect(0, 0, fullRect.dx, 1), lineColor);
        topLine->SetName("TopLine");
        c->AddControl(topLine);
        SafeRelease(topLine);
    }

    DAVA::UIControl* bottomtLine = c->FindByName("BottomLine", false);
    if (!bottomtLine)
    {
        bottomtLine = ControlsFactory::CreateLine(DAVA::Rect(0, fullRect.dy - 1, fullRect.dx, 1), lineColor);
        bottomtLine->SetName("BottomLine");
        c->AddControl(bottomtLine);
        SafeRelease(bottomtLine);
    }
}

void ControlsFactory::ReleaseFonts()
{
    SafeRelease(font12);
    SafeRelease(font20);
}
