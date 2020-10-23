package com.dava.engine.notification;

import com.dava.engine.DavaActivity;
import com.dava.engine.DavaLog;

import android.content.Intent;

public class LocalNotificationListener extends DavaActivity.ActivityListenerImpl
{
    protected long localNotificationController = 0;

    public static native void nativeNewIntent(String uid, long controller);

    public LocalNotificationListener(long controller)
    {
        localNotificationController = controller;
        DavaActivity.instance().registerActivityListener(this);
        DavaNotificationProvider.Init(DavaActivity.instance());
        DavaNotificationProvider.CancelAllNotifications();
        DavaLog.d(DavaActivity.LOG_TAG, "LocalNotificationListener.<init> Create class instance.");
    }

    void release()
    {
        DavaNotificationProvider.CancelAllNotifications();
        DavaActivity.instance().unregisterActivityListener(this);
    }

    @Override
    public void onDestroy()
    {
        release();
    }

    @Override
    public void onNewIntent(Intent intent)
    {
        if (null != intent)
        {
            String uid = intent.getStringExtra("uid");
            if (uid != null)
            {
                DavaNotificationProvider.HideNotification(uid);
                nativeNewIntent(uid, localNotificationController);
            }
        }
    }
}
