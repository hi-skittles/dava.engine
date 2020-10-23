#include "configstorage.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QDir>

ConfigStorage::ConfigStorage(QObject* parent)
    : QObject(parent)
{
    configFilePath = qApp->applicationDirPath() +
#ifdef Q_OS_WIN
    "/Data/config_windows.json";
#elif defined Q_OS_MAC
    "/../Resources/Data/config_mac.json";
#else
#ERROR "usupported platform"
#endif //platform
    configFilePath = QDir::toNativeSeparators(configFilePath);
}

QString ConfigStorage::GetJSONTextFromConfigFile() const
{
    if (!QFile::exists(configFilePath))
    {
        QMessageBox::critical(nullptr, QObject::tr("Config file not available!"), QObject::tr("Can not find config file %1").arg(configFilePath));
        exit(0);
    }
    QFile configFile(configFilePath);
    if (configFile.open(QIODevice::ReadOnly))
    {
        return configFile.readAll();
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Failed to open config file!"), QObject::tr("Failed to open config file %1").arg(configFilePath));
        exit(0);
    }
    return QString();
}
