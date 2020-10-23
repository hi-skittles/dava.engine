package com.dava.engine;

import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.Build;
import android.view.Gravity;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.FrameLayout;

import java.util.LinkedList;
import java.util.List;

public class DavaGlobalLayoutState extends DavaActivity.ActivityListenerImpl implements ViewTreeObserver.OnGlobalLayoutListener {

    public interface GlobalLayoutListener
    {
        void onVisibleFrameChanged(Rect visibleFrame);
    }

    private View contentView = null;
    private FrameLayout layout = null;
    private Rect visibleFrame = new Rect();
    private List<GlobalLayoutListener> listeners = new LinkedList<GlobalLayoutListener>();

    public void initOverlayWindow()
    {
        if (layout == null)
        {
            contentView = DavaActivity.instance().getWindow().getDecorView();
            WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                    0,
                    WindowManager.LayoutParams.MATCH_PARENT,
                    WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
                    WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                            | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                            | WindowManager.LayoutParams.FLAG_FULLSCREEN
                            | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                    PixelFormat.TRANSPARENT);
            params.softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
            params.packageName = DavaActivity.instance().getApplication().getPackageName();
            params.gravity = Gravity.LEFT | Gravity.TOP;
            params.token = contentView.getWindowToken();
            params.alpha = 1.0f;

            layout = new FrameLayout(contentView.getContext());
            DavaActivity.instance().getWindowManager().addView(layout, params);

            layout.getViewTreeObserver().addOnGlobalLayoutListener(this);
        }
    }

    public void releaseOverlayWindow()
    {
        if (layout != null)
        {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
            {
                layout.getViewTreeObserver().removeGlobalOnLayoutListener(this);
            }
            else
            {
                layout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }

            DavaActivity.instance().getWindowManager().removeView(layout);

            visibleFrame.setEmpty();
            layout = null;
            contentView = null;
        }
    }

    public void reinitOverlayWindow()
    {
        if (layout != null)
        {
            releaseOverlayWindow();
            initOverlayWindow();
        }
    }

    @Override
    public void onPause()
    {
        // On some devices onPause was called earlier than focus lost that produced some graphical
        // defects while displaying other overlay windows (Facebook, G+ Login, etc.)
        releaseOverlayWindow();
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus) {
        if (hasWindowFocus)
        {
            initOverlayWindow();
        }
        else
        {
            releaseOverlayWindow();
        }
    }

    public void addGlobalLayoutListener(GlobalLayoutListener l)
    {
        if (l != null)
        {
            listeners.add(l);

            if (!visibleFrame.isEmpty())
            {
                l.onVisibleFrameChanged(visibleFrame);
            }
        }
    }

    public void removeGlobalLayoutListener(GlobalLayoutListener l)
    {
        if (l != null)
        {
            listeners.remove(l);
        }
    }

    public boolean hasGlobalLayoutListener(GlobalLayoutListener l)
    {
        return l != null && listeners.contains(l);
    }

    @Override
    public void onGlobalLayout()
    {
        if (layout != null)
        {
            layout.getWindowVisibleDisplayFrame(visibleFrame);

            // Convert window visible frame to content view frame size.
            // On some devices dimensions of content view and overlay window (layout) are
            // different.
            int layoutHeight = layout.getHeight();
            int contentViewHeight = contentView.getHeight();
            if (layoutHeight != contentViewHeight)
            {
                double scaleFactor = (double) contentViewHeight / (double) layoutHeight;
                int top = (int) Math.floor(visibleFrame.top * scaleFactor);
                int left = (int) Math.floor(visibleFrame.left * scaleFactor);
                int bottom = (int) Math.ceil(visibleFrame.bottom * scaleFactor);
                int right = (int) Math.ceil(visibleFrame.right * scaleFactor);
                visibleFrame.set(left, top, right, bottom);
            }

            emitVisibleFrameChanged();
        }
    }

    private void emitVisibleFrameChanged()
    {
        for (GlobalLayoutListener l : listeners)
        {
            l.onVisibleFrameChanged(visibleFrame);
        }
    }

}
