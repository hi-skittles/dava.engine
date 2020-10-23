#include "Time/SystemTimer.h"

#include "Base/Platform.h"
#include "Debug/DVAssert.h"
#include "Debug/Replay.h"

#include <chrono>

#if defined(__DAVAENGINE_APPLE__)
#include <sys/sysctl.h>
#include <sys/time.h>
#elif defined(__DAVAENGINE_ANDROID__)
#include <sys/time.h>
// Crystax NDK complains that CLOCK_BOOTTIME is undefined symbol, though it is present in headers
// Google NDK compiles well
#ifndef CLOCK_BOOTTIME
#define CLOCK_BOOTTIME 7
#endif
#elif defined(__DAVAENGINE_LINUX__)
#include <sys/sysinfo.h>
#endif

namespace DAVA
{
int64 SystemTimer::frameTimestamp = 0;
int64 SystemTimer::frameTimestampForRealDelta = 0;
float32 SystemTimer::frameDelta = 0.f;
float32 SystemTimer::realFrameDelta = 0.f;
float32 SystemTimer::globalTime = 0.f;
bool SystemTimer::globalTimePaused = false;
int64 SystemTimer::adjustmentMillis = 0;
int64 SystemTimer::adjustmentMicros = 0;
int64 SystemTimer::adjustmentNanos = 0;

#if defined(_MSC_VER) && _MSC_VER <= 1800
// Ops, MSVC2013 and earlier uses low resulution clock for std::chrono::high_resolution_clock
// which has been acknowledged as a bug, see https://web.archive.org/web/20141212192132/https://connect.microsoft.com/VisualStudio/feedback/details/719443/
// This bug was fixed in MSVC2015.
struct msvc2013_high_resolution_clock
{
    typedef long long rep;
    typedef std::nano period;
    typedef std::chrono::nanoseconds duration;
    typedef std::chrono::time_point<msvc2013_high_resolution_clock> time_point;
    static const bool is_steady = true;

    static long long GetPerfFrequency()
    {
        static long long freq = 0;
        if (freq == 0)
        {
            // The frequency of the performance counter is fixed at system boot
            LARGE_INTEGER li;
            ::QueryPerformanceFrequency(&li);
            freq = li.QuadPart;
        }
        return freq;
    }

    static long long GetPerfCounter()
    {
        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }

    static time_point now()
    {
        const long long freq = GetPerfFrequency();
        const long long cntr = GetPerfCounter();
        const long long whole = (cntr / freq) * period::den;
        const long long part = (cntr % freq) * period::den / freq;
        return time_point(duration(whole + part));
    }
};
using SystemTimerHighResolutionClock = msvc2013_high_resolution_clock;
#else
using SystemTimerHighResolutionClock = std::chrono::high_resolution_clock;
#endif

int64 SystemTimer::GetMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(SystemTimerHighResolutionClock::now().time_since_epoch()).count() + adjustmentMillis;
}

int64 SystemTimer::GetUs()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(SystemTimerHighResolutionClock::now().time_since_epoch()).count() + adjustmentMicros;
}

int64 SystemTimer::GetNs()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(SystemTimerHighResolutionClock::now().time_since_epoch()).count() + adjustmentNanos;
}

