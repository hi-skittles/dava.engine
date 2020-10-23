#ifndef ABSTRACTCOLORPICKER_H
#define ABSTRACTCOLORPICKER_H

#include <QWidget>

class AbstractColorPicker
: public QWidget
{
    Q_OBJECT

signals:
    void begin();
    void changing(const QColor& c);
    void changed(const QColor& c);
    void canceled();

public:
    explicit AbstractColorPicker(QWidget* parent);
    ~AbstractColorPicker();

    QColor GetColor() const;

public slots:
    void SetColor(const QColor& c);

protected:
    virtual void SetColorInternal(const QColor& c) = 0;
    QColor color;
};


#endif // ABSTRACTCOLORPICKER_H
