package com.dava.engine.notification;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import android.support.v4.app.NotificationCompat;

import com.dava.engine.DavaActivity;
import com.dava.engine.DavaLog;

public class ScheduledNotificationReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
    {
        Intent tapIntent;
        String activityClassName = intent.getStringExtra("activityClassName");
		if (null == activityClassName)
		{
			DavaLog.e(DavaActivity.LOG_TAG, "ScheduledNotificationReceiver.onReceive intent not contain activityClassName.");
            return;
		}
        try
        {
            Class<?> activityClass = Class.forName(activityClassName);
            tapIntent = new Intent(context, activityClass);
        }
        catch (ClassNotFoundException e)
        {
            DavaLog.e(DavaActivity.LOG_TAG, "ScheduledNotificationReceiver.onReceive activityClassName not found.");
            return;
        }

        Uri uri = null;
        if (intent.getBooleanExtra("useSound", false))
        {
            uri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
        }
        String uid = intent.getStringExtra("uid");
		int hash = 0;
		if (null != uid)
		{
			hash = uid.hashCode();
		}
        tapIntent.putExtra("uid", uid);
        PendingIntent pendingIntent = PendingIntent.getActivity(context, hash, tapIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        NotificationCompat.Builder builder = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new NotificationCompat.Builder(context, DavaNotificationProvider.channelId);
        }
        else {
            builder = new NotificationCompat.Builder(context);
        }

        builder.setContentTitle(intent.getStringExtra("title"))
               .setContentText(intent.getStringExtra("text"))
               .setSmallIcon(intent.getIntExtra("icon", 0))
			   .setContentIntent(pendingIntent)
               .setSound(uri);

		NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);     
		notificationManager.notify(uid, 0, builder.build());
    }
}
