#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)

#include "DeviceManager/DeviceManagerTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"

namespace DAVA
{
class DeviceManager;
namespace Private
{
struct DeviceManagerImpl final
{
    DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher);

    void UpdateDisplayConfig();

    float32 GetCpuTemperature() const;

    DeviceManager* deviceManager = nullptr;
    Private::MainDispatcher* mainDispatcher = nullptr;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_QT__
