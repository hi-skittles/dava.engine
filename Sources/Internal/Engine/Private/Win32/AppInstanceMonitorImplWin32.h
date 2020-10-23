#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN32__)

#include "Base/BaseTypes.h"

#include <atomic>

namespace DAVA
{
class AppInstanceMonitor;

namespace Private
{
class AppInstanceMonitorImpl final
{
public:
    AppInstanceMonitorImpl(AppInstanceMonitor* appInstanceMonitor, const char* uniqueAppId);
    ~AppInstanceMonitorImpl();

    bool IsAnotherInstanceRunning() const;
    void PassActivationFilenameToAnotherInstance(const String& filename);

private:
    void MonitorThread();

    AppInstanceMonitor* appInstanceMonitor = nullptr;
    bool isAnotherInstanceRunning = false;

    HANDLE hmutex = nullptr;
    HANDLE hevent = nullptr;
    HANDLE hmapping = nullptr;
    void* sharedMemory = nullptr;
    uint32* stringLength = nullptr;
    char* stringData = nullptr;

    HANDLE hthread = nullptr;
    std::atomic<bool> finishThread = { false };

    static const uint32 SharedMemorySize = 4096 * 2;
    static const uint32 MaxDataSize = SharedMemorySize - sizeof(uint32);
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_WIN32__
