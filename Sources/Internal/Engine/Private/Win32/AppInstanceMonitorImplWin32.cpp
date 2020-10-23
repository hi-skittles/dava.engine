#include "Engine/Private/Win32/AppInstanceMonitorImplWin32.h"

#if defined(__DAVAENGINE_WIN32__)

#include "Engine/AppInstanceMonitor.h"
#include "Utils/UTF8Utils.h"

#include <cassert>
#include <process.h>

namespace DAVA
{
namespace Private
{
AppInstanceMonitorImpl::AppInstanceMonitorImpl(AppInstanceMonitor* appInstanceMonitor, const char* uniqueAppId)
    : appInstanceMonitor(appInstanceMonitor)
{
    assert(uniqueAppId != nullptr);

    const WideString wideUniqueAppId = UTF8Utils::EncodeToWideString(uniqueAppId);
    const WideString mutexName = wideUniqueAppId + L"_dava_mutex";
    const WideString eventName = wideUniqueAppId + L"_dava_event";
    const WideString mappingName = wideUniqueAppId + L"_dava_mapping";

    hmutex = ::CreateMutexW(nullptr, TRUE, mutexName.c_str());
    assert(hmutex != nullptr);

    DWORD err = ::GetLastError();
    if (err == NOERROR)
    {
        isAnotherInstanceRunning = false;

        hevent = ::CreateEventW(nullptr, FALSE, FALSE, eventName.c_str());
        hmapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, SharedMemorySize, mappingName.c_str());
        sharedMemory = ::MapViewOfFile(hmapping, FILE_MAP_WRITE, 0, 0, 0);
        if (sharedMemory != nullptr)
        {
            stringLength = static_cast<uint32*>(sharedMemory);
            stringData = static_cast<char*>(sharedMemory) + sizeof(uint32);
            *stringLength = 0;

            auto threadProc = [](void* arg) -> unsigned int {
                AppInstanceMonitorImpl* self = static_cast<AppInstanceMonitorImpl*>(arg);
                self->MonitorThread();
                return 0;
            };
            hthread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, threadProc, this, 0, nullptr));
        }

        ::ReleaseMutex(hmutex);
    }
    else if (err == ERROR_ALREADY_EXISTS)
    {
        isAnotherInstanceRunning = true;

        ::WaitForSingleObject(hmutex, INFINITE);

        hevent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, eventName.c_str());
        hmapping = ::OpenFileMappingW(FILE_MAP_WRITE, FALSE, mappingName.c_str());
        sharedMemory = ::MapViewOfFile(hmapping, FILE_MAP_WRITE, 0, 0, 0);

        if (sharedMemory != nullptr)
        {
            stringLength = static_cast<uint32*>(sharedMemory);
            stringData = static_cast<char*>(sharedMemory) + sizeof(uint32);
        }

        ::ReleaseMutex(hmutex);
    }
}

AppInstanceMonitorImpl::~AppInstanceMonitorImpl()
{
    if (hthread != nullptr)
    {
        finishThread = true;
        ::SetEvent(hevent);
        ::WaitForSingleObject(hthread, INFINITE);
        ::CloseHandle(hthread);
    }

    if (sharedMemory != nullptr)
        ::UnmapViewOfFile(sharedMemory);
    if (hmapping != nullptr)
        ::CloseHandle(hmapping);
    if (hevent != nullptr)
        ::CloseHandle(hevent);
    if (hmutex != nullptr)
        ::CloseHandle(hmutex);
}

bool AppInstanceMonitorImpl::IsAnotherInstanceRunning() const
{
    return isAnotherInstanceRunning;
}

void AppInstanceMonitorImpl::PassActivationFilenameToAnotherInstance(const String& filename)
{
    assert(IsAnotherInstanceRunning());

    ::WaitForSingleObject(hmutex, INFINITE);

    const uint32 length = static_cast<uint32>(filename.length());
    if (0 < length && length <= MaxDataSize)
    {
        *stringLength = length;
        std::memcpy(stringData, filename.c_str(), length);

        ::SetEvent(hevent);
    }

    ::ReleaseMutex(hmutex);
}

void AppInstanceMonitorImpl::MonitorThread()
{
    for (;;)
    {
        ::WaitForSingleObject(hevent, INFINITE);
        if (finishThread != 0)
            break;

        ::WaitForSingleObject(hmutex, INFINITE);

        if (0 < *stringLength && *stringLength <= MaxDataSize)
        {
            appInstanceMonitor->ActivatedWithFilenameFromAnotherInstance(String{ stringData, stringData + *stringLength });
            *stringLength = 0;
        }

        ::ReleaseMutex(hmutex);
    }
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
