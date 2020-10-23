#include "HangingObjectsHeight.h"

#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include "DAVAEngine.h"

#include <QObject>
#include <QHBoxLayout>
#include <QLabel>

using namespace DAVA;

HangingObjectsHeight::HangingObjectsHeight(QWidget* parent /*= 0*/)
    : QWidget(parent)
{
    heightValue = new EventFilterDoubleSpinBox(this);
    heightValue->setToolTip("Min height for hanging objects");
    heightValue->setMinimum(-100);
    heightValue->setMaximum(100);
    heightValue->setSingleStep(0.01);
    heightValue->setDecimals(4);

    QLabel* caption = new QLabel("Min height:", this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);

    layout->addWidget(caption);
    layout->addWidget(heightValue);

    QObject::connect(heightValue, SIGNAL(valueChanged(double)), this, SLOT(ValueChanged(double)));
}

void HangingObjectsHeight::SetHeight(DAVA::float32 value)
{
    heightValue->setValue(value);
}

void HangingObjectsHeight::ValueChanged(double value)
{
    emit HeightChanged(value);
}
