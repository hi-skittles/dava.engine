#include "BaseScreen.h"

uint32 BaseScreen::SCREEN_INDEX = 0;

BaseScreen::BaseScreen()
    : currentScreenIndex(-1)
{
}

void BaseScreen::RegisterScreen()
{
    GetEngineContext()->uiScreenManager->RegisterScreen(SCREEN_INDEX, this);
    GetEngineContext()->uiScreenManager->SetScreen(SCREEN_INDEX);

    currentScreenIndex = SCREEN_INDEX;
    SCREEN_INDEX++;
}

bool BaseScreen::IsRegistered() const
{
    return this == GetEngineContext()->uiScreenManager->GetScreen(currentScreenIndex);
}

bool BaseScreen::IsFinished() const
{
    return false;
}
