#pragma once

#include <TArc/Utils/QtDelayedExecutor.h>

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QEventLoop>

namespace Ui
{
class QtWaitDialog;
}

class QtWaitDialog : public QWidget
{
    Q_OBJECT

public:
    QtWaitDialog(QWidget* parent = 0);

    void Exec(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel);
    void Show(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel);
    void Reset();

    void SetMessage(const QString& message);

    void SetRange(int min, int max);
    void SetRangeMin(int min);
    void SetRangeMax(int max);
    void SetValue(int value);
    void EnableCancel(bool enable);

    bool WasCanceled() const;

signals:
    void canceled();
    void closed();

protected slots:
    void CancelPressed();
    void WaitCanceled();

private:
    void ResetImpl();
    void processEvents();

    void Setup(const QString& title, const QString& message, bool hasWaitbar, bool hasCancel);
    Ui::QtWaitDialog* ui;
    DAVA::QtDelayedExecutor executor;

    bool wasCanceled = false;
    bool isRunnedFromExec = false;
    QEventLoop loop;
};
