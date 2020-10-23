package com.dava.engine;

import android.graphics.Rect;
import java.util.LinkedList;
import java.util.List;

class DavaKeyboardState extends DavaActivity.ActivityListenerImpl
                        implements DavaGlobalLayoutState.GlobalLayoutListener
{
    public interface KeyboardStateListener
    {
        void onKeyboardOpened(Rect keyboardRect);
        void onKeyboardClosed();
    }

    private boolean isKeyboardOpen = false;
    private Rect keyboardRect = new Rect();
    private List<KeyboardStateListener> listeners = new LinkedList<KeyboardStateListener>();

    DavaKeyboardState()
    {
        onResume();
    }

    // DavaActivity.ActivityListener interface
    @Override
    public void onResume()
    {
        DavaActivity.instance().globalLayoutState.addGlobalLayoutListener(this);
    }

    @Override
    public void onPause()
    {
        DavaActivity.instance().globalLayoutState.removeGlobalLayoutListener(this);

        keyboardRect.setEmpty();
        isKeyboardOpen = false;

        emitKeyboardClosed();
    }

    public Rect keyboardRect()
    {
        return keyboardRect;
    }

    public boolean isKeyboardOpen()
    {
        return isKeyboardOpen;
    }

    public void addKeyboardStateListener(KeyboardStateListener l)
    {
        if (l != null)
        {
            listeners.add(l);
        }
    }

    public void removeKeyboardStateListener(KeyboardStateListener l)
    {
        listeners.remove(l);
    }

    @Override
    public void onVisibleFrameChanged(Rect visibleFrame)
    {
        int viewHeight = DavaActivity.instance().getWindow().getDecorView().getHeight();
        int heightThreshold = viewHeight / 4;  
        int dy = viewHeight - visibleFrame.height();

        if (dy > heightThreshold)
        {
            if (!isKeyboardOpen)
            {
                keyboardRect.left = visibleFrame.left;
                keyboardRect.top = visibleFrame.bottom;
                keyboardRect.right = visibleFrame.right;
                keyboardRect.bottom = visibleFrame.bottom + dy;

                isKeyboardOpen = true;
                emitKeyboardOpened();
            }
        }
        else
        {
            if (isKeyboardOpen)
            {
                emitKeyboardClosed();
                isKeyboardOpen = false;
            }
        }
    }

    private void emitKeyboardOpened()
    {
        for (KeyboardStateListener l : listeners)
        {
            l.onKeyboardOpened(keyboardRect);
        }
    }

    private void emitKeyboardClosed()
    {
        for (KeyboardStateListener l : listeners)
        {
            l.onKeyboardClosed();
        }
    }
}
