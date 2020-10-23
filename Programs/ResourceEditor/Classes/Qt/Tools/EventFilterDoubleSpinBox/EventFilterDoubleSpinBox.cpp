#include "EventFilterDoubleSpinBox.h"

#include "Math/MathConstants.h"

#include <QChar>
#include <QLocale>
#include <QLineEdit>

namespace EventFilterDoubleSpinBoxDetail
{
const QString infinityStrValue = "inf";
}

EventFilterDoubleSpinBox::EventFilterDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
    setKeyboardTracking(false);
}

void EventFilterDoubleSpinBox::keyPressEvent(QKeyEvent* event)
{
    QKeyEvent* changedKeyEvent = NULL;
    // Get decimal point specific to current system
    QChar decimalPoint = QLocale().decimalPoint();

    if (event->key() == Qt::Key_Comma && decimalPoint.toLatin1() == Qt::Key_Period)
    {
        // Change comma key event to period key event
        changedKeyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Period, Qt::NoModifier, decimalPoint, 0);
    }
    else if (event->key() == Qt::Key_Period && decimalPoint.toLatin1() == Qt::Key_Comma)
    {
        // Change period key event to comma key event
        changedKeyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Comma, Qt::NoModifier, decimalPoint, 0);
    }

    // Default behaviour
    QDoubleSpinBox::keyPressEvent(changedKeyEvent ? changedKeyEvent : event);
}

QValidator::State EventFilterDoubleSpinBox::validate(QString& input, int& pos) const
{
    if (EventFilterDoubleSpinBoxDetail::infinityStrValue.startsWith(input, Qt::CaseInsensitive))
    {
        return QValidator::Acceptable;
    }

    return QDoubleSpinBox::validate(input, pos);
}

double EventFilterDoubleSpinBox::valueFromText(const QString& text) const
{
    if (EventFilterDoubleSpinBoxDetail::infinityStrValue.compare(text, Qt::CaseInsensitive) == 0)
    {
        return std::numeric_limits<DAVA::float32>::max();
    }

    return QDoubleSpinBox::valueFromText(text);
}

QString EventFilterDoubleSpinBox::textFromValue(double val) const
{
    if (fabs(std::numeric_limits<DAVA::float32>::max() - val) < DAVA::EPSILON)
    {
        return EventFilterDoubleSpinBoxDetail::infinityStrValue;
    }

    return QDoubleSpinBox::textFromValue(val);
}

QPalette EventFilterDoubleSpinBox::GetTextPalette() const
{
    if (lineEdit())
    {
        return lineEdit()->palette();
    }

    return QPalette();
}

void EventFilterDoubleSpinBox::SetTextPalette(const QPalette& palette)
{
    if (lineEdit())
    {
        return lineEdit()->setPalette(palette);
    }
}
