#include "Core/ApplicationQuitController.h"

#include <QApplication>
#include <QDialog>

void ApplicationQuitController::RestartApplication()
{
    requireRestart = true;

    QDialog* currentDialog = qobject_cast<QDialog*>(qApp->activeModalWidget());
    if (currentDialog != nullptr)
    {
        currentDialog->reject();
    }

    qApp->quit();
}
