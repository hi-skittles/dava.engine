#ifndef ABSTRACTSLIDER_H
#define ABSTRACTSLIDER_H

#include <QWidget>
#include <QPointer>

class MouseHelper;

class AbstractSlider
: public QWidget
{
    Q_OBJECT

signals:
    void started(const QPointF&);
    void changing(const QPointF&);
    void changed(const QPointF&);
    void canceled();

public:
    explicit AbstractSlider(QWidget* parent);
    ~AbstractSlider();

    QPointF PosF() const;
    void SetPosF(const QPointF& posF);

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

    virtual void DrawBackground(QPainter* p) const;
    virtual void DrawForeground(QPainter* p) const;
    virtual QRect PosArea() const;

    QPoint Pos() const;
    void SetPos(const QPoint& pos);
    MouseHelper* Mouse() const;

private slots:
    void OnMousePress(const QPoint& pos);
    void OnMouseMove(const QPoint& pos);
    void OnMouseRelease(const QPoint& pos);

private:
    QPointF posF;
    QPoint pressPos;
    QSize lastSize; //  остыль, т.к. Qt присылает неверный oldSize в первый resizeEvent
    QPointer<MouseHelper> mouse;
};


#endif // ABSTRACTSLIDER_H
