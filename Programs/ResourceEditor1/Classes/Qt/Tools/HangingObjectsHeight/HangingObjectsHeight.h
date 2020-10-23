#ifndef __QT_HANGING_OBJECTS_HEIGHT_H__
#define __QT_HANGING_OBJECTS_HEIGHT_H__

#include <QWidget>
#include "DAVAEngine.h"

class EventFilterDoubleSpinBox;
class HangingObjectsHeight : public QWidget
{
    Q_OBJECT

public:
    HangingObjectsHeight(QWidget* parent = 0);

    void SetHeight(DAVA::float32 value);

signals:
    void HeightChanged(double value);

protected slots:

    void ValueChanged(double value);

protected:
    EventFilterDoubleSpinBox* heightValue;
};

#endif // __QT_HANGING_OBJECTS_HEIGHT_H__
