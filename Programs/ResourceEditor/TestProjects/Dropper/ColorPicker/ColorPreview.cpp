#include "ColorPreview.h"

#include <QPainter>

#include "../Helpers/PaintingHelper.h"

ColorPreview::ColorPreview(QWidget* parent)
    : QWidget(parent)
    , bgBrush(PaintingHelper::DrawGridBrush(QSize(7, 7)))
{
}

ColorPreview::~ColorPreview()
{
}

void ColorPreview::SetColorOld(const QColor& c)
{
    cOld = c;
    repaint();
}

void ColorPreview::SetColorNew(const QColor& c)
{
    cNew = c;
    repaint();
}

void ColorPreview::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);

    QColor cOldS(cOld);
    cOldS.setAlpha(255);
    QColor cNewS(cNew);
    cNewS.setAlpha(255);

    const int x1 = 0;
    const int y1 = 0;
    const int x2 = width() / 2;
    const int y2 = height() / 2;
    const int w = x2;
    const int h = y2;

    p.fillRect(x2, y1, w, height() - 1, bgBrush);
    p.fillRect(x1, y1, w, h, cOldS);
    p.fillRect(x2, y1, w, h, cOld);
    p.fillRect(x1, y2, w, h, cNewS);
    p.fillRect(x2, y2, w, h, cNew);

    p.setPen(Qt::black);
    p.drawRect(x1, y1, width() - 1, height() - 1);
}
