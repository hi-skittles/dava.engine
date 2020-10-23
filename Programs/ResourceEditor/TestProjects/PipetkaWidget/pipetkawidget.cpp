#include "pipetkawidget.h"

#include "ColorPicker/ColorWidget.h"

PipetkaWidget::PipetkaWidget(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    ColorWidget* w = new ColorWidget(this);
    w->setWindowFlags(Qt::Window);
    w->show();
}

PipetkaWidget::~PipetkaWidget()
{
}
