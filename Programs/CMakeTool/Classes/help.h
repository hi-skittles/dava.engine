#pragma once

#include <QObject>
#include <QString>

class Help : public QObject
{
    Q_OBJECT

public:
    explicit Help(QObject* parent = 0);
    Q_INVOKABLE void Show() const;

private:
    QString helpPath;
};
