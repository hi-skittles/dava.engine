#ifndef PALETTEHELPER_H
#define PALETTEHELPER_H

#include <QObject>
#include <QColor>
#include <QImage>

class PaintingHelper
{
public:
    static QImage DrawHSVImage(const QSize& size);
    static QImage DrawGradient(const QSize& size, const QColor& c1, const QColor& c2, Qt::Orientation orientation);
    static QBrush DrawGridBrush(const QSize& size);
    static QImage DrawArrowIcon(const QSize& size, Qt::Edge dimension, const QColor& bgColor = Qt::lightGray, const QColor& brdColor = Qt::gray);

    static QPoint GetHSVColorPoint(const QColor& c, const QSize& size);
    static int HueRC(const QPoint& pt, const QSize& size);
    static int SatRC(const QPoint& pt, const QSize& size);
    static int ValRC(const QPoint& pt, const QSize& size);

    static QColor MinColorComponent(const QColor& color, char component);
    static QColor MaxColorComponent(const QColor& color, char component);
};

#endif // PALETTEHELPER_H
