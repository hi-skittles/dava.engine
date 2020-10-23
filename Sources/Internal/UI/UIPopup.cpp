#include "UI/UIPopup.h"
#include "UI/UIControlSystem.h"
#include "Engine/Engine.h"

namespace DAVA
{
UIPopup::UIPopup(const Rect& rect)
    : UIScreen(rect)
    , isTransparent(true)
{
}

void UIPopup::Show()
{
    GetEngineContext()->uiControlSystem->AddPopup(this);
}

void UIPopup::Hide()
{
    if (IsActive())
        GetEngineContext()->uiControlSystem->RemovePopup(this);
}
};
