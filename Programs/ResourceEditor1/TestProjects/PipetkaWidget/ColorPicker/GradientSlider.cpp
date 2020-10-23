#include "GradientSlider.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QMessageBox>

#include <QDebug>
#include "PaintingHelper.h"

GradientSlider::GradientSlider(QWidget* parent)
    : GradientWidget(parent)
    , arrowSize(9, 9)
    , mouse(new MouseHelper(this))
{
    connect(mouse, SIGNAL(mousePress(const QPoint&)), SLOT(onMousePress(const QPoint&)));
    connect(mouse, SIGNAL(mouseMove(const QPoint&)), SLOT(onMouseMove(const QPoint&)));
    connect(mouse, SIGNAL(mouseRelease(const QPoint&)), SLOT(onMouseRelease(const QPoint&)));
    connect(mouse, SIGNAL(clicked()), SLOT(onClick()));
}

GradientSlider::~GradientSlider()
{
}

void GradientSlider::setEditorDimensions(Qt::Edges flags)
{
    arrows = flags;
    update();
}

void GradientSlider::setPrefferableArrows()
{
    const int paddingBase = padding().left + padding().right + padding().top + padding().bottom;
    const int paddingSum = paddingBase < 27 ? paddingBase : 27;
    const int d = paddingSum * 3 / 7;
    //arrowSize = QSize( d, d );
    arrowCache.clear();
}

QPointF GradientSlider::GetPosF() const
{
    const QSize size(width() - padding().left - padding().right, height() - padding().top - padding().bottom);
    const QPoint relPos(currentPos.x() + padding().left, currentPos.y() + padding().top);
    const QPointF pos(double(relPos.x()) / double(size.width()), double(relPos.y()) / double(size.height()));

    return pos;
}

QColor GradientSlider::GetColor() const
{
    const QPoint relPos(currentPos.x() - padding().left, currentPos.y() - padding().top);
    return GetColorAt(relPos);
}

void GradientSlider::setColor(const QColor& c)
{
}

QPixmap GradientSlider::drawContent() const
{
    QPixmap buf(size());
    buf.fill(Qt::transparent);

    QPainter p(&buf);
    drawArrows(&p);

    return buf;
}

void GradientSlider::resizeEvent(QResizeEvent* e)
{
    GradientWidget::resizeEvent(e);
}

void GradientSlider::onMousePress(const QPoint& pos)
{
    setPos(pos);
    startColor = GetColor();
    emit begin();
    emit changing(GetColor());
}

void GradientSlider::onMouseMove(QPoint const& pos)
{
    if (mouse->IsPressed())
    {
        setPos(pos);
        emit changing(GetColor());
    }
}

void GradientSlider::onMouseRelease(const QPoint& pos)
{
    setPos(pos);
    const QColor endColor = GetColor();
    if (startColor != endColor)
    {
        emit changed(endColor);
    }
    else
    {
        emit canceled();
    }
}

void GradientSlider::onClick()
{
    QMessageBox::information(this, "Test", QString());
}

QPoint GradientSlider::fitInBackground(QPoint const& pos) const
{
    const QRect rc = QRect(0, 0, width(), height()).adjusted(padding().left, padding().top, -padding().right, -padding().bottom);
    QPoint pt = pos;

    if (!rc.contains(pt))
    {
        if (pt.x() < rc.left())
        {
            pt.setX(rc.left());
        }
        if (pt.x() > rc.right())
        {
            pt.setX(rc.right());
        }
        if (pt.y() < rc.top())
        {
            pt.setY(rc.top());
        }
        if (pt.y() > rc.bottom())
        {
            pt.setY(rc.bottom());
        }
    }

    return pt;
}

void GradientSlider::setPos(QPoint const& pos)
{
    currentPos = fitInBackground(pos);
    repaint();
}

void GradientSlider::drawArrows(QPainter* p) const
{
    p->save();

    if (arrows.testFlag(Qt::TopEdge))
        drawArrow(Qt::TopEdge, p);
    if (arrows.testFlag(Qt::LeftEdge))
        drawArrow(Qt::LeftEdge, p);
    if (arrows.testFlag(Qt::RightEdge))
        drawArrow(Qt::RightEdge, p);
    if (arrows.testFlag(Qt::BottomEdge))
        drawArrow(Qt::BottomEdge, p);

    p->restore();
}

void GradientSlider::drawArrow(Qt::Edge arrow, QPainter* p) const
{
    const auto it = arrowCache.constFind(arrow);
    if (it == arrowCache.constEnd())
    {
        arrowCache[arrow] = QPixmap::fromImage(PaintingHelper::BuildArrowIcon(arrowSize, arrow, Qt::lightGray));
    }

    QPoint pos;

    switch (arrow)
    {
    case Qt::TopEdge:
        pos.setX(currentPos.x() - arrowSize.width() / 2);
        pos.setY(0);
        break;
    case Qt::LeftEdge:
        pos.setX(0);
        pos.setY(currentPos.y() - arrowSize.height() / 2);
        break;
    case Qt::RightEdge:
        pos.setX(width() - arrowSize.width() + 1);
        pos.setY(currentPos.y() - arrowSize.height() / 2);
        break;
    case Qt::BottomEdge:
        pos.setX(currentPos.x() - arrowSize.width() / 2);
        pos.setY(height() - arrowSize.height() + 1);
        break;
    default:
        return;
    }

    QRect rc(pos, arrowSize);
    p->drawPixmap(pos, arrowCache[arrow]);
}
