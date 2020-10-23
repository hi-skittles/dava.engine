#include "Time/DateTime.h"

#if defined(__DAVAENGINE_WINDOWS__)

#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"
#include "FileSystem/LocalizationSystem.h"

namespace DAVA
{
DAVA::WideString DateTime::AsWString(const wchar_t* format) const
{
    String configLocale = LocalizationSystem::Instance()->GetCountryCode();
    configLocale.replace(configLocale.find("_"), 1, "-");

    Timestamp timeWithTZ = innerTime + timeZoneOffset;

    tm timeinfo;
    GmTimeThreadSafe(&timeinfo, &timeWithTZ);

    _locale_t loc = _create_locale(LC_ALL, configLocale.c_str());
    DVASSERT(loc);

    std::array<wchar_t, 256> buffer;
    _wcsftime_l(&buffer[0], buffer.size(), format, &timeinfo, loc);

    DAVA::WideString str(buffer.data());
    _free_locale(loc);
    return str;
}

int32 DateTime::GetLocalTimeZoneOffset()
{
    TIME_ZONE_INFORMATION TimeZoneInfo;
    GetTimeZoneInformation(&TimeZoneInfo);

    // TimeZoneInfo.Bias is the difference between local time
    // and GMT in minutes.
    return TimeZoneInfo.Bias * (-60);
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

#endif // __DAVAENGINE_WINDOWS__
