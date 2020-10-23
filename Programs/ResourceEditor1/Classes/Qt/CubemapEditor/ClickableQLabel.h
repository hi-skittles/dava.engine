#ifndef __QT_CLICKABLE_QLABEL_H__
#define __QT_CLICKABLE_QLABEL_H__

#include <QLabel>

class ClickableQLabel : public QLabel
{
    Q_OBJECT

public:
    ClickableQLabel(QWidget* parent = 0);
    ~ClickableQLabel();

    void SetRotation(int rotation);
    int GetRotation();

    void SetVisualRotation(int rotation);
    int GetVisualRotation();

    void SetFaceLoaded(bool loaded);
    bool GetFaceLoaded();

    void OnParentMouseMove(QMouseEvent* ev);

protected:
    void mousePressEvent(QMouseEvent* ev);
    void enterEvent(QEvent* ev);
    void leaveEvent(QEvent* ev);
    void paintEvent(QPaintEvent* ev);
    void mouseMoveEvent(QMouseEvent* ev);

signals:

    void OnLabelClicked();
    void OnRotationChanged();

private:
    enum RotateButtonDrawFlags
    {
        None = 0,
        RotateClockwise = 1,
        RotateCounterclockwise = 2
    };

    bool IsPointInsideClockwiseRotationArea(QMouseEvent* ev);
    bool IsPointInsideCounterclockwiseRotationArea(QMouseEvent* ev);
    bool IsPointOutsideControl(QMouseEvent* ev);
    void DrawRotationIcon(QPaintEvent* ev, QPoint position, float opacity, bool flipped);
    void DrawFaceImage(QPaintEvent* ev);
    QPoint GetPointForButton(RotateButtonDrawFlags flag);

private:
    bool faceLoaded;
    bool mouseEntered;
    int buttonDrawFlags;
    int currentRotation;
    int visualRotation;

    static QImage rotateClockwiseImage;
    static QImage rotateCounterclockwiseImage;
};

#endif /* defined(__QT_CLICKABLE_QLABEL_H__) */
