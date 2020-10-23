#include "Classes/Qt/CubemapEditor/ClickableQLabel.h"

#include <Debug/DVAssert.h>

#include <QMouseEvent>
#include <QPainter>

const float DEFAULT_ROTATION_BUTTON_OPACITY = 0.6f;
const float HOVER_ROTATION_BUTTON_OPACITY = 1.0f;

QImage ClickableQLabel::rotateClockwiseImage;
QImage ClickableQLabel::rotateCounterclockwiseImage;

ClickableQLabel::ClickableQLabel(QWidget* parent)
    : QLabel(parent)
{
    mouseEntered = false;
    buttonDrawFlags = ClickableQLabel::None;
    currentRotation = 0;
    faceLoaded = false;
    visualRotation = 0;

    setMouseTracking(true);

    if (rotateClockwiseImage.isNull())
    {
        bool loadResult = rotateClockwiseImage.load(":/QtIcons/btn_rotate_cw.png");
        DVASSERT(loadResult); //the image is required
    }

    if (rotateCounterclockwiseImage.isNull())
    {
        bool loadResult = rotateCounterclockwiseImage.load(":/QtIcons/btn_rotate_ccw.png");
        DVASSERT(loadResult); //the image is required
    }
}

ClickableQLabel::~ClickableQLabel()
{
}

void ClickableQLabel::SetRotation(int rotation)
{
    currentRotation = rotation;
}

int ClickableQLabel::GetRotation()
{
    return currentRotation;
}

void ClickableQLabel::SetFaceLoaded(bool loaded)
{
    faceLoaded = loaded;
}

bool ClickableQLabel::GetFaceLoaded()
{
    return faceLoaded;
}

void ClickableQLabel::mousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        if (ClickableQLabel::RotateClockwise == buttonDrawFlags)
        {
            currentRotation += 90;
            if (currentRotation >= 360)
            {
                currentRotation = currentRotation - 360;
            }

            this->update();

            emit OnRotationChanged();
        }
        else if (ClickableQLabel::RotateCounterclockwise == buttonDrawFlags)
        {
            currentRotation -= 90;
            if (currentRotation < 0)
            {
                currentRotation = 360 + currentRotation;
            }

            this->update();

            emit OnRotationChanged();
        }
        else
        {
            emit OnLabelClicked();

            mouseEntered = false;
            buttonDrawFlags = ClickableQLabel::None;
            this->update();
        }
    }

    QLabel::mousePressEvent(ev);
}

void ClickableQLabel::enterEvent(QEvent* ev)
{
    mouseEntered = true;
    buttonDrawFlags = ClickableQLabel::None;
    this->update();

    QLabel::enterEvent(ev);
}

void ClickableQLabel::leaveEvent(QEvent* ev)
{
    mouseEntered = false;
    buttonDrawFlags = ClickableQLabel::None;
    this->update();

    QLabel::leaveEvent(ev);
}

void ClickableQLabel::OnParentMouseMove(QMouseEvent* ev)
{
    if (mouseEntered &&
        IsPointOutsideControl(ev))
    {
        mouseEntered = false;
        this->update();
    }
}

void ClickableQLabel::paintEvent(QPaintEvent* ev)
{
    if (faceLoaded)
    {
        DrawFaceImage(ev);

        if (mouseEntered)
        {
            if (ClickableQLabel::None == buttonDrawFlags)
            {
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateClockwise), DEFAULT_ROTATION_BUTTON_OPACITY, false);
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateCounterclockwise), DEFAULT_ROTATION_BUTTON_OPACITY, true);
            }
            else if (ClickableQLabel::RotateClockwise == buttonDrawFlags)
            {
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateClockwise), HOVER_ROTATION_BUTTON_OPACITY, false);
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateCounterclockwise), DEFAULT_ROTATION_BUTTON_OPACITY, true);
            }
            else if (ClickableQLabel::RotateCounterclockwise == buttonDrawFlags)
            {
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateClockwise), DEFAULT_ROTATION_BUTTON_OPACITY, false);
                DrawRotationIcon(ev, GetPointForButton(ClickableQLabel::RotateCounterclockwise), HOVER_ROTATION_BUTTON_OPACITY, true);
            }
        }
    }
    else
    {
        QLabel::paintEvent(ev);
    }
}

void ClickableQLabel::mouseMoveEvent(QMouseEvent* ev)
{
    if (IsPointInsideClockwiseRotationArea(ev))
    {
        buttonDrawFlags = ClickableQLabel::RotateClockwise;
    }
    else if (IsPointInsideCounterclockwiseRotationArea(ev))
    {
        buttonDrawFlags = ClickableQLabel::RotateCounterclockwise;
    }
    else
    {
        if (IsPointOutsideControl(ev))
        {
            mouseEntered = false;
        }

        buttonDrawFlags = ClickableQLabel::None;
    }

    this->update();

    QLabel::mouseMoveEvent(ev);
}

bool ClickableQLabel::IsPointInsideClockwiseRotationArea(QMouseEvent* ev)
{
    QRect rect = QRect(GetPointForButton(ClickableQLabel::RotateClockwise),
                       QSize(rotateClockwiseImage.width(), rotateClockwiseImage.height()));
    return rect.contains(ev->pos());
}

bool ClickableQLabel::IsPointInsideCounterclockwiseRotationArea(QMouseEvent* ev)
{
    QRect rect = QRect(GetPointForButton(ClickableQLabel::RotateCounterclockwise),
                       QSize(rotateCounterclockwiseImage.width(), rotateCounterclockwiseImage.height()));
    return rect.contains(ev->pos());
}

void ClickableQLabel::DrawRotationIcon(QPaintEvent* ev, QPoint position, float opacity, bool flipped)
{
    QPainter painter(this);
    painter.setOpacity(opacity);
    painter.drawImage(position, (flipped) ? rotateCounterclockwiseImage : rotateClockwiseImage);
}

void ClickableQLabel::DrawFaceImage(QPaintEvent* ev)
{
    QPainter painter(this);

    painter.translate(QPoint(width() / 2, height() / 2));
    painter.rotate(currentRotation + visualRotation);

    const QPixmap& pixmap = *this->pixmap();
    painter.drawPixmap(QPoint(-pixmap.width() / 2, -pixmap.height() / 2), pixmap);

    painter.resetTransform();
    painter.setPen(QPen(QBrush(Qt::black), 1.0f));
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

QPoint ClickableQLabel::GetPointForButton(RotateButtonDrawFlags flag)
{
    QPoint pt(-1, -1);

    if (ClickableQLabel::RotateClockwise == flag)
    {
        pt = QPoint(0, 0);
    }
    else if (ClickableQLabel::RotateCounterclockwise == flag)
    {
        pt = QPoint(width() - rotateClockwiseImage.width(), height() - rotateClockwiseImage.height());
    }

    return pt;
}

bool ClickableQLabel::IsPointOutsideControl(QMouseEvent* ev)
{
    QRect r = rect();
    QRect currentRect = QRect(mapToGlobal(QPoint(r.left(), r.top())), size());
    QPoint pos = ev->globalPos();
    return !currentRect.contains(pos);
}

void ClickableQLabel::SetVisualRotation(int rotation)
{
    if (rotation != visualRotation)
    {
        visualRotation = rotation;
        this->update();
    }
}

int ClickableQLabel::GetVisualRotation()
{
    return visualRotation;
}
