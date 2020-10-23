#include "ScrollZoomWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include <Base/Introspection.h>

ScrollZoomWidget::ScrollZoomWidget(QWidget* parent)
    : QWidget(parent)
{
    minValue = std::numeric_limits<DAVA::float32>::infinity();
    maxValue = -std::numeric_limits<DAVA::float32>::infinity();
    minValueLimit = -std::numeric_limits<DAVA::float32>::infinity();
    maxValueLimit = std::numeric_limits<DAVA::float32>::infinity();
    minTime = 0.0;
    maxTime = 1;
    generalMinTime = minTime;
    generalMaxTime = maxTime;
    initialTimeInterval = 1;

    gridStyle = GRID_STYLE_LIMITS;

    horizontalScrollBar = new QScrollBar(Qt::Horizontal, this);
    connect(horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(HandleHorizontalScrollChanged(int)));
    zoomSlider = new QSlider(Qt::Horizontal, this);
    connect(zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(HandleZoomScrollChanged(int)));
    //UpdateSizePolicy();

    zoomSlider->setStyleSheet(
    "QSlider::groove:horizontal {"
    "border: 1px none;"
    "height: 4px;"
    "background: palette(midlight);"
    "margin: 3px 0;"
    "}"
    "QSlider::handle:horizontal {"
    "background: palette(window);"
    "border: 1px solid #999999;"
    "width: 4px;"
    "margin: -3px 0;"
    "border-radius: 2px;"
    "}");

    scale = 1.0f;
}

ScrollZoomWidget::~ScrollZoomWidget()
{
    disconnect(horizontalScrollBar, SIGNAL(sliderMoved(int)), this, SLOT(HandleHorizontalScrollChanged(int)));
    delete horizontalScrollBar;

    disconnect(zoomSlider, SIGNAL(sliderMoved(int)), this, SLOT(HandleZoomScrollChanged(int)));
    delete zoomSlider;
}

void ScrollZoomWidget::Init(DAVA::float32 minT, DAVA::float32 maxT)
{
    this->minTime = minT;
    this->maxTime = maxT;

    this->generalMinTime = minT;
    this->generalMaxTime = maxT;

    this->initialTimeInterval = generalMaxTime - generalMinTime;
    this->scale = 1.0f;

    UpdateScrollBarSlider();
    UpdateZoomSlider();
}

QString ScrollZoomWidget::float2QString(DAVA::float32 value) const
{
    QString strValue;
    if (fabs(value) < 10)
        strValue = "%.2f";
    else if (fabs(value) < 100)
        strValue = "%.1f";
    else
        strValue = "%.0f";
    strValue.sprintf(strValue.toLatin1(), value);
    return strValue;
}

void ScrollZoomWidget::paintEvent(QPaintEvent* /*paintEvent*/)
{
    //draw scroll bar
    UpdateScrollBarPosition();

    //draw slider
    UpdateSliderPosition();

    QPainter painter(this);
    painter.setPen(Qt::black);

    //draw increase box
    {
        painter.setPen(Qt::black);
        QRect increaseRect = GetIncreaseRect();
        painter.drawRect(increaseRect);
        int increaseRectX1;
        int increaseRectY1;
        int increaseRectX2;
        int increaseRectY2;
        increaseRect.getRect(&increaseRectX1, &increaseRectY1, &increaseRectX2, &increaseRectY2);
        int vertLineX1 = increaseRectX1 + increaseRect.width() / 2;
        int vertLineY1 = increaseRectY1;
        int vertLineX2 = vertLineX1;
        int vertLineY2 = increaseRectY1 + increaseRectY2;
        painter.drawLine(vertLineX1, vertLineY1, vertLineX2, vertLineY2);
        int horLineX1 = increaseRectX1;
        int horLineY1 = increaseRectY1 + increaseRect.height() / 2;
        int horLineX2 = increaseRectX1 + increaseRectX2;
        int horLineY2 = horLineY1;
        painter.drawLine(horLineX1, horLineY1, horLineX2, horLineY2);
    }

    //draw scale value
    {
        char scaleChar[10];
        sprintf(scaleChar, "%.0f", scale * 100);
        QString scale(scaleChar);
        painter.setPen(Qt::black);
        QRect scaleRect(GetScaleRect());
        scaleRect.setWidth(SCALE_WIDTH);
        painter.drawText(scaleRect.bottomLeft(), scale);
    }

    //draw decrease box
    {
        painter.setPen(Qt::black);
        QRect decreaseRect = GetDecreaseRect();
        painter.drawRect(decreaseRect);
        int decreaseRectX1;
        int decreaseRectY1;
        int decreaseRectX2;
        int decreaseRectY2;
        decreaseRect.getRect(&decreaseRectX1, &decreaseRectY1, &decreaseRectX2, &decreaseRectY2);
        int horLineX1 = decreaseRectX1;
        int horLineY1 = decreaseRectY1 + decreaseRect.height() / 2;
        int horLineX2 = decreaseRectX1 + decreaseRectX2;
        int horLineY2 = horLineY1;
        painter.drawLine(horLineX1, horLineY1, horLineX2, horLineY2);
    }
}

void ScrollZoomWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mouseStartPos.x() != 0)
    {
        PerformOffset(mouseStartPos.x() - event->pos().x());
        mouseStartPos = event->pos();
    }
}

void ScrollZoomWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    setFocus();
    mouseStartPos = event->pos();
    if (event->button() == Qt::LeftButton)
    {
        if (GetIncreaseRect().contains(event->pos()))
        {
            PerformZoom(scale + ZOOM_STEP);
            return;
        }
        else if (GetDecreaseRect().contains(event->pos()))
        {
            PerformZoom(scale - ZOOM_STEP);
            return;
        }
    }
    else
    {
        mouseStartPos.setX(0);
    }
}

void ScrollZoomWidget::mouseReleaseEvent(QMouseEvent* e)
{
    QWidget::mouseReleaseEvent(e);
    setFocus();
    mouseStartPos.setX(0);
}

DAVA::float32 ScrollZoomWidget::GetMinBoundary()
{
    return minTime;
}

DAVA::float32 ScrollZoomWidget::GetMaxBoundary()
{
    return maxTime;
}

void ScrollZoomWidget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier)
    {
        // get wheel steps according qt documentation
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;

        bool zoomDirection = numSteps > 0 ? true : false;
        numSteps = abs(numSteps);
        if (event->orientation() == Qt::Vertical)
        {
            for (int i = 0; i < numSteps; ++i)
            {
                float newZoom = zoomDirection ? scale + ZOOM_STEP : scale - ZOOM_STEP;
                PerformZoom(newZoom);
            }
        }
        setFocus();
        event->accept();
    }
    else
    {
        QWidget::wheelEvent(event);
    }
}

QRect ScrollZoomWidget::GetScrollBarRect() const
{
    QRect graphRect = GetGraphRect();
    QRect rect = QRect(graphRect.left(), this->height() - SCROLL_BAR_HEIGHT - 1, graphRect.width(), SCROLL_BAR_HEIGHT);
    return rect;
}

void ScrollZoomWidget::UpdateScrollBarPosition()
{
    QRect scrollBarRect = GetScrollBarRect();
    horizontalScrollBar->move(scrollBarRect.x(), scrollBarRect.y());
    horizontalScrollBar->resize(scrollBarRect.width(), scrollBarRect.height());
}

void ScrollZoomWidget::UpdateSliderPosition()
{
    QRect sliderRect = GetSliderRect();
    zoomSlider->move(sliderRect.x(), sliderRect.y());
    zoomSlider->resize(sliderRect.width(), sliderRect.height());
}

void ScrollZoomWidget::UpdateZoomSlider()
{
    this->zoomSlider->setPageStep(100 * (MAX_ZOOM - MIN_ZOOM));
    this->zoomSlider->setMinimum(100 * MIN_ZOOM);
    this->zoomSlider->setMaximum(100 * MAX_ZOOM);

    this->zoomSlider->setSliderPosition(ceil(scale * 100));
}

