#include "Core/ConfigRefresher.h"
#include <QTimer>

const char* packageName = "ConfigRefresher";

ConfigRefresher::ConfigRefresher(QObject* parent /*= nullptr*/)
    : QObject(parent)
{
    timer = new QTimer(this);
    timer->setSingleShot(false);
    timer->setInterval(300 * 1000); //default timeout
    timer->start();
    connect(timer, &QTimer::timeout, this, &ConfigRefresher::RefreshConfig);
}

bool ConfigRefresher::IsEnabled() const
{
    return timer->isActive();
}

int ConfigRefresher::GetTimeout() const
{
    return timer->interval();
}

int ConfigRefresher::GetMinimumTimeout() const
{
    return 1 * 60 * 1000; //one minute
}

int ConfigRefresher::GetMaximumTimeout() const
{
    return 60 * 60 * 1000; //one hour
}

void ConfigRefresher::SetEnabled(bool enabled)
{
    timer->stop();
    if (enabled)
    {
        timer->start();
    }
}

void ConfigRefresher::SetTimeout(int timeoutMs)
{
    timeoutMs = qBound(GetMinimumTimeout(), timeoutMs, GetMaximumTimeout());
    timer->setInterval(timeoutMs);
}
