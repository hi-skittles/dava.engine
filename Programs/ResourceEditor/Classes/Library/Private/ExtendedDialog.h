#ifndef __EXTENDED_DIALOG_H__
#define __EXTENDED_DIALOG_H__

#include "DAVAEngine.h"
#include "DraggableDialog.h"

class ExtendedDialog : public DAVA::UIControl
{
protected:
    virtual ~ExtendedDialog();

public:
    ExtendedDialog();
    void Show();
    virtual void Close();

    virtual void OnActive();

protected:
    bool IsShown() const;
    const DAVA::Rect GetScreenRect() const;

    virtual void UpdateSize();

    virtual const DAVA::Rect GetDialogRect() const;
    DraggableDialog* draggableDialog = nullptr;

private:
    bool isShown = false;
};

#endif //__EXTENDED_DIALOG_H__