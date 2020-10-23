#ifndef VALUESLIDER_H
#define VALUESLIDER_H

#include <QWidget>
#include <QPointer>

class QLineEdit;
class MouseHelper;

//
//  Parent widget must have Click focus policy
//

class ValueSlider
: public QWidget
{
    Q_OBJECT

signals:
    void started(double);
    void changing(double);
    void changed(double);
    void canceled();

public:
    explicit ValueSlider(QWidget* parent = NULL);
    ~ValueSlider();

    void SetDigitsAfterDot(int c);
    void SetRange(double min, double max);
    void SetValue(double val);
    double GetValue() const;

protected:
    virtual void DrawBackground(QPainter* p) const;
    virtual void DrawForeground(QPainter* p) const;
    virtual QRect PosArea() const;

    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

    bool eventFilter(QObject* obj, QEvent* e) override;

    bool IsEditorMode() const;

private slots:
    void OnMousePress(const QPoint& pos);
    void OnMouseMove(const QPoint& pos);
    void OnMouseRelease(const QPoint& pos);
    void OnMouseClick();

private:
    void normalize();
    void undoEditing();
    void acceptEditing();

    double minVal;
    double maxVal;
    double val;
    int digitsAfterDot;

    QPointer<MouseHelper> mouse;
    QPoint clickPos;
    double clickVal;
    mutable QPixmap arrows;

    QPointer<QLineEdit> editor;
};


#endif // VALUESLIDER_H
