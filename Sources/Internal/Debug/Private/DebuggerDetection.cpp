#include "Debug/DebuggerDetection.h"

#if defined(__DAVAENGINE_WIN32__)
#include <Base/Platform.h>
#include <debugapi.h>
#elif defined(__DAVAENGINE_MACOS__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

namespace DAVA
{
bool IsDebuggerPresent()
{
#if defined(__DAVAENGINE_WIN32__)
    return ::IsDebuggerPresent() == TRUE;
#elif defined(__DAVAENGINE_MACOS__)
    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);

    info.kp_proc.p_flag = 0;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

    return (info.kp_proc.p_flag & P_TRACED) != 0;
#else
    return false;
#endif
}

} // namespace DAVA
