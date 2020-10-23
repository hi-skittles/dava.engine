#include "Classes/Qt/DockParticleEditor/WheellIgnorantComboBox.h"

#include <QEvent>

WheellIgnorantComboBox::WheellIgnorantComboBox(QWidget* parent /*= 0*/)
    : QComboBox(parent)
{
}

bool WheellIgnorantComboBox::event(QEvent* e)
{
    if (e->type() == QEvent::Wheel)
    {
        if (this->hasFocus() == false)
        {
            return false;
        }
    }
    return QComboBox::event(e);
}