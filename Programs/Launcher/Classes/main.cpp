#include "Core/GuiApplicationManager.h"
#include "Core/CommandLineParser.h"
#include "Utils/ErrorMessenger.h"

#include <QtHelpers/ProcessHelper.h>
#include <QtHelpers/HelperFunctions.h>
#include <QtHelpers/RunGuard.h>

#include <QApplication>
#include <QStyleFactory>

void LogMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    ErrorMessenger::LogMessage(type, msg);
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("DAVA");
    a.setApplicationName("Launcher");
    a.setApplicationVersion(LAUNCHER_VER);

    QStringList arguments = a.arguments();
    if (arguments.size() > 1)
    {
        CommandLineParser parser;
        parser.Parse(arguments);
        return 0;
    }

    const QString appUid = "{E5C30634-7624-4D0F-9DD9-C8D52AECA3D0}";
    const QString appUidPath = QCryptographicHash::hash((appUid + QApplication::applicationDirPath()).toUtf8(), QCryptographicHash::Sha1).toHex();

    QString appPath = QtHelpers::GetApplicationFilePath();
    ApplicationQuitController appQuitController;
    int retCode = 0;
    {
        QtHelpers::RunGuard runGuard(appUidPath);
        if (runGuard.TryToRun())
        {
            qInstallMessageHandler(LogMessageHandler);

            a.setAttribute(Qt::AA_UseHighDpiPixmaps);

            QApplication::setStyle(QStyleFactory::create("Fusion"));

            GuiApplicationManager appManager(&appQuitController);
            appManager.Start();

            retCode = a.exec();
        }
    }
    if (appQuitController.requireRestart)
    {
        ProcessHelper::OpenApplication(appPath);
    }
    return retCode;
}
