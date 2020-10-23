#include "PaintingHelper.h"

#include <QPainter>
#include <QLinearGradient>

QImage PaintingHelper::DrawHSVImage(QSize const& size)
{
    QImage img(size.width(), size.height(), QImage::Format_RGB32);

    uint* p = reinterpret_cast<uint*>(img.scanLine(0));

    if (size.width() == 1 || size.height() == 1)
    {
        img.fill(Qt::black);
    }
    else
    {
        for (int y = 0; y < size.height(); y++)
        {
            const uint* end = p + size.width();
            int x = 0;
            while (p < end)
            {
                const QPoint pt(x, y);
                QColor c;
                c.setHsv(HueRC(pt, size), SatRC(pt, size), ValRC(pt, size));
                *p = c.rgb();
                p++;
                x++;
            }
        }
    }

    return img;
}

QPoint PaintingHelper::GetHSVColorPoint(const QColor& c, const QSize& size)
{
    int h, s, v, a;
    c.getHsv(&h, &s, &v, &a);

    const int x = (360 - h) * (size.width() - 1) / 360;
    const int y = (360 - s) * (size.height() - 1) / 360;

    return QPoint(x, y);
}

int PaintingHelper::HueRC(QPoint const& pt, QSize const& size)
{
    return 360 - pt.x() * 360 / (size.width() - 1);
}

int PaintingHelper::SatRC(QPoint const& pt, QSize const& size)
{
    return 255 - pt.y() * 255 / (size.height() - 1);
}

int PaintingHelper::ValRC(QPoint const& pt, QSize const& size)
{
    Q_UNUSED(pt);
    Q_UNUSED(size);

    return 220;
}

QImage PaintingHelper::DrawGradient(const QSize& size, const QColor& c1, const QColor& c2, Qt::Orientation orientation)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    const qreal x1 = 0;
    const qreal y1 = 0;
    const qreal x2 = (orientation == Qt::Horizontal) ? size.width() : 0;
    const qreal y2 = (orientation == Qt::Vertical) ? size.height() : 0;

    QLinearGradient gradient(x1, y1, x2, y2);
    gradient.setColorAt(0.0, c1);
    gradient.setColorAt(1.0, c2);

    QPainter p(&img);
    p.fillRect(0, 0, size.width(), size.height(), QBrush(gradient));

    return img;
}

QBrush PaintingHelper::DrawGridBrush(QSize const& size)
{
    QImage img(size.width() * 2, size.height() * 2, QImage::Format_ARGB32);
    img.fill(Qt::white);

    QPainter p(&img);
    p.fillRect(QRect(0, 0, size.width(), size.height()), Qt::lightGray);
    p.fillRect(QRect(size.width(), size.height(), size.width(), size.height()), Qt::lightGray);

    return QBrush(img);
}

QImage PaintingHelper::DrawArrowIcon(QSize const& size, Qt::Edge dimension, const QColor& bgColor, const QColor& brdColor)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    const int x1 = 0;
    const int x2 = size.width() / 2;
    const int x3 = size.width() - 1;
    const int y1 = 0;
    const int y2 = size.height() / 2;
    const int y3 = size.height() - 1;

    QPoint point[3];

    switch (dimension)
    {
    case Qt::LeftEdge:
        point[0] = QPoint(x1, y1);
        point[1] = QPoint(x3, y2);
        point[2] = QPoint(x1, y3);
        break;
    case Qt::TopEdge:
        point[0] = QPoint(x1, y1);
        point[1] = QPoint(x2, y3);
        point[2] = QPoint(x3, y1);
        break;
    case Qt::RightEdge:
        point[0] = QPoint(x1, y2);
        point[1] = QPoint(x3, y1);
        point[2] = QPoint(x3, y3);
        break;
    case Qt::BottomEdge:
        point[0] = QPoint(x1, y3);
        point[1] = QPoint(x2, y1);
        point[2] = QPoint(x3, y3);
        break;
    default:
        return img;
    }

    QPainterPath path;
    path.moveTo(point[0]);
    path.lineTo(point[1]);
    path.lineTo(point[2]);
    path.lineTo(point[0]);

    QPainter p(&img);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    p.fillPath(path, bgColor);
    p.setPen(brdColor);
    p.drawPath(path);

    return img;
}

QColor PaintingHelper::MinColorComponent(QColor const& color, char component)
{
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();

    switch (component)
    {
    case 'r':
        r = 0;
        a = 255;
        break;
    case 'g':
        g = 0;
        a = 255;
        break;
    case 'b':
        b = 0;
        a = 255;
        break;
    case 'a':
        a = 0;
        break;
    }

    return QColor(r, g, b, a);
}

QColor PaintingHelper::MaxColorComponent(QColor const& color, char component)
{
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();

    switch (component)
    {
    case 'r':
        r = 255;
        a = 255;
        break;
    case 'g':
        g = 255;
        a = 255;
        break;
    case 'b':
        b = 255;
        a = 255;
        break;
    case 'a':
        a = 255;
        break;
    }

    return QColor(r, g, b, a);
}
