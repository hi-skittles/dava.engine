#include "Base/BaseTypes.h"
#include "Base/Platform.h"

using namespace DAVA;

#if defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_LINUX__)

#include <unistd.h>

int32 GetCpuCount()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

#elif defined(__DAVAENGINE_WINDOWS__)

int32 GetCpuCount()
{
    SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    return static_cast<int32>(sysinfo.dwNumberOfProcessors);
}

#endif
