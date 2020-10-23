#ifndef HSVPALETTEWIDGET_H
#define HSVPALETTEWIDGET_H

#include <QWidget>
#include "IColorEditor.h"

class HSVPaletteWidget
: public QWidget
  ,
  public IColorEditor
{
    Q_OBJECT

signals:
    void begin();
    void changing(const QColor& c);
    void changed(const QColor& c);
    void canceled();

public:
    explicit HSVPaletteWidget(QWidget* parent = NULL);
    ~HSVPaletteWidget();

    // IColorEditor
    QColor GetColor() const;
    void setColor(QColor const& c);

private:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);

    QColor color;
    QPixmap cache;
};

#endif // HSVPALETTEWIDGET_H
