#ifndef DRAGGABLE_DIALOG
#define DRAGGABLE_DIALOG

#include "DAVAEngine.h"

class DraggableDialog : public DAVA::UIControl
{
protected:
    ~DraggableDialog();

public:
    DraggableDialog(const DAVA::Rect& rect);

    virtual void DidAppear();
    virtual void DidDisappear();

    virtual void Input(DAVA::UIEvent* currentInput);

protected:
    DAVA::Vector2 originalPosition;
    DAVA::Vector2 basePoint;
};

#endif