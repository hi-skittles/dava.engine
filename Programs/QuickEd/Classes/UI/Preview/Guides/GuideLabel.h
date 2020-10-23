#pragma once

#include <Math/Vector.h>

#include <QWidget>

class QPaintEvent;

class GuideLabel : public QWidget
{
    Q_OBJECT

public:
    GuideLabel(DAVA::Vector2::eAxis orientation, QWidget* parent);

    void SetValue(int value);
    int GetValue() const;

private:
    void paintEvent(QPaintEvent*);

    const DAVA::Vector2::eAxis orientation;
    int value = 0;
};
