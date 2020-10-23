package com.dava.engine;

import java.io.IOException;
import java.io.File;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Scanner;

import android.os.Build;
import android.view.Display;
import android.content.Context;
import android.util.DisplayMetrics;
import android.hardware.display.DisplayManager;

// Not thread-safe (should be used only from Android's main thread)
// Only handles displays for now
public final class DeviceManager
{
    // Displays

    public static final class DisplayInfo
    {
        public final String name;
        public final int id;
        public final int width;
        public final int height;
        public final float dpiX;
        public final float dpiY;
        public final float maxFps;
        
        public DisplayInfo(String name, int id, int width, int height, float dpiX, float dpiY, float maxFps)
        {
            this.name = name;
            this.id = id;
            this.width = width;
            this.height = height;
            this.dpiX = dpiX;
            this.dpiY = dpiY;
            this.maxFps = maxFps;
        }
    }

    // @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
    private class DisplayManagerListener implements DisplayManager.DisplayListener
    {
        @Override
        public void onDisplayRemoved(int removedDisplayId)
        {
            Iterator<DisplayInfo> iterator = DeviceManager.instance().displaysInfo.iterator();
            while (iterator.hasNext())
            {
                final DisplayInfo display = iterator.next();
                if (display.id == removedDisplayId)
                {
                    iterator.remove();
                }
            }
        }

        @Override
        public void onDisplayChanged(int displayId)
        {
        }

        @Override
        public void onDisplayAdded(int addedDisplayId)
        {
            // Assume default display can never be switched

            final Display newDisplay = DeviceManager.getDisplayManager().getDisplay(addedDisplayId);
            DeviceManager.instance().displaysInfo.add(getDisplayInfo(newDisplay));
        }
    }
    
    private static DeviceManager instance;
    public static DeviceManager instance()
    {
        if (instance == null)
        {
            instance = new DeviceManager();
        }
        
        return instance;
    }
    
    // List of currently connected displays
    // Use List since it's convenient to add & remove items from it during DisplayListener events
    private List<DisplayInfo> displaysInfo = new ArrayList<DisplayInfo>();
    
    private boolean isTrackingChanges = false;
    private DisplayManager.DisplayListener displayManagerListener = null;
    
    private DeviceManager()
    {
        updateDisplaysInfo();
    }
    
    public DisplayInfo[] getDisplaysInfo()
    {
        return displaysInfo.toArray(new DisplayInfo[displaysInfo.size()]);
    }
    
    // @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
    public void startTrackingChanges()
    {
        if (runningOnPostJellyBeanMR1() && !isTrackingChanges)
        {
            if (displayManagerListener == null)
            {
                displayManagerListener = new DisplayManagerListener();
            }

            getDisplayManager().registerDisplayListener(displayManagerListener, null);
            isTrackingChanges = true;
        }
    }
    
    // @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
    public void stopTrackingChanges()
    {
        if (isTrackingChanges)
        {
            getDisplayManager().unregisterDisplayListener(displayManagerListener);
            isTrackingChanges = false;
        }
    }
    
    private void updateDisplaysInfo()
    {
        displaysInfo.clear();
        
        if (runningOnPostJellyBeanMR1())
        {
            final Display[] displays = getDisplayManager().getDisplays();
            
            // Making sure default display is in the beginning
            for (int i = 0; i < displays.length; ++i)
            {
                final Display display = displays[i];
                
                if (display.getDisplayId() == Display.DEFAULT_DISPLAY)
                {
                    if (i > 0)
                    {
                        displays[i] = displays[0];
                        displays[0] = display;
                    }
                    
                    break;
                }
            }
            
            for (int i = 0; i < displays.length; ++i)
            {
                displaysInfo.add(getDisplayInfo(displays[i]));
            }
        }
        else
        {
            final DavaActivity activity = DavaActivity.instance();
            final Display primaryDisplay = activity.getWindowManager().getDefaultDisplay();
            displaysInfo.add(getDisplayInfo(primaryDisplay));
        }
    }
    
    // Multiple displays support and some Display APIs were introduced only in SDK 17
    // This method is used to avoid the same checks inside other methods
    private static boolean runningOnPostJellyBeanMR1()
    {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1);
    }
    
    // @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR1)
    private static DisplayManager getDisplayManager()
    {
        final DavaActivity activity = DavaActivity.instance();
        return (DisplayManager)activity.getApplicationContext().getSystemService(Context.DISPLAY_SERVICE);
    }
    
    private static DisplayInfo getDisplayInfo(Display display)
    {
        DisplayMetrics metrics = new DisplayMetrics();
        if (runningOnPostJellyBeanMR1())
        {
            display.getRealMetrics(metrics);
        }
        else
        {
            display.getMetrics(metrics);
        }


        int width = metrics.widthPixels;
        int height = metrics.heightPixels;
        float maxFps = display.getRefreshRate();
        
        final int id = display.getDisplayId();
        final String name = runningOnPostJellyBeanMR1() ? display.getName() : ("Unnamed-" + id);

        // metrics.xdpi & metrics.ydpi are filled by OEM and can be incorrect (for example on ZTE Nubia Z5S)
        // So we use densityDpi instead
        return new DisplayInfo(name, id, width, height, metrics.densityDpi, metrics.densityDpi, maxFps);
    }

    // CPU stats

    static boolean firstTimeCpuTempException = true;

    float getCpuTemperature()
    {
        // Even though Android provides API for getting temperature from two sensors (TYPE_TEMPERATURE and TYPE_AMBIENT_TEMPERATURE),
        // none of them guarantees that this temperature represents CPU temperature with some acceptable margin:
        // - TYPE_TEMPERATURE is deprecated since API 14 and is not widely supported
        // - TYPE_AMBIENT_TEMPERATURE gives ambient temperature which is not what we need
        //
        // The workaround is to read system files CPU temperature is written into.
        // The most common one seems to be /sys/class/thermal/thermal_zone0/temp
        // There are other files we can read this data from, but it seems to be sufficient to use only this one for now
        // TODO we can do it from C++ code

        float temperature = 0.0f;

        final String filepath = "/sys/class/thermal/thermal_zone0/temp";
        try
        {
            // Try to read int from the file

            Scanner scanner = new Scanner(new File(filepath));
            if (scanner.hasNextInt())
            {
                temperature = (float)scanner.nextInt();
            }
            else
            {
                if (firstTimeCpuTempException)
                {
                    DavaLog.e(DavaActivity.LOG_TAG, "Could not retrieve CPU temperature from file: " + filepath + ": file format is wrong");
                    firstTimeCpuTempException = false;
                }
            }
            scanner.close();
        }
        catch (IOException e)
        {
            if (firstTimeCpuTempException)
            {
                DavaLog.e(DavaActivity.LOG_TAG, "Could not retrieve CPU temperature from file: " + filepath + ", exception: " + e.getMessage());
                firstTimeCpuTempException = false;
            }
        }

        // Some vendors use thousands to represent one celsius degree
        // So we should divide value we read by 1000.0f to get a real value
        // Use 150.0f as top limit that should never be exceeded if value is not stored in thousands
        final float topLimitTemperature = 150.0f;
        if (temperature >= topLimitTemperature)
        {
            temperature /= 1000.0f;
        }

        return temperature;
    }
}