package com.dava.engine;

import android.Manifest;

import android.content.pm.PackageManager;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;

import static android.content.Context.TELEPHONY_SERVICE;

class PhoneStateListener extends android.telephony.PhoneStateListener
{
    private String currentCarrierName = GetCarrierName();
    private int currentSignalStrength = 0;

    private native void OnCarrierNameChanged();

    public PhoneStateListener()
    {
        DavaActivity activity = DavaActivity.instance();

        String name = activity.getPackageName();
        int result = activity.getPackageManager().checkPermission(Manifest.permission.READ_PHONE_STATE, activity.getPackageName());

        TelephonyManager tm = (TelephonyManager)DavaActivity.instance().getSystemService(TELEPHONY_SERVICE);
        if ((tm != null) && activity.getPackageManager().checkPermission(Manifest.permission.READ_PHONE_STATE, activity.getPackageName()) != PackageManager.PERMISSION_DENIED)
        {
            tm.listen(this, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS | PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
        }
    }

    public String GetCarrierName()
    {
        String carrierName = "unknown";

        TelephonyManager tm = (TelephonyManager)DavaActivity.instance().getSystemService(TELEPHONY_SERVICE);
        if (tm != null && tm.getSimOperatorName() != null)
        {
            carrierName = tm.getSimOperatorName();
        }

        return carrierName;
    }

    public int GetSignalStrength()
    {
        return currentSignalStrength;
    }

    @Override
    public void onDataConnectionStateChanged(int state, int networkType)
    {
        String newCarrierName = GetCarrierName();
        if (!newCarrierName.equals(currentCarrierName))
        {
            currentCarrierName = newCarrierName;
            OnCarrierNameChanged();
        }

        super.onDataConnectionStateChanged(state, networkType);
    }

    @Override
    public void onSignalStrengthsChanged(SignalStrength signalStrength)
    {
        currentSignalStrength = signalStrength.getGsmSignalStrength();

        super.onSignalStrengthsChanged(signalStrength);
    }
}
