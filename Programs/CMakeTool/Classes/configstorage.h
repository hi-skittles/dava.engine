#pragma once

#include <QObject>
#include <QString>

class ConfigStorage : public QObject
{
    Q_OBJECT

public:
    explicit ConfigStorage(QObject* parent = 0);
    Q_INVOKABLE QString GetJSONTextFromConfigFile() const;

private:
    QString configFilePath;
};
