package com.dava.engine;

import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
    Logger class which allows for better extensibility compared to `android.util.Log`.
    It can be used in case some additional actions need to be performed for each log entry, for example sending it to Crashlytics, saving to file etc.
    
    Each log entry is processed in two steps:
        - First, it's passed to `java.util.logging.Logger` object
        - Then passed to according `android.util.Log` function (i.e. `DavaLog.e` calls `android.util.Log.e` method)

    A user can get a `java.util.logging.Logger` object, every log entry is passed to, via `DavaLog.getLogger` method and setup as required.
    As an example, a custom handler can be added to it, which will send every record with level bigger than INFO to Crashlytics:
    \code
    DavaLog.getLogger().addHandler(
                new Handler() {
                    @Override
                    public void publish(LogRecord record)
                    {
                        if (isLoggable(record) && record.getLevel().intValue() >= Level.INFO.intValue())
                        {
                            Crashlytics.log(record.getMessage());
                        }
                    }

                    ...
                });
    \endcode
*/
public class DavaLog
{
    private static Logger logger;

    static
    {
        logger = Logger.getLogger("com.dava.engine.DavaLog");
        logger.setLevel(Level.FINEST);
    }

    public static Logger getLogger()
    {
        return logger;
    }

    public static int e(String tag, String msg)
    {
        logger.log(Level.SEVERE, tag + ": " + msg);
        return android.util.Log.e(tag, msg);
    }

    public static int e(String tag, String msg, Throwable tr)
    {
        logger.log(Level.SEVERE, tag + ": " + msg, tr);
        return android.util.Log.e(tag, msg, tr);
    }

    public static int w(String tag, String msg)
    {
        logger.log(Level.WARNING, tag + ": " + msg);
        return android.util.Log.w(tag, msg);
    }

    public static int i(String tag, String msg)
    {
        logger.log(Level.INFO, tag + ": " + msg);
        return android.util.Log.i(tag, msg);
    }

    public static int d(String tag, String msg)
    {
        logger.log(Level.FINEST, tag + ": " + msg);
        return android.util.Log.d(tag, msg);
    }
}