#include "ColorComponentSlider.h"

#include <QVBoxLayout>
#include <QPainter>

#include "../Widgets/ValueSlider.h"
#include "GradientSlider.h"

ColorComponentSlider::ColorComponentSlider(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* l = new QVBoxLayout();
    l->setMargin(0);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    value = new ValueSlider();
    value->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    value->setMinimumSize(0, 20);
    l->addWidget(value);

    gradient = new GradientSlider();
    gradient->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    gradient->setMinimumSize(0, 5);
    l->addWidget(gradient);

    setLayout(l);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    updateGeometry();

    connect(value, SIGNAL(started(double)), SIGNAL(started(double)));
    connect(value, SIGNAL(changing(double)), SIGNAL(changing(double)));
    connect(value, SIGNAL(changed(double)), SIGNAL(changed(double)));
    connect(value, SIGNAL(canceled()), SIGNAL(canceled()));
}

ColorComponentSlider::~ColorComponentSlider()
{
}

void ColorComponentSlider::SetColorRange(QColor const& c1, QColor const& c2)
{
    gradient->SetColors(c1, c2);
}

void ColorComponentSlider::SetValue(double val)
{
    value->SetValue(val);
}

double ColorComponentSlider::GetValue() const
{
    return value->GetValue();
}