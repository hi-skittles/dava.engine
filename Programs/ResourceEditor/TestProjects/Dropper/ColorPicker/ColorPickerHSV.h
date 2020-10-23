#ifndef COLORPICKERHSV_H
#define COLORPICKERHSV_H

#include <QPointer>

#include "AbstractColorPicker.h"

class PaletteHSV;
class GradientSlider;

class ColorPickerHSV
: public AbstractColorPicker
{
    Q_OBJECT

public:
    explicit ColorPickerHSV(QWidget* parent = NULL);
    ~ColorPickerHSV();

protected:
    void SetColorInternal(const QColor& c) override;

private slots:
    void OnChanging();
    void OnChanged();

    void OnHS();
    void OnVal();
    void OnAlpha();

private:
    void UpdateColor();

    QPointer<PaletteHSV> pal;
    QPointer<GradientSlider> val;
    QPointer<GradientSlider> alpha;
};


#endif // COLORPICKERHSV_H