int64 SystemTimer::GetSystemUptimeUs()
{
#if defined(__DAVAENGINE_WINDOWS__)
    // Windows provides only milliseconds elapsed from boot
    return static_cast<int64>(::GetTickCount64()) * 1000ll;
#elif defined(__DAVAENGINE_APPLE__)
    // iOS and macOS keep UTC boot timestamp which is updated when machine clock is adjusted.
    // So time elapsed from boot is the difference between current time and boot time.
    timeval bootTime;
    timeval curTime;
    gettimeofday(&curTime, nullptr);

    size_t timevalSize = sizeof(bootTime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    sysctl(mib, 2, &bootTime, &timevalSize, nullptr, 0);

    int64 bootTimeMicro = bootTime.tv_sec * 1000000ll + bootTime.tv_usec;
    int64 curTimeMicro = curTime.tv_sec * 1000000ll + curTime.tv_usec;
    return curTimeMicro - bootTimeMicro;
#elif defined(__DAVAENGINE_ANDROID__)
    timespec bootTime;
    clock_gettime(CLOCK_BOOTTIME, &bootTime);
    return bootTime.tv_sec * 1000000ll + bootTime.tv_nsec / 1000ll;
#elif defined(__DAVAENGINE_LINUX__)
    struct sysinfo info
    {
    };
    sysinfo(&info);
    return info.uptime * 1000000ll;
#else
#error "SystemTimer: unknown platform"
#endif
}

int64 SystemTimer::GetFrameTimestampMs()
{
    return frameTimestamp / 1000;
}

float32 SystemTimer::GetFrameDelta()
{
    return frameDelta;
}

float32 SystemTimer::GetRealFrameDelta()
{
    return realFrameDelta;
}

void SystemTimer::SetFrameDelta(float32 delta)
{
    DVASSERT(Replay::IsPlayback() || Replay::IsRecord());
    frameDelta = delta;
}

DAVA_DEPRECATED(float32 SystemTimer::GetGlobalTime())
{
    return globalTime;
}

DAVA_DEPRECATED(void SystemTimer::UpdateGlobalTime(float32 timeElapsed))
{
    if (!globalTimePaused)
    {
        globalTime += timeElapsed;
    }
}

DAVA_DEPRECATED(void SystemTimer::ResetGlobalTime())
{
    globalTime = 0.f;
}

DAVA_DEPRECATED(void SystemTimer::PauseGlobalTime())
{
    globalTimePaused = true;
}

DAVA_DEPRECATED(void SystemTimer::ResumeGlobalTime())
{
    globalTimePaused = false;
}

void SystemTimer::StartFrame()
{
    if (frameTimestamp == 0)
    {
        frameTimestamp = GetUs();
        frameTimestampForRealDelta = frameTimestamp;
    }

    int64 timestamp = GetUs();
    float32 delta = static_cast<float32>((timestamp - frameTimestamp) / 1000000.0);

    realFrameDelta = delta;
    frameDelta = std::min(0.1f, std::max(0.0004f, delta));

    frameTimestamp = timestamp;
}

void SystemTimer::ComputeRealFrameDelta()
{
    realFrameDelta = static_cast<float32>((frameTimestamp - frameTimestampForRealDelta) / 1000000.0);
    frameTimestampForRealDelta = frameTimestamp;
}

void SystemTimer::Adjust(int64 micros)
{
    /*
        On several platforms GetMs() and friends may stop after some amount of time
        when device enters sleep mode (screen is darkened, power cable is not connected).
        To ensure clock monotonicity SystemTimer uses time adjustment which tells how long
        device has spent in deep sleep.
        Plaform backend knows how to compute time spent is deep sleep and indirectly calls 
        SystemTimer::Adjust.

        Timer behavior without adjustment:
          active |      sleep      | again active
        1  2  3  4  5  6  6  6  6  7  8  9  10          <--- timer without adjustment
        1  2  3  4  5  6  6  6  6  10 11 12 13          <--- timer with adjustment
                                   ^
                                  / \
                                   |
                           here adjust timer
                               by 6 ticks

        Device spent 6 ticks in deep sleep and timer continues from value when clock has stopped.
        Without adjustment real frame delta will be 3 tick (defference between 7 and 4) instead of 6.
        Platform impementation can measure time spent in deep sleep and adjust SystemTimer by 6 ticks
        and real frame delta will be real 6 ticks.
    */
    // Precompute adjustments for clocks with various precision
    adjustmentMicros += micros;
    adjustmentMillis += micros / 1000;
    adjustmentNanos += micros * 1000;
}

} // namespace DAVA
