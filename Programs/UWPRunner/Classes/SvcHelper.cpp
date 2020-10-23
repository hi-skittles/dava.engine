#include "Concurrency/Thread.h"
#include "SvcHelper.h"

using namespace DAVA;

SvcHelper::SvcHelper(const WideString& name)
    : serviceName(name)
{
    serviceControlManager = ::OpenSCManagerW(nullptr, nullptr, 0);
    if (!serviceControlManager)
    {
        return;
    }

    service = ::OpenServiceW(serviceControlManager, name.c_str(), SC_MANAGER_ALL_ACCESS);
}

SvcHelper::~SvcHelper()
{
    if (service)
        ::CloseServiceHandle(service);

    if (serviceControlManager)
        ::CloseServiceHandle(serviceControlManager);
}

WideString SvcHelper::ServiceName() const
{
    return serviceName;
}

WideString SvcHelper::ServiceDescription() const
{
    Array<wchar_t, 8 * 1024> data;
    LPBYTE infoData = reinterpret_cast<LPBYTE>(data.data());
    DWORD infoSize = static_cast<DWORD>(data.size());
    DWORD bytesNeeded;

    BOOL res = ::QueryServiceConfig2W(service,
                                      SERVICE_CONFIG_DESCRIPTION, infoData, infoSize, &bytesNeeded);

    if (!res)
    {
        return L"";
    }

    return WideString(data.data());
}

bool SvcHelper::IsInstalled() const
{
    return service != nullptr;
}

bool SvcHelper::IsRunning() const
{
    SERVICE_STATUS info;
    if (!::QueryServiceStatus(service, &info))
    {
        return false;
    }

    return info.dwCurrentState != SERVICE_STOPPED;
}

bool SvcHelper::Start()
{
    if (::StartServiceW(service, 0, nullptr) != TRUE)
    {
        return false;
    }

    SERVICE_STATUS status;
    bool running = false;
    int i = 0;

    while (!running && i < 10)
    {
        Thread::Sleep(500);
        if (!::QueryServiceStatus(service, &status))
        {
            break;
        }

        running = status.dwCurrentState == SERVICE_RUNNING;
        ++i;
    }

    return running;
}

bool SvcHelper::Stop()
{
    SERVICE_STATUS status;
    if (::ControlService(service, SERVICE_CONTROL_STOP, &status) == TRUE)
    {
        bool stopped = status.dwCurrentState == SERVICE_STOPPED;
        unsigned i = 0;

        while (!stopped && i < 10)
        {
            Thread::Sleep(200);
            if (!::QueryServiceStatus(service, &status))
            {
                break;
            }

            stopped = status.dwCurrentState == SERVICE_STOPPED;
            ++i;
        }

        return stopped;
    }

    return false;
}