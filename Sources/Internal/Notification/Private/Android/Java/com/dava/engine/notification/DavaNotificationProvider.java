package com.dava.engine.notification;

import android.app.AlarmManager;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.support.v4.app.NotificationCompat;

import com.dava.engine.DavaActivity;
import com.dava.engine.DavaLog;

import java.util.Calendar;

public class DavaNotificationProvider {
    private static NotificationCompat.Builder builder = null;
    private static NotificationManager notificationManager = null;
    private static AssetManager assetsManager = null;
    private static boolean isInited = false;
    private static int icon;

    private static Context context;
    private static DavaActivity activity;

    public static String channelId = "DavaNotificationChannel";

    static void Init(DavaActivity davaActivity)
    {
        DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.Init");
        activity = davaActivity;
        context = davaActivity.getApplication();
        
        assetsManager = context.getAssets();
        notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(channelId, channelId, NotificationManager.IMPORTANCE_DEFAULT);
            notificationManager.createNotificationChannel(channel);
            builder = new NotificationCompat.Builder(context, channelId);
        }
        else {
            builder = new NotificationCompat.Builder(context);
        }

        isInited = (null != notificationManager && null != assetsManager);
        if (!isInited)
        {
            DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider not inited!");
            return;
        }

        if (icon == 0)
        {
            icon = android.R.drawable.sym_def_app_icon;
        }

        builder.setSmallIcon(icon);
    }

    public static void SetNotificationIcon(int value)
    {
        icon = value;

        if (builder != null)
        {
            builder.setSmallIcon(icon);
        }
    }

    static int GetNotificationIcon()
    {
        return icon;
    }

    static void CleanBuilder()
    {
        builder.setContentTitle("").setContentText("").setProgress(0, 0, false);
    }

	// TODO: Remove this method, after transition on Core V2.
    static void EnableTapAction(String uid)
    {
    }
    
    static void NotifyProgress(String uid, String title, String text, int maxValue, int value, boolean useSound)
    {
        if (isInited)
        {
            CleanBuilder();
            
            Uri uri = null;
            if (useSound)
            {
                uri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);    
            }

            Intent intent = new Intent(activity, activity.getClass());
            intent.putExtra("uid", uid);
			int hash = uid.hashCode();
            PendingIntent pIntent = PendingIntent.getActivity(activity, hash, intent, PendingIntent.FLAG_UPDATE_CURRENT);

            
            builder.setContentTitle(title)
                   .setContentText(text)
                   .setProgress(maxValue, value, false)
                   .setSound(uri)
				   .setContentIntent(pIntent);
            
            notificationManager.notify(uid, 0, builder.build());
        }
    }
    
    static void NotifyText(String uid, String title, String text, boolean useSound)
    {
        DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.NotifyText");
        if (isInited)
        {
            CleanBuilder();
            
            Uri uri = null;
            if (useSound)
            {
                uri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);    
            }
            Intent intent = new Intent(activity, activity.getClass());
			int hash = 0;
			if (null != uid)
			{
				hash = uid.hashCode();
			}
            intent.putExtra("uid", uid);
            PendingIntent pIntent = PendingIntent.getActivity(activity, hash, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            builder.setContentTitle(title)
                    .setContentText(text)
                    .setSound(uri)
					.setContentIntent(pIntent);

            notificationManager.notify(uid, 0, builder.build());
        }
    }

    static void NotifyDelayed(String uid, String title, String text, int delaySeconds, boolean useSound)
    {
        try {
            DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.NotifyDelayed");
            AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
            Intent intent = new Intent(context, ScheduledNotificationReceiver.class);
            intent.putExtra("uid", uid);
            intent.putExtra("icon", icon);
            intent.putExtra("title", title);
            intent.putExtra("text", text);
            intent.putExtra("useSound", useSound);
            if(activity != null)
            {
                intent.putExtra("activityClassName", activity.getClass().getName());
            }
            PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
            alarmManager.set(AlarmManager.RTC_WAKEUP, Calendar.getInstance().getTimeInMillis() + delaySeconds * 1000, pendingIntent);
        } catch (Exception e) {
            DavaLog.e(DavaActivity.LOG_TAG, "DavaNotificationProvider.NotifyDelayed failed: " + e.toString());
        }
    }

    static void RemoveAllDelayedNotifications()
    {
        DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.RemoveAllDelayedNotifications");
        Intent intent = new Intent(context, ScheduledNotificationReceiver.class);

        // Flags FLAG_CANCEL_CURRENT does not actually cancels intent and alarms are accumulated in system which may
        // result in `java.lang.SecurityException: !@Too many alarms (500) registered from uid` on some devices,
        // especially on Samsungs with Android 5+.
        // People in StackOverflow suggest to use FLAG_UPDATE_CURRENT:
        // https://stackoverflow.com/questions/29344971/java-lang-securityexception-too-many-alarms-500-registered-from-pid-10790-u
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        alarmManager.cancel(pendingIntent);
    }

    static void HideNotification(String uid)
    {
        DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.HideNotification");
        if (isInited)
        {
            CleanBuilder();
            notificationManager.cancel(uid, 0);
        }
    }

    static void CancelAllNotifications()
    {
        DavaLog.d(DavaActivity.LOG_TAG, "DavaNotificationProvider.CancelAllNotifications");
        if (isInited) {
            notificationManager.cancelAll();
        }
    }
}
