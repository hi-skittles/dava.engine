#pragma once

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
using Timestamp = time_t;

class DateTime
{
public:
    DateTime() = default;
    /**
	 \brief Creates DateTime with specified params. Hours, seconds, minuntes will be setted to 0.
	 \param[in] year: 1969 and lower is not allowed.
     \param[in] month: 0-11.
     \param[in] day of the month: 1-31, attention: 0 as param will lead to last day of prev. month.
     \param[in] time zone offset in seconds.For example, for U.S. Eastern Standard Time, the value is -5*60*60.
	 */
    DateTime(int32 year, int32 month, int32 day, int32 timeZoneOffset);

    /**
	 \brief Creates DateTime with specified params.
	 \param[in] year: 1969 and lower is not allowed.
     \param[in] month: 0-11.
     \param[in] day of the month: 1-31, attention: 0 as param will lead to last day of prev. month.
     \param[in] hour: 0-23.
     \param[in] minute: 0-59.
     \param[in] second: 0-59.
     \param[in] time zone offset in seconds.For example, for U.S. Eastern Standard Time, the value is -5*60*60.
	 */
    DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 timeZoneOffset);

    /**
	 \brief Returns DateTime with shifted time zone offset to local one.
	 \param[in] input timeStamp will be recognized as in utc.
     \returns DateTime with local offset.
	 */
    static DateTime LocalTime(Timestamp);

    /**
	 \brief Uses the value pointed by Timestamp to fill DateTime with the values that represent the corresponding time, expressed as a UTC time (i.e., the time at the GMT timezone).
	 \param[in] input timeStamp.
     \returns DateTime in utc.
	 */
    static DateTime GmTime(Timestamp);

    /**
    \brief Get DateTime in current time zone on local mashing.
    \returns DateTime in local time zone.
    */
    static DateTime Now();

    /**
	 \brief Returns DateTime with changed time zone to specified one.
	 \param[in] time zone offset. For example, for U.S. Eastern Standard Time, the value is -5*60*60.
     \returns DateTime with mentioned time zone offset.
	 */
    DateTime ConvertToTimeZone(int32 timeZoneOffset);

    /**
	 \brief Returns DateTime with changed time zone local one.
     \returns DateTime with local time zone offset.
	 */
    DateTime ConvertToLocalTimeZone();

    /**
	 \brief Returns year of current DateTime according to inner time zone offset.
     \returns year.
	 */
    int32 GetYear() const;

    /**
	 \brief Returns month of current DateTime according to inner time zone offset.
     \returns month.
	 */
    int32 GetMonth() const;

    /**
	 \brief Returns day of current DateTime according to inner time zone offset.
     \returns day.
	 */
    int32 GetDay() const;

    /**
	 \brief Returns hour of current DateTime according to inner time zone offset.
     \returns hour.
	 */
    int32 GetHour() const;

    /**
	 \brief Returns minute of current DateTime according to inner time zone offset.
     \returns minute.
	 */
    int32 GetMinute() const;

    /**
	 \brief Returns second of current DateTime according to inner time zone offset.
     \returns second.
	 */
    int32 GetSecond() const;

    /**
	 \brief Returns millisecond of current DateTime.
	 \returns millisecond.
	 */
    int32 GetMillisecond() const;

    /**
	 \brief Set time zone offset in seconds for current DateTime.
	 \param[in] input time zone offset.
	 */
    void SetTimeZoneOffset(int32);

    /**
	 \brief Returns time zone offset on current Date Time in seconds.
	 \returns time zone offset.
	 */
    inline int32 GetTimeZoneOffset() const;

    /**
	 \brief Returns inner time stamp (amount of seconds since 1970) in utc.
	 \returns time stamp.
	 */
    inline Timestamp GetTimestamp() const;

    /**
	 \brief Parcing of string in format like "1969-07-21T02:56:15Z".
	 \param[in] string representation of date in ISO8601 standart.
     \returns bool value of success.
	 */
    bool ParseISO8601Date(const DAVA::String&);

    /**
	 \brief Parcing of string in format like "Wed, 27 Sep 2006 21:36:45 +0200".
	 \param[in] string representation of date in RFC822 standart.
     \returns bool value of success.
	 */
    bool ParseRFC822Date(const DAVA::String&);

    /**
	 \brief Present DateTime according to pattern. (deprecated use GetLocalizedDate
     or GetLocalizedTime)
	 \param[in] string pattern in strftime format.
     \returns localized string representation.
	 */
    DAVA_DEPRECATED(WideString AsWString(const wchar_t* format) const);

    /**
    \brief for current locale print date, platform dependent
    example:
    locale "ru_RU" -> "08.09.1984"
    locale "en_US" -> "9/8/1984
    */
    WideString GetLocalizedDate() const;

    /**
    \brief for current locale print time, platform dependent
    example:
    locale ru_RU -> "15:20:35"
    locale en_US -> "10:15:15 PM"
    */
    WideString GetLocalizedTime() const;

private:
    DateTime(Timestamp timeStamp, int32 timeZone);

    static void GmTimeThreadSafe(tm* resultGmTime, const time_t* unixTimestamp);

    static void LocalTimeThreadSafe(tm* resultLocalTime, const time_t* unixTimestamp);

    static int32 GetLocalTimeZoneOffset();

    Timestamp GetRawTimeStampForCurrentTZ() const;

    void UpdateLocalTimeStructure();

    inline bool IsLeap(int32 year) const;

    inline int32 DaysFrom1970(int32 year) const;

    inline int32 DaysFrom0(int32 year) const;

    inline int32 DaysFrom1jan(int32 year, int32 month, int32 day) const;

    Timestamp InternalTimeGm(tm* t) const;

    bool IsNumber(const String& s) const;

    tm localTime = tm();
    Timestamp innerTime = 0;
    int32 innerMilliseconds = 0; // Stored separately since time_t and tm do not support milliseconds. Can only be obtained from parsing ISO8601 date for now. Cannot be affected by time zone offset
    int32 timeZoneOffset = 0; // Offset in seconds
};

int32 DateTime::GetTimeZoneOffset() const
{
    return timeZoneOffset;
}

Timestamp DateTime::GetTimestamp() const
{
    return innerTime;
}

bool DateTime::IsLeap(int32 year) const
{
    DVASSERT(year >= 1970);
    if (year % 400 == 0)
        return true;
    if (year % 100 == 0)
        return false;
    if (year % 4 == 0) //-V112 // PVS warning disable: not an adderss
        return true;
    return false;
}

int32 DateTime::DaysFrom0(int32 year) const
{
    DVASSERT(year >= 1970);
    year--;
    return 365 * year + (year / 400) - (year / 100) + (year / 4); //-V112 // PVS warning disable: not an adderss
}

int32 DateTime::DaysFrom1970(int32 year) const
{
    DVASSERT(year >= 1970);
    static const int32 daysFrom0To1970 = DaysFrom0(1970);
    return DaysFrom0(year) - daysFrom0To1970;
}

int32 DateTime::DaysFrom1jan(int32 year, int32 month, int32 day) const
{
    DVASSERT(year >= 1970 && month >= 0 && month < 12 && day >= 1 && day <= 31);
    static const int32 days[2][12] =
    {
      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
      { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
    };
    int32 rowNumberToSelect = IsLeap(year) ? 1 : 0;
    return days[rowNumberToSelect][month] + day - 1;
}
};
