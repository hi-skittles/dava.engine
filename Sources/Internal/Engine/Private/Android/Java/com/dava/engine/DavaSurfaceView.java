package com.dava.engine;

import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.InputDevice;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;
import android.util.DisplayMetrics;
import java.lang.reflect.Constructor;

/**
    \ingroup engine
    DavaSurfaceView provides surface where rendering takes place. This class is tightly coupled with C++ `WindowImpl` class.

    `DavaSurfaceView` does:
        - implements SurfaceHolder.Callback interface and notifies native code about surface state changing,
        - elementary input handling and forwards input to native code,
        - manages native controls: creation, position, removal.
*/
final class DavaSurfaceView extends SurfaceView
                            implements SurfaceHolder.Callback,
                                       DavaGlobalLayoutState.GlobalLayoutListener,
                                       View.OnTouchListener,
                                       View.OnGenericMotionListener,
                                       View.OnKeyListener
{
    private long windowImplPointer = 0;
    private boolean isSurfaceReady = false;
    
    public static native void nativeSurfaceViewOnResume(long windowImplPointer);
    public static native void nativeSurfaceViewOnPause(long windowImplPointer);
    public static native void nativeSurfaceViewOnSurfaceCreated(long windowImplPointer, DavaSurfaceView surfaceView);
    public static native void nativeSurfaceViewOnSurfaceChanged(long windowImplPointer, Surface surface, int width, int height, int surfaceWidth, int surfaceHeight, int dpi);
    public static native void nativeSurfaceViewOnSurfaceDestroyed(long windowImplPointer);
    public static native void nativeSurfaceViewProcessEvents(long windowImplPointer);
    public static native void nativeSurfaceViewOnMouseEvent(long windowImplPointer, int action, int buttonId, float x, float y, float deltaX, float deltaY, int modifierKeys);
    public static native void nativeSurfaceViewOnTouchEvent(long windowImplPointer, int action, int touchId, float x, float y, int modifierKeys);
    public static native void nativeSurfaceViewOnKeyEvent(long windowImplPointer, int action, int keyScancode, int keyVirtual, int unicodeChar, int modifierKeys, boolean isRepeated);
    public static native void nativeSurfaceViewOnGamepadButton(long windowImplPointer, int deviceId, int action, int keyCode);
    public static native void nativeSurfaceViewOnGamepadMotion(long windowImplPointer, int deviceId, int axis, float value);
    public static native void nativeSurfaceViewOnVisibleFrameChanged(long windowImplPointer, int x, int y, int w, int h);

    public DavaSurfaceView(Context context, long windowImplPtr)
    {
        super(context);
        getHolder().addCallback(this);
        getHolder().setFormat(PixelFormat.OPAQUE);
        
        windowImplPointer = windowImplPtr;

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnTouchListener(this);
        setOnKeyListener(this);
        setOnGenericMotionListener(this);
    }

    public boolean isSurfaceReady()
    {
        return isSurfaceReady;
    }
    
    public Object createNativeControl(String className, long backendPointer)
    {
        try {
            Class<?> clazz = Class.forName(className);
            Constructor<?> ctor = clazz.getConstructor(DavaSurfaceView.class, Long.TYPE);
            return ctor.newInstance(this, backendPointer);
        } catch (Throwable e) {
            DavaLog.e(DavaActivity.LOG_TAG, String.format("DavaSurfaceView.createNativeControl '%s' failed: %s", className, e.toString()));
            return null;
        }
    }

    public void addControl(View control)
    {
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                                            FrameLayout.LayoutParams.MATCH_PARENT,
                                            FrameLayout.LayoutParams.MATCH_PARENT);
        ((ViewGroup)getParent()).addView(control, params);
    }

    public void positionControl(View control, float x, float y, float w, float h)
    {
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams)control.getLayoutParams();
        params.leftMargin = (int)x;
        params.topMargin = (int)y;
        params.width = (int)w;
        params.height = (int)h;
        control.setLayoutParams(params);
    }

    public void removeControl(View control)
    {
        ((ViewGroup)getParent()).removeView(control);
    }

    public void triggerPlatformEvents()
    {
        DavaActivity.commandHandler().sendTriggerProcessEvents(this);
    }

    public void processEvents()
    {
        nativeSurfaceViewProcessEvents(windowImplPointer);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
        // To fix keyboard blinking when a DavaTextField looses focus but keyboard isn't hidden yet
        // (since we wait to close it in case some other field gets focused)
        outAttrs.imeOptions = DavaTextField.getLastSelectedImeMode();
        outAttrs.inputType = DavaTextField.getLastSelectedInputType();
        return super.onCreateInputConnection(outAttrs);
    }

    public void onResume()
    {
        DavaGlobalLayoutState gls = DavaActivity.instance().globalLayoutState;
        if (gls != null && !gls.hasGlobalLayoutListener(this))
        {
            gls.addGlobalLayoutListener(this);
        }

        setFocusableInTouchMode(true);
        setFocusable(true);
        requestFocus();
        setOnTouchListener(this);

        nativeSurfaceViewOnResume(windowImplPointer);
    }

    public void onPause()
    {
        DavaGlobalLayoutState gls = DavaActivity.instance().globalLayoutState;
        if (gls != null)
        {
            gls.removeGlobalLayoutListener(this);
        }

        nativeSurfaceViewOnPause(windowImplPointer);
    }

    public int getDpi()
    {
        final DisplayMetrics dm = new DisplayMetrics();
        DavaActivity.instance().getWindowManager().getDefaultDisplay().getMetrics(dm);

        // Use dm.densityDpi because it returns DPI that used by system for UI scaling.
        // Values of dm.(x|y)dpi don't return correct DPI on some devices.
        return dm.densityDpi; 
    }

    // SurfaceHolder.Callback interaface
    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        DavaLog.i(DavaActivity.LOG_TAG, "DavaSurface.surfaceCreated");
        nativeSurfaceViewOnSurfaceCreated(windowImplPointer, this);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
    {
        boolean skip = false;
        int requestedOrientation = DavaActivity.instance().getRequestedOrientation();
        if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED)
        {
            // accept any
        }
        else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT ||
                requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT ||
                requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT)
        {
            if (w > h)
               skip = true;
        } else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE ||
                requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE ||
                requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE)
        {
            if (w < h)
                skip = true;
        }
        
        if (skip)
        {
            DavaLog.i(DavaActivity.LOG_TAG, String.format("DavaSurface.surfaceChanged: skip w=%d, h=%d", w, h));
            return;
        }

        int dpi = getDpi();
        isSurfaceReady = true;

        DavaLog.i(DavaActivity.LOG_TAG, String.format("DavaSurface.surfaceChanged: w=%d, h=%d, surfW=%d, surfH=%d, dpi=%d", w, h, w, h, dpi));
        nativeSurfaceViewOnSurfaceChanged(windowImplPointer, holder.getSurface(), w, h, w, h, dpi);
        
        if (!DavaActivity.isNativeThreadRunning())
        {
            // continue initialization of game after creating main window
            DavaActivity.instance().onFinishCreatingMainWindowSurface();
        }
        DavaActivity.instance().handleResume();

        // Workaround: Duplicate register global layout listener here because method `onResume` don't
        // called in `DavaActivity.instance().handleResume()` in first launch.
        DavaGlobalLayoutState gls = DavaActivity.instance().globalLayoutState;
        if (gls != null && !gls.hasGlobalLayoutListener(this))
        {
            gls.addGlobalLayoutListener(this);

            // Reinitialize overlay window if surface changed after initializing.
            // This is workaround for "transparent" keyboard defect.
            gls.reinitOverlayWindow();
        }
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        DavaLog.i(DavaActivity.LOG_TAG, "DavaSurface.surfaceDestroyed");
        
        DavaActivity.instance().handlePause();
        if (isSurfaceReady)
        {
            isSurfaceReady = false;
            nativeSurfaceViewOnSurfaceDestroyed(windowImplPointer);
        }
    }

    @Override
    public void onVisibleFrameChanged(Rect visibleFrame)
    {
        nativeSurfaceViewOnVisibleFrameChanged(windowImplPointer, visibleFrame.left, visibleFrame.top, visibleFrame.width(), visibleFrame.height());
    }
    
    // View.OnTouchListener interface
    @Override
    public boolean onTouch(View v, MotionEvent event) 
    {
        int source = event.getSource();
        if (InputDevice.SOURCE_MOUSE == (source & InputDevice.SOURCE_MOUSE))
        {
            handleMouseEvent(event);
        }
        else if (InputDevice.SOURCE_TOUCHSCREEN == (source & InputDevice.SOURCE_TOUCHSCREEN))
        {
            handleTouchEvent(event);
        }
        return true;
    }

    // View.OnKeyListener interface
    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event)
    {
        if (!hasWindowFocus())
        {
            return false;
        }
        
        // Bixby has a dedicated button on some Samsung devices,
        // pressing it triggers a KeyEvent with code 1082.
        // There is no need to handle it
        final int KEYCODE_BIXBY = 1082;
        if (keyCode == KEYCODE_BIXBY)
        {
            return false;
        }

        int source = event.getSource();
        int action = event.getAction();

        // Some SOURCE_GAMEPAD and SOURCE_DPAD events are also SOURCE_KEYBOARD. So first check and process events
        // from gamepad and then from keyboard if not processed.
        if (InputDevice.SOURCE_GAMEPAD == (source & InputDevice.SOURCE_GAMEPAD)
                || InputDevice.SOURCE_DPAD == (source & InputDevice.SOURCE_DPAD))
        {
            nativeSurfaceViewOnGamepadButton(windowImplPointer, event.getDeviceId(), action, keyCode);
            return true;
        }

        if (InputDevice.SOURCE_KEYBOARD == (source & InputDevice.SOURCE_KEYBOARD))
        {
            int modifierKeys = event.getMetaState();
            int unicodeChar = event.getUnicodeChar();
            boolean isRepeated = event.getRepeatCount() > 0;
            int scanCode = event.getScanCode();
            nativeSurfaceViewOnKeyEvent(windowImplPointer, action, scanCode, keyCode, unicodeChar, modifierKeys, isRepeated);

            return true;
        }
        return false;
    }

    // View.OnGenericMotionListener interface
    @Override
    public boolean onGenericMotion(View v, MotionEvent event)
    {
        int source = event.getSource();
        if (InputDevice.SOURCE_GAMEPAD == (source & InputDevice.SOURCE_GAMEPAD)
                || InputDevice.SOURCE_DPAD == (source & InputDevice.SOURCE_DPAD)
                || InputDevice.SOURCE_JOYSTICK == (source & InputDevice.SOURCE_JOYSTICK))
        {
            handleGamepadMotionEvent(event);
            return true;
        }
        else if (InputDevice.SOURCE_MOUSE == (source & InputDevice.SOURCE_MOUSE))
        {
            handleMouseEvent(event);
            return true;
        }
        return false;
    }

    private void handleGamepadMotionEvent(MotionEvent event)
    {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE)
        {
            int deviceId = event.getDeviceId();
            // First check whether gamepad is in list of attached devices
            DavaGamepadManager.Gamepad gamepad = DavaActivity.gamepadManager().getGamepad(deviceId);
            if (gamepad != null)
            {
                int actionIndex = event.getActionIndex();
                for (int i = 0, n = gamepad.axes.size();i < n;++i)
                {
                    InputDevice.MotionRange r = gamepad.axes.get(i);
                    int axis = r.getAxis();
                    float value = event.getAxisValue(axis, actionIndex);
                    // Notify native code only if axis value has changed
                    // For now simply compare floats (alternatives are floatToIntBits or java.lang.Float.compare)
                    if (gamepad.axisValues[i] != value)
                    {
                        gamepad.axisValues[i] = value;
                        nativeSurfaceViewOnGamepadMotion(windowImplPointer, deviceId, axis, value);
                    }
                }
            }
        }
    }
    
    private int touchIdFromPointerId(int pointerId)
    {
    	// Pointer id can be equal to 0 which is used as 'no touch' in InputSystem
    	// So we should always generate different number
    	return pointerId + 1;
    }
    
    private void handleTouchEvent(MotionEvent event)
    {
        int action = event.getActionMasked();
        int pointerCount = event.getPointerCount();
        int modifierKeys = event.getMetaState();
        switch (action)
        {
        case MotionEvent.ACTION_MOVE:
            for (int i = 0;i < pointerCount;++i)
            {
                int pointerId = event.getPointerId(i);
                float x = event.getX(i);
                float y = event.getY(i);
                nativeSurfaceViewOnTouchEvent(windowImplPointer, MotionEvent.ACTION_MOVE, touchIdFromPointerId(pointerId), x, y, modifierKeys);
            }
            break;
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_DOWN:
        case MotionEvent.ACTION_POINTER_UP:
        case MotionEvent.ACTION_POINTER_DOWN:
        {
            int i = 0;
            if (action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_POINTER_DOWN)
            {
                i = event.getActionIndex();
            }
            int pointerId = event.getPointerId(i);
            float x = event.getX(i);
            float y = event.getY(i);
            nativeSurfaceViewOnTouchEvent(windowImplPointer, action, touchIdFromPointerId(pointerId), x, y, modifierKeys);
            break;
        }
        case MotionEvent.ACTION_CANCEL:
            for (int i = 0;i < pointerCount;++i)
            {
                int pointerId = event.getPointerId(i);
                float x = event.getX(i);
                float y = event.getY(i);
                nativeSurfaceViewOnTouchEvent(windowImplPointer, MotionEvent.ACTION_UP, touchIdFromPointerId(pointerId), x, y, modifierKeys);
            }
            break;
        default:
            break;
        }
    }

    private void handleMouseEvent(MotionEvent event)
    {
        int action = event.getActionMasked();
        int buttonState = event.getButtonState();
        float x = event.getX(0);
        float y = event.getY(0);
        float deltaX = event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0);
        float deltaY = event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0);
        int modifierKeys = event.getMetaState();
        nativeSurfaceViewOnMouseEvent(windowImplPointer, action, buttonState, x, y, deltaX, deltaY, modifierKeys);
    }
}
