#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_QT__) || defined(__DAVAENGINE_LINUX__)

namespace DAVA
{
class LocalNotificationController;

namespace Private
{
struct LocalNotificationListener
{
    LocalNotificationListener(LocalNotificationController& controller);
};

LocalNotificationListener::LocalNotificationListener(LocalNotificationController& controller)
{
}
} // namespace Private
} // namespace DAVA

#endif // defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_QT__) || defined(__DAVAENGINE_LINUX__)
