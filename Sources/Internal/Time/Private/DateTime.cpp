#include "Time/DateTime.h"

#include <cstdlib>
#include <cctype>
#include <algorithm>

#ifdef __DAVAENGINE_WINDOWS__
#include <ctime>
#endif

#define SKIP_WHITESPACE while (*s == ' ' || *s == '\t') s++;
#define SKIP_NON_WHITESPACE while (*s != ' ' && *s != '\t' && *s != '\0') s++;
#define CHECK_PREMATURE_END    if (*s == '\0') return false;

#define SECONDS_IN_HOUR 3600
#define MAX_OFFSET_IN_HOURS 12

static const char* days[] = { "sun", "mon", "tue", "wed", "thu", "fri", "sat" };
static const char* months[] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };

namespace DAVA
{
DateTime::DateTime(Timestamp timeStamp, int32 _timeZoneOffset)
    : innerTime(timeStamp)
    , timeZoneOffset(_timeZoneOffset)
{
    UpdateLocalTimeStructure();
}

DateTime::DateTime(int32 year, int32 month, int32 day, int32 _timeZoneOffset)
    : timeZoneOffset(_timeZoneOffset)
{
    DVASSERT(year >= 0 && month >= 0 && day >= 0 &&
             timeZoneOffset <= (MAX_OFFSET_IN_HOURS * SECONDS_IN_HOUR) && timeZoneOffset >= (-1 * MAX_OFFSET_IN_HOURS * SECONDS_IN_HOUR));
    tm t = { 0 };
    t.tm_mon = month;
    t.tm_year = year - 1900;
    t.tm_isdst = -1;
    t.tm_mday = day;
    innerTime = InternalTimeGm(&t);
    innerTime -= timeZoneOffset; // time member should be always in UTC format
    UpdateLocalTimeStructure();
    // dates before 1970 is not supported
    DVASSERT(innerTime >= 0);
}

DateTime::DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 _timeZoneOffset)
    : timeZoneOffset(_timeZoneOffset)
{
    DVASSERT(year >= 0 && month >= 0 && day >= 0 && hour >= 0 && minute >= 0 && second >= 0 &&
             timeZoneOffset <= (MAX_OFFSET_IN_HOURS * SECONDS_IN_HOUR) && timeZoneOffset >= (-1 * MAX_OFFSET_IN_HOURS * SECONDS_IN_HOUR));
    tm t = { 0 };
    t.tm_sec = second;
    t.tm_min = minute;
    t.tm_hour = hour;
    t.tm_mon = month;
    t.tm_year = year - 1900;
    t.tm_isdst = -1;
    t.tm_mday = day;
    innerTime = InternalTimeGm(&t);
    innerTime -= timeZoneOffset;
    UpdateLocalTimeStructure();
    // dates before 1970 is not supported
    DVASSERT(innerTime >= 0);
}

DateTime DateTime::Now()
{
    return DateTime(time(nullptr), GetLocalTimeZoneOffset());
}

DateTime DateTime::LocalTime(Timestamp timeStamp)
{
    return DateTime(timeStamp, GetLocalTimeZoneOffset());
}

DateTime DateTime::GmTime(Timestamp timeStamp)
{
    return DateTime(timeStamp, 0);
}

DateTime DateTime::ConvertToTimeZone(int32 _timeZoneOff)
{
    return DateTime(innerTime, _timeZoneOff);
}

DateTime DateTime::ConvertToLocalTimeZone()
{
    return ConvertToTimeZone(GetLocalTimeZoneOffset());
}

int32 DateTime::GetYear() const
{
    return localTime.tm_year + 1900;
}

int32 DateTime::GetMonth() const
{
    return localTime.tm_mon;
}

int32 DateTime::GetDay() const
{
    return localTime.tm_mday;
}

int32 DateTime::GetHour() const
{
    return localTime.tm_hour;
}

int32 DateTime::GetMinute() const
{
    return localTime.tm_min;
}

int32 DateTime::GetSecond() const
{
    return localTime.tm_sec;
}

int32 DateTime::GetMillisecond() const
{
    return innerMilliseconds;
}

void DateTime::SetTimeZoneOffset(int32 value)
{
    timeZoneOffset = value;
    UpdateLocalTimeStructure();
}

Timestamp DateTime::GetRawTimeStampForCurrentTZ() const
{
    return innerTime + timeZoneOffset;
}

void DateTime::UpdateLocalTimeStructure()
{
    Timestamp timeWithTimeZone = GetRawTimeStampForCurrentTZ();
    GmTimeThreadSafe(&localTime, &timeWithTimeZone);
}

