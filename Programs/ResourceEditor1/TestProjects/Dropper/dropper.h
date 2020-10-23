#ifndef DROPPER_H
#define DROPPER_H

#include <QtWidgets/QWidget>
#include "ui_dropper.h"

class Dropper : public QWidget
{
    Q_OBJECT

public:
    explicit Dropper(QWidget* parent = 0);
    ~Dropper();

private slots:
    void showCP();

    void OnStarted(const QPointF&);
    void OnChanging(const QPointF&);
    void OnChanged(const QPointF&);
    void OnOn(const QPointF&);

private:
    Ui::DropperClass ui;
};

#endif // DROPPER_H
