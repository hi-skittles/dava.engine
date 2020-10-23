#include "QResourceEditorProgressDialog.h"
#include <qprogressbar.h>

#define TIME_INTERVAL_FOR_1_PERCENT 10

QResourceEditorProgressDialog::QResourceEditorProgressDialog(QWidget* parent, Qt::WindowFlags f, bool _isCycled)
    : QProgressDialog(parent, f)
{
    isCycled = _isCycled;
    timeIntervalForPercent = TIME_INTERVAL_FOR_1_PERCENT;
    connect(&timer, SIGNAL(timeout()), this, SLOT(UpdateProgress()), Qt::QueuedConnection);

    if (isCycled)
    {
        //add custom bar to avoid parcents displaying
        QProgressBar* bar = new QProgressBar(this);
        bar->setTextVisible(false);
        this->setBar(bar);
    }
}

void QResourceEditorProgressDialog::showEvent(QShowEvent* e)
{
    QProgressDialog::showEvent(e);
    if (isCycled)
    {
        timer.start(timeIntervalForPercent);
    }
}

void QResourceEditorProgressDialog::UpdateProgress()
{
    if ((!this->isVisible()) || (isCycled == false))
    {
        return;
    }

    int newValue = this->value() + 1;
    if (newValue >= maximum())
    {
        newValue = 0;
    }

    this->setValue(newValue);
    timer.start(timeIntervalForPercent);
}