void DateTime::GmTimeThreadSafe(tm* resultGmTime, const time_t* unixTimestamp)
{
#ifdef __DAVAENGINE_WINDOWS__
    gmtime_s(resultGmTime, unixTimestamp);
#else
    gmtime_r(unixTimestamp, resultGmTime);
#endif
}

void DateTime::LocalTimeThreadSafe(tm* resultLocalTime, const time_t* unixTimestamp)
{
#ifdef __DAVAENGINE_WINDOWS__
    localtime_s(resultLocalTime, unixTimestamp);
#else
    localtime_r(unixTimestamp, resultLocalTime);
#endif
}

// like 1969-07-21T02:56:15Z
bool DateTime::ParseISO8601Date(const DAVA::String& src)
{
    // 1969-07-21T02:56:15Z
    // 1969-07-21T02:56:15.1Z
    // 1969-07-20T21:56:15-05:00
    size_t const minLengthWithoutMilliseconds = std::strlen("1969-07-21T02:56:15Z");
    if (src.length() < minLengthWithoutMilliseconds)
    {
        return false;
    }

    tm parseTime = { 0 };
    /// parsing date part
    {
        const size_t substringYearLength = 4;
        const DAVA::String yr = src.substr(0, substringYearLength);
        if (!IsNumber(yr))
        {
            return false;
        }

        parseTime.tm_year = std::atoi(yr.c_str()) - 1900;
        if (parseTime.tm_year < 0)
        {
            return false;
        }

        const DAVA::String mon = src.substr(5, 2);
        if (!IsNumber(mon))
        {
            return false;
        }

        parseTime.tm_mon = std::atoi(mon.c_str()) - 1;
        if (parseTime.tm_mon < 0 || parseTime.tm_mon > 11)
        {
            return false;
        }

        const DAVA::String dy = src.substr(8, 2);
        if (!IsNumber(dy))
        {
            return false;
        }

        parseTime.tm_mday = std::atoi(dy.c_str());
        if (parseTime.tm_mday < 1 || parseTime.tm_mday > 31)
        {
            return false;
        }
    }

    /// time
    int milliseconds = 0;
    size_t firstTimeZoneSymbolIndex;
    {
        const DAVA::String hr = src.substr(11, 2);
        if (!IsNumber(hr))
        {
            return false;
        }

        parseTime.tm_hour = std::atoi(hr.c_str());
        if (parseTime.tm_hour < 0 || parseTime.tm_hour > 23)
        {
            return false;
        }

        const DAVA::String mn = src.substr(14, 2);
        if (!IsNumber(mn))
        {
            return false;
        }

        parseTime.tm_min = std::atoi(mn.c_str());
        if (parseTime.tm_min < 0 || parseTime.tm_min > 59)
        {
            return false;
        }

        const DAVA::String sc = src.substr(17, 2);
        if (!IsNumber(sc))
        {
            return false;
        }

        parseTime.tm_sec = std::atoi(sc.c_str());
        if (parseTime.tm_sec < 0 || parseTime.tm_sec > 59)
        {
            return false;
        }

        // Parsing milliseconds (if specified)

        const char separator = src[19];

        // Can be both . or ,
        if (separator == '.' || separator == ',')
        {
            // Count milliseconds length and multiplier

            size_t currentSymbolIndex = 20;
            float mul = 1.0;
            while (currentSymbolIndex < src.length() && std::isdigit(src[currentSymbolIndex]))
            {
                ++currentSymbolIndex;
                mul *= 0.1f;
            }

            const size_t millisecondsSubstringLength = currentSymbolIndex - 20;
            if (millisecondsSubstringLength > 0)
            {
                const DAVA::String millisecondsSubstring = src.substr(20, millisecondsSubstringLength);
                const int millisecondsFractional = std::atoi(millisecondsSubstring.c_str());

                // Convert fraction to int (from 0 to 999)
                // Android NDK might not have std::round, so use global namespace
                milliseconds = static_cast<int>(::round(millisecondsFractional * mul * 1000));

                DVASSERT(milliseconds >= 0 && milliseconds < 1000);

                // Now when we know how many symbols milliseconds take, rough-check again if string is valid
                // For cases when milliseconds were specified but time zone wasn't specified at all
                if (src.length() < minLengthWithoutMilliseconds + (millisecondsSubstringLength + 1))
                {
                    return false;
                }
            }
            else
            {
                // Treat as an error if separator exists but no fraction specified
                return false;
            }

            firstTimeZoneSymbolIndex = currentSymbolIndex;
        }
        else
        {
            firstTimeZoneSymbolIndex = 19;
        }
    }
    /// time zone
    {
        int32 timeZone = 0;
        if (src[firstTimeZoneSymbolIndex] == 'Z')
        {
            timeZone = 0;
        }
        else
        {
            if (src[firstTimeZoneSymbolIndex] == '-')
            {
                timeZone = -1;
            }
            else if (src[firstTimeZoneSymbolIndex] == '+')
            {
                timeZone = 1;
            }
            else
            {
                return false;
            }

            const DAVA::String hr = src.substr(firstTimeZoneSymbolIndex + 1, 2);
            if (!IsNumber(hr))
            {
                return false;
            }

            int32 tzHour = std::atoi(hr.c_str());
            if (tzHour < 0 || tzHour > 23)
            {
                return false;
            }

            const DAVA::String mn = src.substr(firstTimeZoneSymbolIndex + 4, 2);
            if (!IsNumber(mn))
            {
                return false;
            }

            int32 tzMinute = std::atoi(mn.c_str());
            if (tzMinute < 0 || tzMinute > 59)
            {
                return false;
            }

            timeZoneOffset = (timeZone * (tzHour * 60 + tzMinute)) * 60;
        }
    }
    innerTime = InternalTimeGm(&parseTime) - timeZoneOffset;
    innerMilliseconds = milliseconds;
    UpdateLocalTimeStructure();
    return true;
}

