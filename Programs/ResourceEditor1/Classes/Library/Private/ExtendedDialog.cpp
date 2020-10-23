#include "Classes/Library/Private/ExtendedDialog.h"
#include "Classes/Library/Private/ControlsFactory.h"

ExtendedDialog::ExtendedDialog()
    : UIControl(DAVA::UIScreenManager::Instance()->GetScreen()->GetRect())
{
    ControlsFactory::CustomizeDialogFreeSpace(this);

    draggableDialog = new DraggableDialog(GetDialogRect());
    ControlsFactory::CustomizeDialog(draggableDialog);
    AddControl(draggableDialog);
}

ExtendedDialog::~ExtendedDialog()
{
    SafeRelease(draggableDialog);
}

void ExtendedDialog::Show()
{
    isShown = true;
}

bool ExtendedDialog::IsShown() const
{
    return isShown;
}

const DAVA::Rect ExtendedDialog::GetScreenRect() const
{
    DAVA::UIScreen* activeScreen = DAVA::UIScreenManager::Instance()->GetScreen();
    if (activeScreen)
    {
        return activeScreen->GetRect();
    }

    return DAVA::Rect();
}

const DAVA::Rect ExtendedDialog::GetDialogRect() const
{
    const DAVA::Rect screenRect = GetScreenRect();

    return DAVA::Rect(screenRect.dx / 4, screenRect.dy / 4, screenRect.dx / 2, screenRect.dy / 2);
}

void ExtendedDialog::Close()
{
    isShown = false;
    if (GetParent())
    {
        GetParent()->RemoveControl(this);
    }
}

void ExtendedDialog::OnActive()
{
    UIControl::OnActive();

    UpdateSize();
}

void ExtendedDialog::UpdateSize()
{
    SetRect(GetScreenRect());

    draggableDialog->SetRect(GetDialogRect());
}
