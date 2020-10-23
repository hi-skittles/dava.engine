#pragma once

#include <QObject>
#include <QString>

class PlatformHelper : public QObject
{
    Q_OBJECT

public:
    enum ePlatform
    {
        Windows,
        Mac
    };
    Q_ENUMS(ePlatform)

    explicit PlatformHelper(QObject* parent = 0);
    Q_INVOKABLE static ePlatform CurrentPlatform();
};
