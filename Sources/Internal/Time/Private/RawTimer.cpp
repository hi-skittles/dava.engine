#include "Time/RawTimer.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
void RawTimer::Start()
{
    timerStartTime = SystemTimer::GetMs();
    isStarted = true;
}

void RawTimer::Stop()
{
    isStarted = false;
}

void RawTimer::Resume()
{
    isStarted = true;
}

bool RawTimer::IsStarted()
{
    return isStarted;
}

int64 RawTimer::GetElapsed()
{
    if (isStarted)
    {
        return SystemTimer::GetMs() - timerStartTime;
    }
    else
    {
        return 0;
    }
}
}