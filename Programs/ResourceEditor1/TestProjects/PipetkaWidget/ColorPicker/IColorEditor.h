#ifndef ICOLOREDITOR_H
#define ICOLOREDITOR_H

#include <QObject>

class IColorEditor
{
signals:
    virtual void begin() = 0;
    virtual void changing(const QColor& c) = 0;
    virtual void changed(const QColor& c) = 0;
    virtual void canceled() = 0;

public:
    virtual ~IColorEditor()
    {
    }

    virtual QColor GetColor() const = 0;
    virtual void setColor(const QColor& c) = 0;
};


#endif // ICOLOREDITOR_H
