#include "Time/DateTime.h"

#if defined(__DAVAENGINE_LINUX__)

#include "FileSystem/LocalizationSystem.h"
#include "Utils/UTF8Utils.h"

#include <ctime>
#include <clocale>
#include <sys/time.h>

namespace DAVA
{
WideString DateTime::AsWString(const wchar_t* format) const
{
    String configLocale = LocalizationSystem::Instance()->GetCountryCode();

    struct tm tm = {};
    Timestamp timeWithTZ = innerTime + timeZoneOffset;
    GmTimeThreadSafe(&tm, &timeWithTZ);

    char buf[256] = {};
    String formatUtf8 = UTF8Utils::MakeUTF8String(format);

    locale_t loc = newlocale(LC_ALL, configLocale.c_str(), nullptr);
    DVASSERT(loc != nullptr);
    if (loc != nullptr)
    {
        strftime_l(buf, 256, formatUtf8.c_str(), &tm, loc);
        freelocale(loc);
    }
    else
    {
        strftime(buf, 256, formatUtf8.c_str(), &tm);
    }
    return UTF8Utils::EncodeToWideString(buf);
}

int32 DateTime::GetLocalTimeZoneOffset()
{
    struct timeval tv = {};
    struct timezone tz = {};
    gettimeofday(&tv, &tz);
    return -static_cast<int32>(tz.tz_minuteswest) * 60;
}

WideString DateTime::GetLocalizedDate() const
{
    return AsWString(L"%x");
}

WideString DateTime::GetLocalizedTime() const
{
    return AsWString(L"%X");
}
}

#endif // __DAVAENGINE_LINUX__
