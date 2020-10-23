#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

namespace DAVA
{
class LocalNotificationController;

namespace Private
{
struct LocalNotificationListener final
{
    LocalNotificationListener(LocalNotificationController& controller);
    ~LocalNotificationListener();

private:
    void* nativeListener = nullptr; // To avoid including obj-c header
};
} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
