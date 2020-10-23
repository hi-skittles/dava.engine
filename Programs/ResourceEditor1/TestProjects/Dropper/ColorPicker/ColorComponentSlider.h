#ifndef COLORCOMPONENTSLIDER_H
#define COLORCOMPONENTSLIDER_H


#include <QWidget>
#include <QPointer>

class ValueSlider;
class GradientSlider;

class ColorComponentSlider
: public QWidget
{
    Q_OBJECT

signals:
    void started(double);
    void changing(double);
    void changed(double);
    void canceled();

public:
    explicit ColorComponentSlider(QWidget* parent = NULL);
    ~ColorComponentSlider();

    void SetColorRange(const QColor& c1, const QColor& c2);
    void SetValue(double val);
    double GetValue() const;

private:
    QPointer<ValueSlider> value;
    QPointer<GradientSlider> gradient;
};


#endif // COLORCOMPONENTSLIDER_H
