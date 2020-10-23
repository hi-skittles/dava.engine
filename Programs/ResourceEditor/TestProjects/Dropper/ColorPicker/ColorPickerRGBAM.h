#ifndef COLORPICKERRGBAM_H
#define COLORPICKERRGBAM_H

#include <QPointer>

#include "AbstractColorPicker.h"

class ColorComponentSlider;

class ColorPickerRGBAM
: public AbstractColorPicker
{
    Q_OBJECT

public:
    explicit ColorPickerRGBAM(QWidget* parent = NULL);
    ~ColorPickerRGBAM();

private slots:
    void OnChanging(double val);

private:
    void SetColorInternal(QColor const& c) override;
    void UpdateColorInternal(ColorComponentSlider* source = NULL);

    QLayout* CreateSlider(const QString& text, ColorComponentSlider* w) const;

    QPointer<ColorComponentSlider> r;
    QPointer<ColorComponentSlider> g;
    QPointer<ColorComponentSlider> b;
    QPointer<ColorComponentSlider> a;
    QPointer<ColorComponentSlider> m;
};


#endif // COLORPICKERRGBAM_H
