package com.dava.engine;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.StringTokenizer;
import java.util.TimeZone;

import android.content.Context;
import android.graphics.Point;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings.Secure;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.Display;

public class DeviceInfo {
    private final static String TAG = "JNIDeviceInfo";

    private static PhoneStateListener phoneStateListener;

    public static int zBufferSize = 0;
    public static byte gpuFamily = DavaSplashView.GPU_INVALID;

    static void initialize()
    {
        phoneStateListener = new PhoneStateListener();
    }

    @SuppressWarnings("unused")
    public static String GetVersion()
    {
        return Build.VERSION.RELEASE;
    }

    @SuppressWarnings("unused")
    public static String GetManufacturer()
    {
        return Build.MANUFACTURER;
    }

    @SuppressWarnings("unused")
    public static String GetModel()
    {
        return Build.MODEL;
    }

    @SuppressWarnings("unused")
    public static String GetLocale()
    {
        return Locale.getDefault().getDisplayLanguage(Locale.US);
    }

    @SuppressWarnings("unused")
    public static String GetRegion()
    {
        return DavaActivity.instance().getResources().getConfiguration().locale.getCountry();
    }

    @SuppressWarnings("unused")
    public static String GetTimeZone()
    {
        return TimeZone.getDefault().getID();
    }

    @NonNull
    @SuppressWarnings("unused")
    public static String GetUDID()
    {
        String aid;
        aid = Secure.getString(DavaActivity.instance().getApplicationContext().getContentResolver(), Secure.ANDROID_ID);
        Object obj;
        try {
            ((MessageDigest) (obj = MessageDigest.getInstance("MD5"))).update(aid.getBytes(), 0, aid.length());

            obj = String.format("%040X", new Object[] { new BigInteger(1, ((MessageDigest) obj).digest()) });
        } 
        catch (NoSuchAlgorithmException localNoSuchAlgorithmException) {
            obj = aid.substring(0, 32);
        }

        return obj.toString().toLowerCase();
    }

    @SuppressWarnings("unused")
    public static String GetName()
    {
        String serial = android.os.Build.SERIAL;
        if (serial == null || serial.isEmpty())
            serial = "ErrorGetSerialNumber";
        return serial;
    }

    @SuppressWarnings("unused")
    public static int GetZBufferSize()
    {
        if (DavaActivity.instance() != null)
        {
            return zBufferSize;
        } else
        {
            return GLConfigChooser.GetDepthBufferSize();
        }
    }

    @SuppressWarnings("unused")
    public static byte GetGpuFamily()
    {
        return gpuFamily;
    }

    @SuppressWarnings("unused")
    public static String GetHTTPProxyHost()
    {
        return System.getProperty("http.proxyHost");
    }

    @SuppressWarnings("unused")
    public static int GetHTTPProxyPort()
    {
        String portStr = System.getProperty("http.proxyPort");
        return Integer.parseInt((portStr != null ? portStr : "0"));
    }

    @SuppressWarnings("unused")
    public static String GetHTTPNonProxyHosts()
    {
        return System.getProperty("http.nonProxyHosts");
    }
    
    private static Point GetDefaultDisplaySize()
    {
        Display display;
        display = DavaActivity.instance().getWindowManager().getDefaultDisplay();
    	Point size = new Point();
    	if (Build.VERSION.SDK_INT >= 17)
        {
            display.getRealSize(size);
        } else
        {
            display.getSize(size);
        }
        return size;
    }

    @SuppressWarnings("unused")
    public static int GetDefaultDisplayWidth()
    {
    	Point size = GetDefaultDisplaySize();
    	return size.x;
    }

    @SuppressWarnings("unused")
    public static int GetDefaultDisplayHeight()
    {
    	Point size = GetDefaultDisplaySize();
    	return size.y;
    }
        
    private static final int NETWORK_TYPE_NOT_CONNECTED = 0;
    private static final int NETWORK_TYPE_UNKNOWN = 1;
    private static final int NETWORK_TYPE_MOBILE = 2;
    private static final int NETWORK_TYPE_WIFI = 3;
    private static final int NETWORK_TYPE_WIMAX = 4;
    private static final int NETWORK_TYPE_ETHERNET = 5;
    private static final int NETWORK_TYPE_BLUETOOTH = 6;

