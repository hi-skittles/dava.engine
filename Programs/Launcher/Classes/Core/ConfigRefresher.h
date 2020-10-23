#pragma once

#include <QObject>
class QTimer;

class ConfigRefresher : public QObject
{
    Q_OBJECT

public:
    ConfigRefresher(QObject* parent = nullptr);

    bool IsEnabled() const;
    int GetTimeout() const;
    int GetMinimumTimeout() const;
    int GetMaximumTimeout() const;

    void SetEnabled(bool enabled);
    void SetTimeout(int timeoutMs);

signals:
    void RefreshConfig();

private:
    QTimer* timer = nullptr;
};
