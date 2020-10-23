#ifndef GRADIENDWIDGET_H
#define GRADIENDWIDGET_H

#include <QWidget>

class GradientWidget
: public QWidget
{
    Q_OBJECT

protected:
    struct Offset
    {
        int left;
        int top;
        int right;
        int bottom;

        Offset()
            : left(0)
            , top(0)
            , right(0)
            , bottom(0)
        {
        }
    };

public:
    explicit GradientWidget(QWidget* parent);
    ~GradientWidget();

    void SetColorRange(const QColor& start, const QColor& stop);
    void SetRenderDimensions(bool hor, bool ver);
    void SetBgPadding(int left, int top, int right, int bottom);
    void SetGrid(bool enabled, const QSize& size = QSize());

    QColor GetColorAt(const QPoint& pos) const;

protected:
    virtual QPixmap drawBackground() const;
    virtual QPixmap drawContent() const;

    const Offset& padding() const;

    // QWidget
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    mutable QPixmap cacheBg;
    mutable QImage cacheBgImage; // To fast GetColor of any pixel
    QColor startColor;
    QColor stopColor;
    Offset paddingOfs;
    bool hor; // Direction
    bool ver; // Direction
    bool fillBg;
    QSize gridSize;
};


#endif // GRADIENDWIDGET_H
