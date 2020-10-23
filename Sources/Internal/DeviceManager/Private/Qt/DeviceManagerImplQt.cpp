#include "DeviceManager/Private/Qt/DeviceManagerImplQt.h"

#if defined(__DAVAENGINE_QT__)

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

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
    return 0.0f;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_QT__