/*
     SYNTAX
     
     date-time   =  [ day "," ] date time       ; dd mm yy
                                                ;  hh:mm:ss zzz
     
     day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
                 /  "Fri"  / "Sat" /  "Sun"
     
     date        =  1*2DIGIT month 2DIGIT       ; day month year
                                                ;  e.g. 20 Jun 82
     
     month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
                 /  "May"  /  "Jun" /  "Jul"  /  "Aug"
                 /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
     
     time        =  hour zone                   ; ANSI and Military
     
     hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
                                                ; 00:00:00 - 23:59:59
     
     zone        =  "UT"  / "GMT"               ; Universal Time
                                                ; North American : UT
                 /  "EST" / "EDT"               ;  Eastern:  - 5/ - 4
                 /  "CST" / "CDT"               ;  Central:  - 6/ - 5
                 /  "MST" / "MDT"               ;  Mountain: - 7/ - 6
                 /  "PST" / "PDT"               ;  Pacific:  - 8/ - 7
                 /  1ALPHA                      ; Military: Z = UT;
                                                ;  A:-1; (J not used)
                                                ;  M:-12; N:+1; Y:+12
                 / ( ("+" / "-") 4DIGIT )       ; Local differential
                                                ;  hours+min. (HHMM)
     
     SEMANTICS
     
     If included, day-of-week must be the day implied by the date
     specification.
     
     Time zone may be indicated in several ways.  "UT" is Univer-
     sal  Time  (formerly called "Greenwich Mean Time"); "GMT" is per-
     mitted as a reference to Universal Time.  The  military  standard
     uses  a  single  character for each zone.  "Z" is Universal Time.
     "A" indicates one hour earlier, and "M" indicates 12  hours  ear-
     lier;  "N"  is  one  hour  later, and "Y" is 12 hours later.  The
     letter "J" is not used.  The other remaining two forms are  taken
     from ANSI standard X3.51-1975.  One allows explicit indication of
     the amount of offset from UT; the other uses  common  3-character
     strings for indicating time zones in North America.
     
     */
