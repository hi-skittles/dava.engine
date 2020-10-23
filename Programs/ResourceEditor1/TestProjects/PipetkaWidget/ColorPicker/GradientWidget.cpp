#include "GradientWidget.h"

#include <QPainter>
#include "PaintingHelper.h"

GradientWidget::GradientWidget(QWidget* parent)
    : QWidget(parent)
    , hor(true)
    , ver(false)
    , fillBg(true)
    , gridSize(6, 6)
{
}

GradientWidget::~GradientWidget()
{
}

void GradientWidget::SetColorRange(QColor const& start, QColor const& stop)
{
    startColor = start;
    stopColor = stop;
}

void GradientWidget::SetRenderDimensions(bool _hor, bool _ver)
{
    if (hor != _hor || ver != _ver)
    {
        hor = _hor;
        ver = _ver;

        cacheBg = QPixmap();
        update();
    }
}

void GradientWidget::SetBgPadding(int left, int top, int right, int bottom)
{
    paddingOfs.left = left;
    paddingOfs.top = top;
    paddingOfs.right = right;
    paddingOfs.bottom = bottom;
}

void GradientWidget::SetGrid(bool enabled, const QSize& _size)
{
    if (_size == QSize())
    {
        enabled = false;
    }

    const bool reset = (enabled != fillBg || _size != gridSize);
    fillBg = enabled;
    gridSize = _size;
    if (reset)
    {
        cacheBg = QPixmap();
        update();
    }
}

QColor GradientWidget::GetColorAt(QPoint const& pos) const
{
    drawBackground(); // Refresh cache;
    QRgb data = cacheBgImage.pixel(pos);

    return QColor(data);
}

QPixmap GradientWidget::drawBackground() const
{
    if (cacheBg.isNull())
    {
        const int horPadding = paddingOfs.left + paddingOfs.right;
        const int verPadding = paddingOfs.top + paddingOfs.bottom;
        QSize actualSize(size().width() - horPadding, size().height() - verPadding);

        const QImage& bg = PaintingHelper::BuildGradient(actualSize, startColor, stopColor, hor, ver);
        QImage fullBg(actualSize, QImage::Format_ARGB32);
        fullBg.fill(Qt::transparent);

        const QRect rc(QPoint(0, 0), actualSize);
        QPainter p(&fullBg);
        if (fillBg)
        {
            const QBrush& bgBrush = PaintingHelper::BuildGridBrush(QSize(5, 5));
            p.fillRect(rc, bgBrush);
        }
        p.drawImage(QPoint(0, 0), bg);

        cacheBgImage = fullBg;
        cacheBg = QPixmap::fromImage(fullBg);
    }

    return cacheBg;
}

QPixmap GradientWidget::drawContent() const
{
    return QPixmap();
}

GradientWidget::Offset const& GradientWidget::padding() const
{
    return paddingOfs;
}

void GradientWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);
    const QPixmap& bg = drawBackground();
    const QPixmap& fg = drawContent();
    p.drawPixmap(paddingOfs.left, paddingOfs.top, bg);
    p.drawPixmap(0, 0, fg);
}

void GradientWidget::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    cacheBg = QPixmap();
}
