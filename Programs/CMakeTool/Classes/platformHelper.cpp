#include "platformHelper.h"

PlatformHelper::PlatformHelper(QObject* parent)
    : QObject(parent)
{
}

PlatformHelper::ePlatform PlatformHelper::CurrentPlatform()
{
#if defined(Q_OS_WIN)
    return Windows;
#elif defined(Q_OS_MAC)
    return Mac;
#else
#error "wrong platform"
#endif
}
