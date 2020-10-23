#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_WIN32__)

#include "Base/Platform.h"
#include "DeviceManager/DeviceManagerTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"

namespace DAVA
{
class DeviceManager;
namespace Private
{
struct DeviceManagerImpl final
{
    struct DisplayInfoRange;

    DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher);

    void UpdateDisplayConfig();
    size_t GetDisplays(DisplayInfoRange* range);

    static BOOL CALLBACK DisplayEnumProc(HMONITOR hmonitor, HDC hdc, LPRECT rc, LPARAM lparam);

    float32 GetCpuTemperature() const;

    DeviceManager* deviceManager = nullptr;
    Private::MainDispatcher* mainDispatcher = nullptr;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
