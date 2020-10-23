#include "UIScreens/BaseScreen.h"

#include <Input/InputElements.h>
#include <Render/2D/FTFont.h>
#include <UI/UIButton.h>
#include <UI/UIScreenManager.h>

DAVA::int32 BaseScreen::screensCount = 0;

BaseScreen::BaseScreen()
    : UIScreen()
    , screenID(screensCount++)
{
    DAVA::GetEngineContext()->uiScreenManager->RegisterScreen(screenID, this);
}

bool BaseScreen::SystemInput(DAVA::UIEvent* currentInput)
{
    if ((currentInput->key == DAVA::eInputElements::BACK) && (DAVA::UIEvent::Phase::KEY_DOWN == currentInput->phase))
    {
        SetPreviousScreen();
        return true;
    }

    return UIScreen::SystemInput(currentInput);
}

void BaseScreen::LoadResources()
{
    DVASSERT(!font);
    font = DAVA::FTFont::Create("~res:/UIViewer/Fonts/korinna.ttf");
}

void BaseScreen::UnloadResources()
{
    font.reset();
    RemoveAllControls();
}

DAVA::UIButton* BaseScreen::CreateButton(const DAVA::Rect& rect, const DAVA::WideString& text)
{
    using namespace DAVA;
    DVASSERT(font);

    UIButton* button = new UIButton(rect);
    button->SetStateText(UIControl::STATE_NORMAL, text);
    button->SetStateTextAlign(UIControl::STATE_NORMAL, ALIGN_HCENTER | ALIGN_VCENTER);

    button->SetStateFontColor(UIControl::STATE_NORMAL, Color::White);
    button->SetStateFont(UIControl::STATE_NORMAL, font);
    button->SetStateFontSize(UIControl::STATE_NORMAL, fontSize);
    button->SetStateTextColorInheritType(UIControl::STATE_NORMAL, UIControlBackground::COLOR_IGNORE_PARENT);

    button->SetStateDrawType(UIControl::STATE_NORMAL, UIControlBackground::DRAW_FILL);
    button->GetStateBackground(UIControl::STATE_NORMAL)->SetColor(Color(0.4f, 0.5f, 0.4f, 0.9f));
    button->SetStateDrawType(UIControl::STATE_PRESSED_INSIDE, UIControlBackground::DRAW_FILL);
    button->GetStateBackground(UIControl::STATE_PRESSED_INSIDE)->SetColor(Color(0.65f, 0.75f, 0.65f, 0.9f));

    return button;
}

void BaseScreen::SetPreviousScreen() const
{
    if (screenID)
    {
        DAVA::GetEngineContext()->uiScreenManager->SetScreen(screenID - 1);
    }
}

void BaseScreen::SetNextScreen() const
{
    if (screenID < screensCount - 1)
    {
        DAVA::GetEngineContext()->uiScreenManager->SetScreen(screenID + 1);
    }
}
