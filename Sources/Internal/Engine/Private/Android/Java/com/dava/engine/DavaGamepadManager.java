package com.dava.engine;

import android.view.InputDevice;
import android.view.MotionEvent;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;

/**
    Gamepad device manager.

    This class maintains list of available gamepad devices and periodicaly polls system for gamepad addition and removal.

    **Note.** For now only one connected gamepad is supported, other gamepads are ignored.

    **Note.** This class is not intended for direct use by clients. 

    \todo: In future make use of android InputManager (available in API 16) for all input devices.

    Source of inspiration is SDL library.
*/
final class DavaGamepadManager extends DavaActivity.ActivityListenerImpl
{
    // Gamepad device description 
    static class Gamepad
    {
        public int deviceId; // Gamepad device id as reported by system
        public String name; // Gamepad name as reported by system
        public ArrayList<InputDevice.MotionRange> axes; // Axes supported by gamepad, subset of supportedAxes[]
        public float[] axisValues; // Last axes' values to 
        boolean isAttached; // Internal flag used in updateGamepadDevices() to check whether gamepad has been removed from system
    }

    // List of gamepad axes supported by dava.engine for now
    final static int supportedAxes[] = {
        MotionEvent.AXIS_X,
        MotionEvent.AXIS_Y,
        MotionEvent.AXIS_Z,
        MotionEvent.AXIS_RX,
        MotionEvent.AXIS_RY,
        MotionEvent.AXIS_RZ,
        MotionEvent.AXIS_HAT_X,
        MotionEvent.AXIS_HAT_Y,
        MotionEvent.AXIS_LTRIGGER,
        MotionEvent.AXIS_RTRIGGER,
        MotionEvent.AXIS_BRAKE,
        MotionEvent.AXIS_GAS
    };

    // How often to update available gamepads, ms
    final static long UPDATE_PERIOD = 3000L;

    // List of gamepads currently attached to system
    private ArrayList<Gamepad> gamepads = new ArrayList<Gamepad>();

    public static native void nativeOnGamepadAdded(int deviceId, String name, boolean hasTriggerButtons);
    public static native void nativeOnGamepadRemoved(int deviceId);

    public DavaGamepadManager()
    {
        onResume();
    }

    // DavaActivity.ActivityListener interface
    @Override
    public void onResume()
    {
        updateGamepadDevices();
        DavaActivity.commandHandler().sendUpdateGamepads(this, UPDATE_PERIOD);
    }

    // Get gamepad by its device id or null if there is no such device 
    Gamepad getGamepad(int deviceId)
    {
        for (Gamepad g : gamepads)
        {
            if (g.deviceId == deviceId)
                return g;
        }
        return null;
    }

    // Invoked by DavaCommandHandler when time to update gamepads has come
    void timeToUpdate()
    {
        updateGamepadDevices();
        DavaActivity.commandHandler().sendUpdateGamepads(this, UPDATE_PERIOD);
    }

    // Update available gamepad device list.
    // This method queries available gamepad devices, checks which devices has been added or removed and
    // notifies native code about gamepad addition/removal.
    private void updateGamepadDevices()
    {
        int deviceIds[] = InputDevice.getDeviceIds();
        for (int id : deviceIds)
        {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null)
            {
                continue;
            }
            
            int sources = device.getSources();
            // Select only joystick devices
            // TODO: maybe I should also check and SOURCE_CLASS_BUTTON devices?
            if ((sources & InputDevice.SOURCE_CLASS_JOYSTICK) == InputDevice.SOURCE_CLASS_JOYSTICK)
            {
                Gamepad gamepad = getGamepad(id);
                if (gamepad == null)
                {
                    // New gamepad device has appeared in system so gather its description and add to internal list 
                    gamepad = new Gamepad();
                    gamepad.deviceId = id;
                    gamepad.name = device.getName();
                    gamepad.axes = new ArrayList<InputDevice.MotionRange>();

                    boolean hasTriggerButtons = false;
                    List<InputDevice.MotionRange> ranges = device.getMotionRanges();
                    for (InputDevice.MotionRange r : ranges)
                    {
                        int rangeSource = r.getSource();
                        if ((rangeSource & InputDevice.SOURCE_CLASS_JOYSTICK) == InputDevice.SOURCE_CLASS_JOYSTICK)
                        {
                            int axis = r.getAxis();
                            if (isAxisSupported(axis))
                            {
                                gamepad.axes.add(r);
                                hasTriggerButtons |= (axis == MotionEvent.AXIS_LTRIGGER || axis == MotionEvent.AXIS_BRAKE); 
                            }
                        }
                    }

                    // Sort axes by its numeric value
                    Collections.sort(gamepad.axes, new Comparator<InputDevice.MotionRange>() {
                        @Override public int compare(InputDevice.MotionRange l, InputDevice.MotionRange r)
                        {
                            return l.getAxis() - r.getAxis();
                        }
                    });

                    // Add gamepad to internal list and notify native code about gamepad addition
                    if (!gamepad.axes.isEmpty())
                    {
                        gamepad.axisValues = new float[gamepad.axes.size()];
                        gamepads.add(gamepad);

                        nativeOnGamepadAdded(gamepad.deviceId, gamepad.name, hasTriggerButtons);
                    }
                }

                // Mark gamepad as attached to system, devices that are not attached are considered removed
                gamepad.isAttached = true;
            }
        }

        // Check removed gamepads and notify native code
        for (Iterator<Gamepad> it = gamepads.iterator();it.hasNext();)
        {
            Gamepad g = it.next();
            if (!g.isAttached)
            {
                nativeOnGamepadRemoved(g.deviceId);
                it.remove();
            }
            g.isAttached = false;
        }
    }

    private static boolean isAxisSupported(int axis)
    {
        for (int i : supportedAxes)
        {
            if (i == axis)
                return true;
        }
        return false;
    }
}
