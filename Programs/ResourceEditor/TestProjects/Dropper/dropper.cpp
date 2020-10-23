#include "dropper.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QCursor>
#include <QDateTime>
#include <QDebug>


#include "ColorPicker/ColorPicker.h"
#include "ColorPicker/EyeDropper.h"

Dropper::Dropper(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    ui.test_2->SetColors(QColor(0, 255, 0), QColor(0, 0, 255));
    ui.test_2->SetDimensions(Qt::LeftEdge | Qt::RightEdge);
    ui.test_2->SetOrientation(Qt::Vertical);
    ui.test_2->SetOffsets(10, 10, 10, 10);

    connect(ui.test_2, SIGNAL(started(const QPointF&)), SLOT(OnStarted(const QPointF&)));
    connect(ui.test_2, SIGNAL(changing(const QPointF&)), SLOT(OnChanging(const QPointF&)));
    connect(ui.test_2, SIGNAL(changed(const QPointF&)), SLOT(OnChanged(const QPointF&)));

    connect(ui.btn, SIGNAL(clicked()), SLOT(showCP()));
}

Dropper::~Dropper()
{
}

void Dropper::showCP()
{
    ColorPicker* cp = new ColorPicker(this);
    cp->setWindowFlags(Qt::Window);
    cp->setAttribute(Qt::WA_DeleteOnClose, true);

    cp->SetColor(QColor(90, 150, 230, 120));
    cp->exec();
}

void Dropper::OnStarted(const QPointF& posF)
{
    OnOn(posF);
}

void Dropper::OnChanging(const QPointF& posF)
{
    OnOn(posF);
}

void Dropper::OnChanged(const QPointF& posF)
{
    OnOn(posF);
}

void Dropper::OnOn(const QPointF& posF)
{
    const QString text = QString::number(posF.y());
    ui.edit->setText(text);
}
