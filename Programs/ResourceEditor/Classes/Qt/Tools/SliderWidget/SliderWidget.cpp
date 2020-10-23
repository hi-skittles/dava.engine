#include "SliderWidget.h"
#include "PopupEditorDialog.h"

#include "REPlatform/Global/StringConstants.h"

#include <DAVAEngine.h>

#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>

const int SliderWidget::DEF_LOWEST_VALUE = -999;
const int SliderWidget::DEF_HIGHEST_VALUE = 999;

SliderWidget::SliderWidget(QWidget* parent)
    : QWidget(parent)
    , isRangeChangingBlocked(true)
    , rangeBoundMin(DEF_LOWEST_VALUE)
    , rangeBoundMax(DEF_HIGHEST_VALUE)
{
    InitUI();
    Init(true, 10, 0, 0);
    SetRangeVisible(true);
    SetRangeChangingBlocked(false);

    ConnectToSignals();
}

SliderWidget::~SliderWidget()
{
}

void SliderWidget::InitUI()
{
    QHBoxLayout* layout = new QHBoxLayout();
    labelMinValue = new QLabel(this);
    labelMaxValue = new QLabel(this);
    sliderValue = new QSlider(this);
    spinCurValue = new QSpinBox(this);

    layout->addWidget(labelMinValue);
    layout->addWidget(sliderValue);
    layout->addWidget(labelMaxValue);
    layout->addWidget(spinCurValue);

    setLayout(layout);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    sliderValue->setOrientation(Qt::Horizontal);
    sliderValue->setTickPosition(QSlider::TicksBothSides);
    spinCurValue->setToolTip(DAVA::ResourceEditor::SLIDER_WIDGET_CURRENT_VALUE.c_str());
}

void SliderWidget::ConnectToSignals()
{
    connect(sliderValue, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
    connect(spinCurValue, SIGNAL(valueChanged(int)), this, SLOT(SpinValueChanged(int)));
}

void SliderWidget::Init(bool symmetric, int max, int min, int value)
{
    isSymmetric = symmetric;
    currentValue = value;
    SetRange(min, max);

    UpdateControls();
}

void SliderWidget::SetRange(int min, int max)
{
    if (min > max)
    {
        qSwap(max, min);
    }

    maxValue = qMin(max, rangeBoundMax);
    minValue = qMax(min, rangeBoundMin);
    if (IsSymmetric())
    {
        minValue = -max;
        if (min > max)
        {
            qSwap(max, min);
        }
    }

    if (currentValue > maxValue)
    {
        SetValue(maxValue);
    }
    else if (currentValue < minValue)
    {
        SetValue(minValue);
    }
}

void SliderWidget::SetRangeMax(int max)
{
    SetRange(minValue, max);
    UpdateControls();
}

int SliderWidget::GetRangeMax()
{
    return maxValue;
}

void SliderWidget::SetRangeMin(int min)
{
    SetRange(min, maxValue);
    UpdateControls();
}

int SliderWidget::GetRangeMin()
{
    return minValue;
}

void SliderWidget::SetSymmetric(bool symmetric)
{
    isSymmetric = symmetric;

    if (IsSymmetric())
    {
    }

    SetRangeBoundaries(rangeBoundMin, rangeBoundMax);
    UpdateControls();
}

bool SliderWidget::IsSymmetric()
{
    return isSymmetric;
}

void SliderWidget::SetValue(int value)
{
    if (value == currentValue)
    {
        return;
    }

    value = qMax(minValue, value);
    value = qMin(maxValue, value);

    currentValue = value;
    EmitValueChanged();
    UpdateControls();
}

int SliderWidget::GetValue()
{
    return currentValue;
}

void SliderWidget::SliderValueChanged(int newValue)
{
    SetValue(newValue);
}

void SliderWidget::RangeChanged(int newMinValue, int newMaxValue)
{
    SetRange(newMinValue, newMaxValue);
}

void SliderWidget::EmitValueChanged()
{
    sliderValue->setToolTip(QString::number(currentValue));
    emit ValueChanged(currentValue);
}

void SliderWidget::UpdateControls()
{
    bool blocked = signalsBlocked();
    blockSignals(true);

    labelMinValue->setNum(GetRangeMin());
    labelMaxValue->setNum(GetRangeMax());

    spinCurValue->setRange(GetRangeMin(), GetRangeMax());
    spinCurValue->setValue(GetValue());

    sliderValue->setRange(GetRangeMin(), GetRangeMax());
    sliderValue->setValue(GetValue());

    blockSignals(blocked);
}

void SliderWidget::SetRangeChangingBlocked(bool blocked)
{
    if (blocked == isRangeChangingBlocked)
    {
        return;
    }

    if (blocked)
    {
        labelMinValue->removeEventFilter(this);
        labelMaxValue->removeEventFilter(this);
        labelMinValue->setToolTip("");
        labelMaxValue->setToolTip("");
    }
    else
    {
        labelMinValue->installEventFilter(this);
        labelMaxValue->installEventFilter(this);
        labelMinValue->setToolTip(DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_TOOLTIP.c_str());
        labelMaxValue->setToolTip(DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_TOOLTIP.c_str());
    }

    isRangeChangingBlocked = blocked;
}

bool SliderWidget::IsRangeChangingBlocked()
{
    return isRangeChangingBlocked;
}

void SliderWidget::SpinValueChanged(int newValue)
{
    SetValue(spinCurValue->value());
}

bool SliderWidget::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == labelMinValue || obj == labelMaxValue)
    {
        if (ev->type() == QEvent::MouseButtonDblClick)
        {
            int val = (obj == labelMinValue) ? minValue : maxValue;

            QLabel* label = (QLabel*)obj;
            PopupEditorDialog* dialog = new PopupEditorDialog(val, rangeBoundMin, rangeBoundMax, label, this);

            QPoint pos = label->mapToGlobal(QPoint(0, 0));
            dialog->move(pos.x(), pos.y());
            dialog->setMinimumHeight(label->height());

            connect(dialog, SIGNAL(ValueReady(const QWidget*, int)),
                    this, SLOT(OnValueReady(const QWidget*, int)));

            dialog->show();
            //dialog will self-delete after close

            return true;
        }
    }

    return QObject::eventFilter(obj, ev);
}

void SliderWidget::OnValueReady(const QWidget* widget, int value)
{
    QLabel* label = (QLabel*)widget;
    label->setNum(value);

    int min = minValue;
    int max = maxValue;
    if (label == labelMinValue)
    {
        min = qMax(value, DEF_LOWEST_VALUE);
        if (IsSymmetric())
        {
            max = -min;
        }
    }
    else if (label == labelMaxValue)
    {
        max = qMin(value, DEF_HIGHEST_VALUE);
    }
    SetRange(min, max);
    UpdateControls();
}

void SliderWidget::SetRangeVisible(bool visible)
{
    if (visible == isRangeVisible)
    {
        return;
    }

    labelMaxValue->setVisible(visible);
    labelMinValue->setVisible(visible);

    isRangeVisible = visible;
}

bool SliderWidget::IsRangeVisible()
{
    return isRangeVisible;
}

void SliderWidget::SetRangeBoundaries(int min, int max)
{
    if (IsSymmetric())
    {
        min = -max;
    }

    if (min > max)
    {
        qSwap(min, max);
    }

    rangeBoundMax = max;
    rangeBoundMin = min;

    SetRange(minValue, maxValue);
    UpdateControls();
}

void SliderWidget::SetCurValueVisible(bool visible)
{
    spinCurValue->setVisible(visible);
}

bool SliderWidget::IsCurValueVisible()
{
    return spinCurValue->isVisible();
}
