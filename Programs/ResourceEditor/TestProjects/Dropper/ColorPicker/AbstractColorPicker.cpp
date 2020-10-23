#include "AbstractColorPicker.h"

AbstractColorPicker::AbstractColorPicker(QWidget* parent)
    : QWidget(parent)
{
}

AbstractColorPicker::~AbstractColorPicker()
{
}

QColor AbstractColorPicker::GetColor() const
{
    return color;
}

void AbstractColorPicker::SetColor(QColor const& c)
{
    color = c;
    SetColorInternal(c);
}
