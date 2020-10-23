package com.dava.engine;

import java.util.TimeZone;
import java.text.DateFormat;
import java.util.Date;
import java.util.Locale;

public class DateTime {
    final static String TAG = "JNIDateTime";

    public static String GetTimeAsString(final String format,final String countryCode, long timeStamp, int timeZoneOffset)
    {
        TimeZone tz = TimeZone.getTimeZone("UTC");
        tz.setRawOffset(timeZoneOffset*1000);
        String[] lang_country = countryCode.split("_");
        Locale loc = new Locale(lang_country[0], lang_country[1]);    
        Date dt = new Date();
        dt.setTime(timeStamp*1000);
        
        String result;        
        if (format.equals("%x"))
        {
            DateFormat dateFormat = DateFormat.getDateInstance(DateFormat.SHORT, loc);
            dateFormat.setTimeZone(tz);
            result = dateFormat.format(dt);
        } else if (format.equals("%X"))
        {
            DateFormat dateFormat = DateFormat.getTimeInstance(DateFormat.MEDIUM, loc);
            dateFormat.setTimeZone(tz);
            result = dateFormat.format(dt);
        } else
        {
            // this is old invalid implementation leave it only temporarily, see DAVA::DateTime.AsWString(String format)
            Strftime formatter = new Strftime(format, loc);
            formatter.setTimeZone(tz);
            result = formatter.format(dt);	
        }
        return result;
    }
    
    public static int GetLocalTimeZoneOffset()
    {
        TimeZone tz = TimeZone.getDefault();
        Date now = new Date();
        int offsetFromUtc = tz.getOffset(now.getTime()) / 1000;
        return offsetFromUtc;
    }
}
