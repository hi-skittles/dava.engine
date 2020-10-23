#ifndef EYEDROPPER_H
#define EYEDROPPER_H

#include <QWidget>
#include <QPointer>
#include <QPixmap>

class MouseHelper;

class EyeDropper
: public QWidget
{
    Q_OBJECT

signals:
    void picked(const QColor& color);
    void moved(const QColor& color);

public:
    explicit EyeDropper(QWidget* parent = NULL);
    ~EyeDropper();

public slots:
    void Exec();

private slots:
    void OnMouseMove(const QPoint& pos);
    void OnClicked(const QPoint& pos);

private:
    void paintEvent(QPaintEvent* e);
    void keyPressEvent(QKeyEvent* e);
    void DrawCursor(const QPoint& pos, QPainter* p);
    void CreateShade();
    QColor GetPixel(const QPoint& pos) const;

    QPointer<QWidget> shade;
    QPointer<MouseHelper> mouse;
    QImage cache;
    QSize cursorSize;
    QPoint cursorPos;
};


#endif // EYEDROPPER_H
