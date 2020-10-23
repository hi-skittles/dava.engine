#include "DeviceManager/Private/Win32/DeviceManagerImplWin32.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_WIN32__)

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Win32/DllImportWin32.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
namespace Private
{
struct DeviceManagerImpl::DisplayInfoRange
{
    DisplayInfo* begin;
    DisplayInfo* end;
    DisplayInfo* cur;
};

DeviceManagerImpl::DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher)
    : deviceManager(devManager)
    , mainDispatcher(dispatcher)
{
    size_t count = ::GetSystemMetrics(SM_CMONITORS);
    deviceManager->displays.resize(count);

    DisplayInfoRange range;
    range.begin = &deviceManager->displays[0];
    range.cur = range.begin;
    range.end = range.begin + count;
    count = GetDisplays(&range);
    deviceManager->displays.resize(count);
}

void DeviceManagerImpl::UpdateDisplayConfig()
{
    size_t count = ::GetSystemMetrics(SM_CMONITORS);

    DisplayInfoRange range;
    range.begin = new DisplayInfo[count];
    range.cur = range.begin;
    range.end = range.begin + count;
    count = GetDisplays(&range);

    mainDispatcher->PostEvent(MainDispatcherEvent::CreateDisplayConfigChangedEvent(range.begin, count));
}

size_t DeviceManagerImpl::GetDisplays(DisplayInfoRange* range)
{
    ::EnumDisplayMonitors(nullptr, nullptr, &DisplayEnumProc, reinterpret_cast<LPARAM>(range));

    size_t count = range->cur - range->begin;
    if (DllImport::fnGetDpiForMonitor == nullptr)
    {
        HDC screen = ::GetDC(nullptr);
        float32 dpiX = static_cast<float32>(::GetDeviceCaps(screen, LOGPIXELSX));
        float32 dpiY = static_cast<float32>(::GetDeviceCaps(screen, LOGPIXELSY));
        ::ReleaseDC(NULL, screen);

        for (size_t i = 0; i < count; ++i)
        {
            range->begin[i].rawDpiX = dpiX;
            range->begin[i].rawDpiY = dpiY;
        }
    }

    // Make primary display to be the first in list
    std::sort(range->begin, range->end, [](const DisplayInfo& l, const DisplayInfo& r) { return l.primary > r.primary; });
    return count;
}

BOOL CALLBACK DeviceManagerImpl::DisplayEnumProc(HMONITOR hmonitor, HDC hdc, LPRECT rc, LPARAM lparam)
{
    DisplayInfoRange* range = reinterpret_cast<DisplayInfoRange*>(lparam);
    if (range->cur < range->end)
    {
        MONITORINFOEXW mi;
        mi.cbSize = sizeof(mi);
        ::GetMonitorInfoW(hmonitor, reinterpret_cast<LPMONITORINFO>(&mi));

        DisplayInfo* di = range->cur;
        di->systemId = reinterpret_cast<uintptr_t>(hmonitor);
        di->rect.x = static_cast<float32>(mi.rcMonitor.left);
        di->rect.y = static_cast<float32>(mi.rcMonitor.top);
        di->rect.dx = static_cast<float32>(mi.rcMonitor.right - mi.rcMonitor.left);
        di->rect.dy = static_cast<float32>(mi.rcMonitor.bottom - mi.rcMonitor.top);
        di->primary = (mi.dwFlags & MONITORINFOF_PRIMARY) == MONITORINFOF_PRIMARY;
        di->name = UTF8Utils::MakeUTF8String(mi.szDevice);

        UINT dpiX = 0;
        UINT dpiY = 0;
        if (DllImport::fnGetDpiForMonitor != nullptr)
        {
            DllImport::fnGetDpiForMonitor(hmonitor, MDT_RAW_DPI, &dpiX, &dpiY);
            di->rawDpiX = static_cast<float32>(dpiX);
            di->rawDpiY = static_cast<float32>(dpiY);
        }

        range->cur += 1;
        return TRUE;
    }
    return FALSE;
}

float32 DeviceManagerImpl::GetCpuTemperature() const
{
    return 0.0f;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
