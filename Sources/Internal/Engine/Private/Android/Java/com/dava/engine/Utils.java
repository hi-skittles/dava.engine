package com.dava.engine;

import java.util.UUID;

import android.net.Uri;
import android.view.WindowManager;
import android.content.Intent;
import android.content.ActivityNotFoundException;

public class Utils
{
    private Utils()
    {
        
    }
    
    public static void disableSleepTimer()
    {
        final DavaActivity activity = DavaActivity.instance();
        activity.runOnUiThread(new Runnable()
        {
            @Override
            public void run() 
            {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });
    }
    
    public static void enableSleepTimer()
    {
        final DavaActivity activity = DavaActivity.instance();
        activity.runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });
    }

    public static void openURL(final String url)
    {
        final DavaActivity activity = DavaActivity.instance();
        activity.runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                try
                {
                    if (activity.isPaused())
                    {
                        return;
                    }

                    DavaLog.i(DavaActivity.LOG_TAG, "[OpenURL] " + url);

                    Intent exWeb = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                    activity.startActivity(exWeb);
                }
                catch(ActivityNotFoundException e)
                {
                    DavaLog.e(DavaActivity.LOG_TAG, "[OpenURL] failed with exeption: " + e.toString());
                }
            }
        });
    }

    public static String generateGUID()
    {
        return UUID.randomUUID().toString();
    }
}