bool DateTime::ParseRFC822Date(const DAVA::String& src)
{
    if (src.empty())
    {
        return false;
    }
    const char* str1 = src.c_str();
    int32 dayOfWeek = 0;

    char str[200], *s;
    s = &str[0];
    std::strncpy(str, str1, 199);
    str[199] = '\0';

    // Convert to lowercase.
    int32 j;
    int32 i = 0;
    while (str[i] != '\0')
    {
        str[i] = std::tolower(str[i]);
        i++;
    }

    SKIP_WHITESPACE
    CHECK_PREMATURE_END

    for (j = 0; j < 7; j++)
    {
        if (std::strncmp(s, days[j], 3) == 0)
        {
            break;
        }
    }

    if (j < 7)
    {
        SKIP_NON_WHITESPACE
        dayOfWeek = j;
        SKIP_WHITESPACE
        CHECK_PREMATURE_END
        if (*s == ',')
            s++;
        SKIP_WHITESPACE
        CHECK_PREMATURE_END
    }

    // Get the day, month, and year.
    int32 day;
    char monthStr[20] = { 0 };
    int32 month;
    int32 year;

    if (std::sscanf(s, "%d%19s%d", &day, monthStr, &year) != 3)
        return false;
    SKIP_NON_WHITESPACE
    SKIP_WHITESPACE
    SKIP_NON_WHITESPACE
    SKIP_WHITESPACE
    SKIP_NON_WHITESPACE
    SKIP_WHITESPACE

    for (j = 0; j < 12; j++)
    {
        if (std::strncmp(monthStr, months[j], 3) == 0)
        {
            break;
        }
    }
    if (j == 12)
    {
        return false;
    }
    month = j;

    if (year < 1900)
    {
        if (year < 50)
        {
            year += 2000;
        }
        else
        {
            year += 1900;
        }
    }

    int32 hour, minute, seconds;

    if (std::sscanf(s, "%d:%d:%d", &hour, &minute, &seconds) != 3)
    {
        return false;
    }
    SKIP_NON_WHITESPACE
    SKIP_WHITESPACE

    if (*s == '+')
        s++;
    char zoneStr[20] = { 0 };
    if (std::sscanf(s, "%19s", zoneStr) != 1)
    {
        std::strcpy(zoneStr, "GMT");
    }

    if (zoneStr[0] == '-' ||
        (zoneStr[0] >= '0' && zoneStr[0] <= '9'))
    {
        // Do nothing.
    }
    else if (std::strcmp(zoneStr, "ut") == 0)
    {
        std::strcpy(zoneStr, "0000");
    }
    else if (std::strcmp(zoneStr, "gmt") == 0)
    {
        std::strcpy(zoneStr, "0000");
    }
    else if (std::strcmp(zoneStr, "est") == 0)
    {
        std::strcpy(zoneStr, "-0500");
    }
    else if (std::strcmp(zoneStr, "edt") == 0)
    {
        std::strcpy(zoneStr, "-0400");
    }
    else if (std::strcmp(zoneStr, "cst") == 0)
    {
        std::strcpy(zoneStr, "-0600");
    }
    else if (std::strcmp(zoneStr, "cdt") == 0)
    {
        std::strcpy(zoneStr, "-0500");
    }
    else if (std::strcmp(zoneStr, "mst") == 0)
    {
        std::strcpy(zoneStr, "-0700");
    }
    else if (std::strcmp(zoneStr, "mdt") == 0)
    {
        std::strcpy(zoneStr, "-0600");
    }
    else if (std::strcmp(zoneStr, "pst") == 0)
    {
        std::strcpy(zoneStr, "-0800");
    }
    else if (std::strcmp(zoneStr, "pdt") == 0)
    {
        std::strcpy(zoneStr, "-0700");
    }
    else if (std::strcmp(zoneStr, "a") == 0)
    {
        std::strcpy(zoneStr, "-0100");
    }
    else if (std::strcmp(zoneStr, "z") == 0)
    {
        std::strcpy(zoneStr, "0000");
    }
    else if (std::strcmp(zoneStr, "m") == 0)
    {
        std::strcpy(zoneStr, "-1200");
    }
    else if (std::strcmp(zoneStr, "n") == 0)
    {
        std::strcpy(zoneStr, "0100");
    }
    else if (std::strcmp(zoneStr, "y") == 0)
    {
        std::strcpy(zoneStr, "1200");
    }
    else
    {
        std::strcpy(zoneStr, "0000");
    }

    // Convert zoneStr from (-)hhmm to minutes.
    int32 hh, mm;
    int32 sign = 1;
    s = zoneStr;
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    if (std::sscanf(s, "%02d%02d", &hh, &mm) != 2)
    {
        return false;
    }
    int32 adjustment = sign * ((hh * 60) + mm);
    tm t = { 0 };
    t.tm_year = year - 1900;
    t.tm_mon = month;
    t.tm_wday = dayOfWeek;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = seconds;
    timeZoneOffset = adjustment * 60;
    innerTime = InternalTimeGm(&t) - timeZoneOffset;
    UpdateLocalTimeStructure();
    return true;
}

// calc time_t by hand to get time stamp in utc(system function
// returns it with local timezone shift only )
// code from http://stackoverflow.com/questions/16647819/timegm-cross-platform
Timestamp DateTime::InternalTimeGm(tm* t) const
{
    int32 year = t->tm_year + 1900;
    int32 month = t->tm_mon;
    if (month > 11)
    {
        year += month / 12;
        month %= 12;
    }
    else if (month < 0)
    {
        int32 yearsDiff = (-month + 11) / 12;
        year -= yearsDiff;
        month += 12 * yearsDiff;
    }

    int32 day = t->tm_mday;
    int32 dayOfYear = DaysFrom1jan(year, month, day);
    int32 daysSinceEpoch = DaysFrom1970(year) + dayOfYear;

    Timestamp secondsInDay = 3600 * 24;
    Timestamp result = secondsInDay * daysSinceEpoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

    return result;
}

bool DateTime::IsNumber(const String& s) const
{
    //http://stackoverflow.com/questions/8888748/how-to-check-if-given-c-string-or-char-contains-only-digits
    return std::all_of(s.begin(), s.end(), static_cast<int (*)(int)>(std::isdigit));
}
};
