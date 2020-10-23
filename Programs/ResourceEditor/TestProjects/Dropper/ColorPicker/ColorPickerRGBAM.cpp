#include "ColorPickerRGBAM.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "ColorComponentSlider.h"
#include "../Helpers/PaintingHelper.h"

ColorPickerRGBAM::ColorPickerRGBAM(QWidget* parent)
    : AbstractColorPicker(parent)
{
    r = new ColorComponentSlider();
    g = new ColorComponentSlider();
    b = new ColorComponentSlider();
    a = new ColorComponentSlider();
    m = new ColorComponentSlider();

    ColorComponentSlider* sliders[] = {
        r, g, b, a, m,
    };
    const QString labels[] = { "Red", "Green", "Blue", "Alpha", "Multiply" };

    QVBoxLayout* l = new QVBoxLayout();
    l->setMargin(0);
    l->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < sizeof(sliders) / sizeof(*sliders); i++)
    {
        l->addLayout(CreateSlider(labels[i], sliders[i]));

        connect(sliders[i], SIGNAL(changing(double)), SLOT(OnChanging(double)));
    }
    l->addStretch(1);

    setLayout(l);
}

ColorPickerRGBAM::~ColorPickerRGBAM()
{
}

void ColorPickerRGBAM::OnChanging(double val)
{
    Q_UNUSED(val);
    ColorComponentSlider* source = qobject_cast<ColorComponentSlider*>(sender());
    UpdateColorInternal(source);
    emit changing(GetColor());
}

void ColorPickerRGBAM::SetColorInternal(const QColor& c)
{
    color = c;
    r->SetValue(color.redF());
    g->SetValue(color.greenF());
    b->SetValue(color.blueF());
    a->SetValue(color.alphaF());
    m->SetColorRange(color, color);

    UpdateColorInternal();
}

void ColorPickerRGBAM::UpdateColorInternal(ColorComponentSlider* source)
{
    color.setRgbF(r->GetValue(), g->GetValue(), b->GetValue(), a->GetValue());

    if (source != r)
    {
        r->SetColorRange(PaintingHelper::MinColorComponent(color, 'r'), PaintingHelper::MaxColorComponent(color, 'r'));
        r->SetValue(color.redF());
    }
    if (source != g)
    {
        g->SetColorRange(PaintingHelper::MinColorComponent(color, 'g'), PaintingHelper::MaxColorComponent(color, 'g'));
        g->SetValue(color.greenF());
    }
    if (source != b)
    {
        b->SetColorRange(PaintingHelper::MinColorComponent(color, 'b'), PaintingHelper::MaxColorComponent(color, 'b'));
        b->SetValue(color.blueF());
    }
    if (source != a)
    {
        a->SetColorRange(PaintingHelper::MinColorComponent(color, 'a'), PaintingHelper::MaxColorComponent(color, 'a'));
        a->SetValue(color.alphaF());
    }
    if (source != m)
    {
        m->SetColorRange(color, color);
    }
}

QLayout* ColorPickerRGBAM::CreateSlider(const QString& text, ColorComponentSlider* w) const
{
    QHBoxLayout* l = new QHBoxLayout();
    QLabel* txt = new QLabel(text);
    txt->setMinimumSize(40, 0);
    txt->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    l->addWidget(txt);
    l->addWidget(w);

    return l;
}