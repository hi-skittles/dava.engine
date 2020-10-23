#include "help.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QProcess>

Help::Help(QObject* parent)
    : QObject(parent)
{
    helpPath = qApp->applicationDirPath() +
#ifdef Q_OS_WIN
    "/Data/CMakeToolHelp.pdf";
#elif defined Q_OS_MAC
    "/../Resources/Data/CMakeToolHelp.pdf";
#else
    #ERROR "usupported platform";
#endif //platform
}

void Help::Show() const
{
    QFileInfo fileInfo(helpPath);
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl(fileInfo.absoluteFilePath()));
#elif defined Q_OS_MAC
    QProcess::startDetached("open", QStringList() << fileInfo.absoluteFilePath(), fileInfo.absolutePath());
#else
#ERROR "usupported platform";
#endif //platform
}
