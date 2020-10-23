#include "DeviceManager/Private/Linux/DeviceManagerImplLinux.h"

#if defined(__DAVAENGINE_LINUX__)

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
namespace Private
{
DeviceManagerImpl::DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher)
    : deviceManager(devManager)
    , mainDispatcher(dispatcher)
{
}

void DeviceManagerImpl::UpdateDisplayConfig()
{
}

float32 DeviceManagerImpl::GetCpuTemperature() const
{
    return 0.f;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_LINUX__
