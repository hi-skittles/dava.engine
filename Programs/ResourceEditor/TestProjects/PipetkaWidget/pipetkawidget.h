#ifndef PIPETKAWIDGET_H
#define PIPETKAWIDGET_H

#include <QtWidgets/QWidget>
#include "ui_pipetkawidget.h"

class PipetkaWidget : public QWidget
{
    Q_OBJECT

public:
    PipetkaWidget(QWidget* parent = 0);
    ~PipetkaWidget();

private:
    Ui::PipetkaWidgetClass ui;
};

#endif // PIPETKAWIDGET_H
