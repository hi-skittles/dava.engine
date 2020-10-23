#include "ColorPickerHSV.h"

#include <QHBoxLayout>

#include "PaletteHSV.h"
#include "GradientSlider.h"

ColorPickerHSV::ColorPickerHSV(QWidget* parent)
    : AbstractColorPicker(parent)
{
    QHBoxLayout* l = new QHBoxLayout();

    pal = new PaletteHSV(this);
    pal->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    l->addWidget(pal);

    val = new GradientSlider(this);
    val->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    val->setMinimumSize(30, 0);
    val->SetDimensions(Qt::LeftEdge | Qt::RightEdge);
    val->SetOrientation(Qt::Vertical);
    val->SetOffsets(5, 5, 5, 5);
    l->addWidget(val);

    alpha = new GradientSlider(this);
    alpha->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    alpha->setMinimumSize(30, 0);
    alpha->SetDimensions(Qt::LeftEdge | Qt::RightEdge);
    alpha->SetOrientation(Qt::Vertical);
    alpha->SetOffsets(5, 5, 5, 5);
    l->addWidget(alpha);

    setLayout(l);
    updateGeometry();

    QObject* editors[] = { pal, val, alpha };

    for (size_t i = 0; i < sizeof(editors) / sizeof(*editors); i++)
    {
        connect(editors[i], SIGNAL(started(const QPointF&)), SIGNAL(begin()));
        connect(editors[i], SIGNAL(started(const QPointF&)), SLOT(OnChanging()));
        connect(editors[i], SIGNAL(changing(const QPointF&)), SLOT(OnChanging()));
        connect(editors[i], SIGNAL(changed(const QPointF&)), SLOT(OnChanged()));
        connect(editors[i], SIGNAL(canceled()), SIGNAL(canceled()));
    }

    connect(pal, SIGNAL(started(const QPointF&)), SLOT(OnHS()));
    connect(pal, SIGNAL(changing(const QPointF&)), SLOT(OnHS()));
    connect(val, SIGNAL(started(const QPointF&)), SLOT(OnVal()));
    connect(val, SIGNAL(changing(const QPointF&)), SLOT(OnVal()));
}

ColorPickerHSV::~ColorPickerHSV()
{
}

void ColorPickerHSV::SetColorInternal(QColor const& c)
{
    QColor min;
    QColor max;
    int h, s, v, a;
    c.getHsv(&h, &s, &v, &a);

    pal->SetColor(h, s);

    min.setHsv(h, s, 0, 255);
    max.setHsv(h, s, 255, 255);
    val->SetColors(max, min);
    const double dv = 1.0 - double(v) / 255.0;
    val->SetValue(dv);

    min.setHsv(h, s, v, 0);
    max.setHsv(h, s, v, 255);
    alpha->SetColors(max, min);
    const double da = 1.0 - double(a) / 255.0;
    alpha->SetValue(da);
}

void ColorPickerHSV::OnChanging()
{
    UpdateColor();
    emit changing(GetColor());
}

void ColorPickerHSV::OnChanged()
{
    UpdateColor();
    emit changed(GetColor());
}

void ColorPickerHSV::OnHS()
{
    int h, s, v, a;
    const QColor c = GetColor();
    QColor min;
    QColor max;

    c.getHsv(&h, &s, &v, &a);

    min.setHsv(h, s, 0, 255);
    max.setHsv(h, s, 255, 255);
    val->SetColors(max, min);

    min.setHsv(h, s, v, 0);
    max.setHsv(h, s, v, 255);
    alpha->SetColors(max, min);
}

void ColorPickerHSV::OnVal()
{
    int h, s, v, a;
    const QColor c = GetColor();
    QColor min;
    QColor max;

    c.getHsv(&h, &s, &v, &a);

    min.setHsv(h, s, v, 0);
    max.setHsv(h, s, v, 255);
    alpha->SetColors(max, min);
}

void ColorPickerHSV::OnAlpha()
{
}

void ColorPickerHSV::UpdateColor()
{
    const int h = pal->GetHue();
    const int s = pal->GetSat();
    const int v = int(255 - val->GetValue() * 255); // HSV, max V = 255
    const int a = int(255 - alpha->GetValue() * 255); // Transparency, max = 255

    QColor c;
    c.setHsv(h, s, v, a);
    color = c;
}
