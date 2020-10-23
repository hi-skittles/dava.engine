#include "Classes/Qt/DockParticleEditor/ParticleVector3Widget.h"

#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>

#include <limits>

ParticleVector3Widget::ParticleVector3Widget(const std::string& label, const DAVA::Vector3& initVector)
{
    QVBoxLayout* boxLayout = new QVBoxLayout(this);
    QHBoxLayout* dataLayout = new QHBoxLayout();

    QGroupBox* groupBox = new QGroupBox();

    groupBox->setTitle(label.c_str());
    groupBox->setCheckable(false);
    groupBox->setLayout(dataLayout);

    QLabel* xLabel = new QLabel("X:");
    QLabel* yLabel = new QLabel("Y:");
    QLabel* zLabel = new QLabel("Z:");

    xSpin = new EventFilterDoubleSpinBox();
    ySpin = new EventFilterDoubleSpinBox();
    zSpin = new EventFilterDoubleSpinBox();

    InitSpinBox(xSpin, initVector.x);
    InitSpinBox(ySpin, initVector.y);
    InitSpinBox(zSpin, initVector.z);

    dataLayout->addWidget(xLabel);
    dataLayout->addWidget(xSpin);

    dataLayout->addWidget(yLabel);
    dataLayout->addWidget(ySpin);

    dataLayout->addWidget(zLabel);
    dataLayout->addWidget(zSpin);

    boxLayout->addWidget(groupBox);
    setLayout(boxLayout);
}

DAVA::Vector3 ParticleVector3Widget::GetValue() const
{
    return { static_cast<DAVA::float32>(xSpin->value()), static_cast<DAVA::float32>(ySpin->value()), static_cast<DAVA::float32>(zSpin->value()) };
}

void ParticleVector3Widget::SetValue(const DAVA::Vector3& value)
{
    xSpin->setValue(value.x);
    ySpin->setValue(value.y);
    zSpin->setValue(value.z);
}

void ParticleVector3Widget::OnValueChanged()
{
    emit valueChanged();
}

void ParticleVector3Widget::InitSpinBox(EventFilterDoubleSpinBox* spin, DAVA::float32 value)
{
    spin->setMinimum(-std::numeric_limits<DAVA::float32>::max());
    spin->setMaximum(std::numeric_limits<DAVA::float32>::max());
    spin->setSingleStep(0.001);
    spin->setDecimals(4);
    spin->setValue(value);
    connect(spin, SIGNAL(valueChanged(double)), this, SLOT(OnValueChanged()));
    spin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}
