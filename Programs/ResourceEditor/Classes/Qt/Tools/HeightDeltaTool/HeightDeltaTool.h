#pragma once

#include "DAVAEngine.h"

#include <QWidget>
#include <QScopedPointer>

namespace Ui
{
class HeightDeltaTool;
}

class HeightDeltaTool
: public QWidget
{
    Q_OBJECT

public:
    explicit HeightDeltaTool(QWidget* p = NULL);
    ~HeightDeltaTool();

private slots:
    void OnRun();
    void OnValueChanged(double v = 0);

private:
    double GetThresholdInMeters(double unitSize);

    QScopedPointer<Ui::HeightDeltaTool> ui;
    QString outputFilePath;
};