    @SuppressWarnings("unused")
    public static int GetNetworkType() {
        NetworkInfo info = null;
        ConnectivityManager cm = (ConnectivityManager)DavaActivity.instance().getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm != null)
        {
            info = cm.getActiveNetworkInfo();
        }

        if (info == null || !info.isConnected())
            return NETWORK_TYPE_NOT_CONNECTED;
        
        int netType = info.getType();
        switch (netType) {
        case ConnectivityManager.TYPE_MOBILE:
            return NETWORK_TYPE_MOBILE;
        case ConnectivityManager.TYPE_WIFI:
            return NETWORK_TYPE_WIFI;
        case ConnectivityManager.TYPE_WIMAX:
            return NETWORK_TYPE_WIMAX;
        case ConnectivityManager.TYPE_ETHERNET:
            return NETWORK_TYPE_ETHERNET;
        case ConnectivityManager.TYPE_BLUETOOTH:
            return NETWORK_TYPE_BLUETOOTH;
        }
        return NETWORK_TYPE_UNKNOWN;
    }
    
    private static final int MaxSignalLevel = 100;

    @SuppressWarnings("unused")
    public static int GetSignalStrength(int networkType) {
        switch (networkType) {
        case NETWORK_TYPE_WIFI: {
            WifiManager wifiManager = (WifiManager)DavaActivity.instance().getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wifiManager != null)
            {
                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                return WifiManager.calculateSignalLevel(wifiInfo.getRssi(), MaxSignalLevel);
            }
            return NETWORK_TYPE_NOT_CONNECTED;
        }
        case NETWORK_TYPE_MOBILE: {
            //Get the GSM Signal Strength, valid values are (0-31, 99) as defined in TS 27.007 8.5
            int sign = phoneStateListener.GetSignalStrength();
            if (sign == 99)
                return -1;
            return (int)(MaxSignalLevel * sign / 31.f);
        }
        case NETWORK_TYPE_ETHERNET:
            return MaxSignalLevel;
        }
        return 0;
    }

    public static class StorageInfo
    {
        public final String path;

        final boolean readOnly;
        final boolean removable;
        final boolean emulated;

        final long capacity;
        final long freeSpace;

        StorageInfo(String path, boolean readOnly, boolean removable, boolean emulated, long capacity, long freeSpace)
        {
            this.path = path;
            this.readOnly = readOnly;
            this.removable = removable;
            this.emulated = emulated;
            this.capacity = capacity;
            this.freeSpace = freeSpace;
        }
    }
    
    static class StorageCapacity
    {
        long capacity = 0;
        long free = 0;
    }

    @Nullable
    static StorageCapacity getCapacityAndFreeSpace(String path)
    {
        File f = new File(path);
        if (f.exists())
        {
            StorageCapacity s = new StorageCapacity();
            s.capacity = f.getTotalSpace();
            s.free = f.getFreeSpace();
            return s;
        }
        return null;
    }

    @NonNull
    @SuppressWarnings("unused")
    public static StorageInfo GetInternalStorageInfo()
    {
        String path = Environment.getDataDirectory().getPath();
        StorageCapacity st = getCapacityAndFreeSpace(path);
        if (st == null)
        {
            Log.e(TAG, "can't determine internal storage free space and capacity");
            st = new StorageCapacity();
            st.free = 0;
            st.capacity = 0;
        }
        return new StorageInfo(path + "/", false, false, false, st.capacity, st.free);
    }

    @SuppressWarnings("WeakerAccess")
    public static boolean IsPrimaryExternalStoragePresent()
    {
        if (Build.VERSION.SDK_INT >= 19) {
            Context ctx = DavaActivity.instance().getApplicationContext();
            File[] files = ctx.getExternalFilesDirs(null);
            // acording to docs: https://developer.android.com/reference/android/os/Environment.html#isExternalStorageRemovable(java.io.File)
            // only first element of array is "primary external"
            if (files.length > 0) {
                File f = files[0];
                if (f != null) {
                    String dir = f.getPath();
                    StorageCapacity capacity = getCapacityAndFreeSpace(dir);
                    return capacity != null;
                }
            }
            return false;
        } else
        {
            String state = Environment.getExternalStorageState();
            return state.equals(Environment.MEDIA_MOUNTED) || state.equals(Environment.MEDIA_MOUNTED_READ_ONLY);
        }
    }

    @NonNull
    @SuppressWarnings({"unused", "WeakerAccess"})
    public static StorageInfo GetPrimaryExternalStorageInfo()
    {
        if (IsPrimaryExternalStoragePresent())
        {
            Context ctx = DavaActivity.instance().getApplicationContext();
            // start from KitKat(4.4) you can write to this external path
            if (Build.VERSION.SDK_INT >= 19) {
                File[] files = ctx.getExternalFilesDirs(null);
                if (files.length > 0)
                {
                    File f = files[0];
                    if (f != null)
                    {
                        ExternalPathState extPathState = new ExternalPathState(f, true);
                        if (extPathState.validated)
                        {
                            boolean readonly = extPathState.readonly;
                            boolean removable = extPathState.removable;
                            boolean emulated = extPathState.emulated;
                            StorageCapacity capacity = extPathState.storageCapacity;

                            return new StorageInfo(extPathState.absolutePath + "/", readonly,
                                    removable, emulated, capacity.capacity, capacity.free);
                        }
                    }
                }
            } else
            {
                File f = ctx.getExternalFilesDir(null);
                if (f != null)
                {
                    String path = f.getPath();
                    StorageCapacity st = getCapacityAndFreeSpace(path);
                    if (st != null)
                    {
                        boolean isRemovable = Environment.isExternalStorageRemovable();
                        boolean isEmulated = Environment.isExternalStorageEmulated();
                        String state = Environment.getExternalStorageState();
                        if (state.equals(Environment.MEDIA_MOUNTED_READ_ONLY) ||
                                state.equals(Environment.MEDIA_MOUNTED))
                        {
                            boolean isReadOnly = state.equals(Environment.MEDIA_MOUNTED_READ_ONLY);
                            return new StorageInfo(path + "/", isReadOnly, isRemovable, isEmulated, st.capacity, st.free);
                        }
                    }
                }
            }
        }

        return new StorageInfo("", false, false, false, 0, 0);
    }

    @SuppressWarnings("unused")
    public static StorageInfo[] GetSecondaryExternalStoragesList()
    {
        List<StorageInfo> infos = new ArrayList<>();

        HashSet<String> paths = new HashSet<>();

        StorageInfo primaryStorageInfo = GetPrimaryExternalStorageInfo();

        paths.add(primaryStorageInfo.path);

        if (Build.VERSION.SDK_INT >= 19) // KitKat 4.4
        {
            collectExternalFileDirs(infos, paths);
        } else
        {
            collectAllExternalFileDirsLegacyCode(infos, paths);
        }

        StorageInfo[] arr = new StorageInfo[infos.size()];
        infos.toArray(arr);
        return arr;
    }

    private static void collectExternalFileDirs(List<StorageInfo> infos, HashSet<String> paths) {
        if (Build.VERSION.SDK_INT >= 19) // KitKat 4.4
        {
            Context ctx = DavaActivity.instance().getApplicationContext();
            File[] files = ctx.getExternalFilesDirs(null);
            for (File f : files)
            {
                if (f != null && f != files[0]) // skip first element (primary element)
                {
                    ExternalPathState extPathState = new ExternalPathState(f, false);
                    if (extPathState.validated)
                    {
                        boolean readonly = extPathState.readonly;
                        boolean removable = extPathState.removable;
                        boolean emulated = extPathState.emulated;
                        StorageCapacity sc = extPathState.storageCapacity;

                        String path = extPathState.absolutePath + "/";
                        if(paths.contains(path))
                        {
                            continue;
                        }

                        paths.add(path);
                        infos.add(new StorageInfo(path, readonly,
                                removable, emulated, sc.capacity, sc.free));
                    }
                }
            }
        }
    }

    private static void collectAllExternalFileDirsLegacyCode(List<StorageInfo> infos, HashSet<String> paths) {
        BufferedReader reader = null;
        try
        {
            reader = new BufferedReader(new FileReader("/proc/mounts"));

            String line;
            while ((line = reader.readLine()) != null)
            {
                if (!line.contains("/mnt/secure")
                        && !line.contains("/mnt/asec")
                        && !line.contains("/mnt/obb")
                        && !line.contains("/dev/mapper")
                        && !line.contains("emulated")
                        && !line.contains("tmpfs"))
                {
                    StringTokenizer tokens = new StringTokenizer(line, " ");
                    tokens.nextToken(); //device
                    String mountPoint = tokens.nextToken();

                    if (paths.contains(mountPoint))
                    {
                        continue;
                    }

                    String fileSystem = tokens.nextToken();

                    List<String> flags = Arrays.asList(tokens.nextToken().split(",")); //flags
                    boolean readonly = flags.contains("ro");

                    if (fileSystem.equals("vfat") || mountPoint.startsWith("/mnt") || mountPoint.startsWith("/storage"))
                    {
                        StorageCapacity sc = getCapacityAndFreeSpace(mountPoint);
                        if (sc == null)
                        {
                            continue;
                        }

                        paths.add(mountPoint);
                        infos.add(new StorageInfo(mountPoint+"/", readonly, true, false, sc.capacity, sc.free));
                    }
                }
            }
        }
        catch (FileNotFoundException e)
        {
            Log.e(TAG, e.getMessage());
        }
        catch (IOException e)
        {
            Log.e(TAG, e.getMessage());
        }
        finally
        {
            if (reader != null)
            {
                try
                {
                    reader.close();
                }
                catch (IOException e)
                {
                    Log.e(TAG, e.getMessage());
                }
            }
        }
    }

    @SuppressWarnings("unused")
    public static String GetCarrierName()
    {
        return phoneStateListener.GetCarrierName();
    }

    private static class ExternalPathState {
        boolean validated;
        boolean readonly;
        boolean removable;
        boolean emulated;
        StorageCapacity storageCapacity;
        String absolutePath;

        ExternalPathState(File f, boolean isPrimary) {
            absolutePath = f.getAbsolutePath();
            storageCapacity = getCapacityAndFreeSpace(absolutePath);
            if (storageCapacity != null)
            {
                readonly = false;

                String state;
                if (Build.VERSION.SDK_INT >= 21)
                {
                    emulated = Environment.isExternalStorageEmulated(f);
                    removable = Environment.isExternalStorageRemovable(f);
                    state = Environment.getExternalStorageState(f);
                } else
                {
                    if (isPrimary)
                    {
                        emulated = Environment.isExternalStorageEmulated();
                        removable = Environment.isExternalStorageRemovable();
                    } else
                    {
                        emulated = false;
                        removable = true;
                    }

                    if (Build.VERSION.SDK_INT >= 19)
                    {
                        state = Environment.getStorageState(f);
                    } else
                    {
                        state = Environment.getExternalStorageState();
                    }

                }

                if (state.equals(Environment.MEDIA_NOFS) ||
                        state.equals(Environment.MEDIA_BAD_REMOVAL) ||
                        state.equals(Environment.MEDIA_REMOVED) ||
                        state.equals(Environment.MEDIA_CHECKING) ||
                        state.equals(Environment.MEDIA_EJECTING) ||
                        state.equals(Environment.MEDIA_UNMOUNTED) ||
                        state.equals(Environment.MEDIA_UNKNOWN))
                {
                    validated = false;
                    return;
                }

                if (state.equals(Environment.MEDIA_MOUNTED_READ_ONLY))
                {
                    readonly = true;
                }
                validated = true;
            } else
            {
                validated = false;
            }
        }
    }
}
