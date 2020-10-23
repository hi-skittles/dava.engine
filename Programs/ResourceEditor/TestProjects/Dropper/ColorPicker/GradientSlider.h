#ifndef GRADIENTSLIDER_H
#define GRADIENTSLIDER_H

#include <QWidget>
#include <QMap>

#include "../Widgets/AbstractSlider.h"

class GradientSlider
: public AbstractSlider
{
    Q_OBJECT

public:
    explicit GradientSlider(QWidget* parent = NULL);
    ~GradientSlider();

    void SetColors(const QColor& c1, const QColor& c2);
    void SetDimensions(const Qt::Edges& flags);
    void SetOrientation(Qt::Orientation orientation);
    void SetOffsets(int l, int t, int r, int b);

    double GetValue() const;
    void SetValue(double val);

protected:
    void DrawBackground(QPainter* p) const override;
    void DrawForeground(QPainter* p) const override;
    QRect PosArea() const override;

    void resizeEvent(QResizeEvent* e) override;

private:
    void drawArrow(Qt::Edge arrow, QPainter* p) const;

    QColor c1;
    QColor c2;
    QSize arrowSize;
    Qt::Edges arrows;
    Qt::Orientation orientation;
    int ofsL;
    int ofsR;
    int ofsT;
    int ofsB;
    const QBrush bgBrush;
    mutable QPixmap bgCache;
    mutable QMap<Qt::Edge, QPixmap> arrowCache;
};


#endif // GRADIENTSLIDER_H
