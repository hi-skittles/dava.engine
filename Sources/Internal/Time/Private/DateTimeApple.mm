#include "Time/DateTime.h"

#if defined(__DAVAENGINE_APPLE__)

#include "FileSystem/LocalizationSystem.h"

#include <time.h>
#include <xlocale.h>

#import <Foundation/Foundation.h>

#include "Utils/UTF8Utils.h"

namespace DAVA
{
DAVA::WideString DateTime::AsWString(const wchar_t* format) const
{
    DAVA::String locID = LocalizationSystem::Instance()->GetCountryCode();

    struct tm timeinfo;
    wchar_t buffer[256] = { 0 };

    Timestamp timeWithTZ = innerTime + timeZoneOffset;

    GmTimeThreadSafe(&timeinfo, &timeWithTZ);

    locale_t loc = newlocale(LC_ALL_MASK, locID.c_str(), 0);
    size_t size = wcsftime_l(buffer, 256, format, &timeinfo, loc);
    DVASSERT(size);
    freelocale(loc);
    DAVA::WideString str(buffer);

    return str;
}

static WideString GetLocalizedDateOrTime(NSDateFormatterStyle timeStyle,
                                         NSDateFormatterStyle dateStyle,
                                         int32 timeZoneOffset,
                                         time_t innerTime)
{
    NSTimeZone* timeZone = [NSTimeZone timeZoneForSecondsFromGMT:timeZoneOffset];

    tm timeinfo;
    gmtime_r(&innerTime, &timeinfo);

    DAVA::String locID = LocalizationSystem::Instance()->GetCountryCode();

    NSString* locale_name = [NSString stringWithCString:locID.c_str()
                                               encoding:[NSString defaultCStringEncoding]];

    NSLocale* locale = [[NSLocale alloc] initWithLocaleIdentifier:locale_name];

    NSDate* date = [NSDate dateWithTimeIntervalSince1970:innerTime];

    NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];

    [dateFormatter setLocale:locale];
    [dateFormatter setTimeZone:timeZone];

    dateFormatter.timeStyle = timeStyle;
    dateFormatter.dateStyle = dateStyle;

    NSString* date_str = [dateFormatter stringFromDate:date];

    std::string result = [date_str cStringUsingEncoding:[NSString defaultCStringEncoding]];

    [dateFormatter release];
    [locale release];

    return UTF8Utils::EncodeToWideString(result);
}

WideString DateTime::GetLocalizedDate() const
{
    return GetLocalizedDateOrTime(NSDateFormatterNoStyle,
                                  NSDateFormatterShortStyle,
                                  timeZoneOffset,
                                  innerTime);
}

WideString DateTime::GetLocalizedTime() const
{
    return GetLocalizedDateOrTime(NSDateFormatterMediumStyle,
                                  NSDateFormatterNoStyle,
                                  timeZoneOffset,
                                  innerTime);
}

int32 DateTime::GetLocalTimeZoneOffset()
{
    Timestamp t = time(nullptr);
    struct tm resultStruct = { 0 };
    DateTime::LocalTimeThreadSafe(&resultStruct, &t);
    return static_cast<int32>(resultStruct.tm_gmtoff);
}
}

#endif // __DAVAENGINE_APPLE__
