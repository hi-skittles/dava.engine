#include <QApplication>
#include "Classes/Qt/Tools/QtWaitDialog/QtWaitDialog.h"
#include "ui_waitdialog.h"

QtWaitDialog::QtWaitDialog(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::CustomizeWindowHint)
    , ui(new Ui::QtWaitDialog)
{
    setFixedSize(400, 150);
    setWindowModality(Qt::WindowModal);

    ui->setupUi(this);

    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    ui->waitLabel->setPalette(pal);

    connect(ui->waitButton, SIGNAL(pressed()), this, SLOT(CancelPressed()));
    connect(this, SIGNAL(canceled()), this, SLOT(WaitCanceled()));
}

void QtWaitDialog::Exec(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel)
{
    Setup(title, message, hasWaitbar, hasCancel);

    setCursor(Qt::BusyCursor);
    isRunnedFromExec = true;
    show();
    loop.exec();
}

void QtWaitDialog::Show(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel)
{
    Setup(title, message, hasWaitbar, hasCancel);

    setCursor(Qt::BusyCursor);
    isRunnedFromExec = false;
    show();

    processEvents();
}

void QtWaitDialog::Reset()
{
    executor.DelayedExecute(DAVA::MakeFunction(this, &QtWaitDialog::ResetImpl));
}

void QtWaitDialog::SetMessage(const QString& message)
{
    ui->waitLabel->setPlainText(message);
    processEvents();
}

void QtWaitDialog::SetRange(int min, int max)
{
    ui->waitBar->setRange(min, max);
    processEvents();
}

void QtWaitDialog::SetRangeMin(int min)
{
    ui->waitBar->setMinimum(min);
    processEvents();
}

void QtWaitDialog::SetRangeMax(int max)
{
    ui->waitBar->setMaximum(max);
    processEvents();
}

void QtWaitDialog::SetValue(int value)
{
    ui->waitBar->setVisible(true);
    ui->waitBar->setValue(value);
    processEvents();
}

void QtWaitDialog::CancelPressed()
{
    wasCanceled = true;
    emit canceled();
}

void QtWaitDialog::WaitCanceled()
{
    ui->waitButton->setEnabled(false);
    processEvents();
}

void QtWaitDialog::ResetImpl()
{
    wasCanceled = false;

    if (isRunnedFromExec)
    {
        loop.quit();
    }

    close();
    setCursor(Qt::ArrowCursor);

    emit closed();
}

void QtWaitDialog::processEvents()
{
    if (!isRunnedFromExec && isVisible())
    {
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QApplication::processEvents();
    }
}

void QtWaitDialog::EnableCancel(bool enable)
{
    if (!wasCanceled)
    {
        ui->waitButton->setEnabled(enable);
    }
}

void QtWaitDialog::Setup(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel)
{
    setWindowTitle(title);
    SetMessage(message);

    ui->waitButton->setEnabled(hasCancel);
    ui->waitBar->setVisible(hasWaitbar);

    wasCanceled = false;
}

bool QtWaitDialog::WasCanceled() const
{
    return wasCanceled;
}
