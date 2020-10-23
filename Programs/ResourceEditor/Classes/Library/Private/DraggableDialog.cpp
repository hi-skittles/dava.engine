#include "DraggableDialog.h"

DraggableDialog::DraggableDialog(const DAVA::Rect& rect)
    : UIControl(rect)
{
}

DraggableDialog::~DraggableDialog()
{
}

void DraggableDialog::DidAppear()
{ //make a bool flag to keep original position
    originalPosition = relativePosition;
}

void DraggableDialog::DidDisappear()
{
    relativePosition = originalPosition;
}

void DraggableDialog::Input(DAVA::UIEvent* currentInput)
{
    if (currentInput->phase == DAVA::UIEvent::Phase::BEGAN)
    {
        basePoint = currentInput->point;
    }
    if (currentInput->phase == DAVA::UIEvent::Phase::DRAG)
    {
        relativePosition += (currentInput->point - basePoint);
        basePoint = currentInput->point;
        UIControl* parent = GetParent();
        if (parent)
        {
            if (relativePosition.x > parent->size.x - 10)
            {
                relativePosition.x = parent->size.x - 10;
            }
            if (relativePosition.y > parent->size.y - 10)
            {
                relativePosition.y = parent->size.y - 10;
            }
            if (relativePosition.x + size.x < 10)
            {
                relativePosition.x = 10 - size.x;
            }
            if (relativePosition.y + size.y < 10)
            {
                relativePosition.y = 10 - size.y;
            }
        }
    }
}
