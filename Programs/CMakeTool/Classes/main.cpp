#include "configstorage.h"
#include "processwrapper.h"
#include "filesystemhelper.h"
#include "help.h"
#include "platformHelper.h"

#include <QApplication>
#include <QString>
#include <QQmlApplicationEngine>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QtQuick>
#include <QtQML>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("DAVA");
    app.setApplicationName("CMakeTool");
    QQmlApplicationEngine engine;
    auto rootContext = engine.rootContext();
    qmlRegisterType<ProcessWrapper>("Cpp.Utils", 1, 0, "ProcessWrapper");
    qmlRegisterType<FileSystemHelper>("Cpp.Utils", 1, 0, "FileSystemHelper");
    qmlRegisterType<ConfigStorage>("Cpp.Utils", 1, 0, "ConfigStorage");
    qmlRegisterType<Help>("Cpp.Utils", 1, 0, "Help");
    qmlRegisterType<PlatformHelper>("Cpp.Utils", 1, 0, "PlatformHelper");

    rootContext->setContextProperty("applicationDirPath", app.applicationDirPath());
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().empty())
    {
        QMessageBox::critical(nullptr, QObject::tr("Failed to load QML file"), QObject::tr("QML file not loaded!"));
        return 0;
    }
    return app.exec();
}
