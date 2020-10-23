#ifndef PALETTEHELPER_H
#define PALETTEHELPER_H

#include <QObject>
#include <QColor>
#include <QImage>

class PaintingHelper
{
public:
    static QImage BuildHSVImage(const QSize& size);
    static QImage BuildGradient(const QSize& size, const QColor& c1, const QColor& c2, bool hor, bool ver);
    static QBrush BuildGridBrush(const QSize& size);
    static QImage BuildArrowIcon(const QSize& size, Qt::Edge dimension, const QColor& color = Qt::gray);

private:
    static int hue(const QPoint& pt, const QSize& size);
    static int sat(const QPoint& pt, const QSize& size);
    static int val(const QPoint& pt, const QSize& size);
};

#endif // PALETTEHELPER_H