void ScrollZoomWidget::UpdateScrollBarSlider()
{
    int rengeStep = 100 * (maxTime - minTime);
    int documentLength = 100 * (generalMaxTime - generalMinTime);

    this->horizontalScrollBar->setPageStep(rengeStep);
    this->horizontalScrollBar->setMinimum(0);
    this->horizontalScrollBar->setMaximum(documentLength - rengeStep);

    this->horizontalScrollBar->setSliderPosition(ceil(minTime * 100));
}

DAVA::int32 ScrollZoomWidget::GetIntValue(DAVA::float32 value) const
{
    DAVA::float32 sign = (value < 0) ? -1.f : 1.f;
    return static_cast<DAVA::int32>(value + 0.5f * sign);
}

void ScrollZoomWidget::PerformZoom(DAVA::float32 newScale, bool moveSlider)
{
    float currentInterval = maxTime - minTime;

    if (newScale < MIN_ZOOM || newScale > MAX_ZOOM || currentInterval < MINIMUM_DISPLAYED_TIME)
    {
        return;
    }

    float currentCenter = minTime + currentInterval / 2;
    float newInterval = initialTimeInterval / newScale;

    minTime = currentCenter - (newInterval / 2);
    maxTime = currentCenter + (newInterval / 2);

    if (minTime < generalMinTime)
    {
        minTime = generalMinTime;
        maxTime = minTime + newInterval;
    }
    if (maxTime > generalMaxTime)
    {
        minTime = generalMaxTime - newInterval;
        maxTime = generalMaxTime;
    }

    scale = newScale;

    UpdateScrollBarSlider();

    if (moveSlider)
    {
        UpdateZoomSlider();
    }
}

void ScrollZoomWidget::PerformOffset(DAVA::float32 value, bool moveScroll)
{
    //!
    /*
	if( sizeState != SIZE_STATE_NORMAL )
	{
		return;
	}*/

    //calculate new values of boundaries (in seconds) from given parameter(in pixels)
    DAVA::float32 pixelsPerTime = GetGraphRect().width() / (maxTime - minTime);
    DAVA::float32 offsetFactor = value / pixelsPerTime;

    DAVA::float32 newMinTime = minTime + offsetFactor;

    if (newMinTime < generalMinTime)
    {
        offsetFactor = (minTime - generalMinTime) * (-1.0f);
    }

    DAVA::float32 newMaxTime = maxTime + offsetFactor;
    if (newMaxTime > generalMaxTime)
    {
        offsetFactor = generalMaxTime - maxTime;
    }

    maxTime += offsetFactor;
    minTime += offsetFactor;

    if (moveScroll)
    {
        UpdateScrollBarSlider();
    }
}

ScrollZoomWidget::ePositionRelativelyToDrawRect ScrollZoomWidget::GetPointPositionFromDrawingRect(QPoint point) const
{
    //check if point is situated inside or outside of drawing rectangle
    QRect graphRect = GetGraphRect();
    if (point.x() < graphRect.x())
    {
        return ScrollZoomWidget::POSITION_LEFT;
    }
    else if (point.x() > (graphRect.x() + graphRect.width()))
    {
        return ScrollZoomWidget::POSITION_RIGHT;
    }
    return ScrollZoomWidget::POSITION_INSIDE;
}

void ScrollZoomWidget::HandleHorizontalScrollChanged(int value) // value in miliseconds
{
    float newMinTime = (float)value / 100;
    float pixelsPerTime = GetGraphRect().width() / (maxTime - minTime);
    float offsetFactor = (newMinTime - minTime) * pixelsPerTime;
    PerformOffset(offsetFactor, true);
    this->update();
}

void ScrollZoomWidget::HandleZoomScrollChanged(int value)
{
    float newScale = (float)value / 100;
    PerformZoom(newScale, false);
    this->update();
}
