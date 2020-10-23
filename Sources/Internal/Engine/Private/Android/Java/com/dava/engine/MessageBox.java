package com.dava.engine;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.DialogInterface;
import android.os.Bundle;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

public class MessageBox extends DialogFragment
                 implements DialogInterface.OnClickListener,
                            DialogInterface.OnDismissListener
{
    // Semaphore to restrict only one MessageBox shown at a time, except case when MessageBox is shown in UI thread
    private static Semaphore semaphore = new Semaphore(1);

    // Synchronization object used to block thread that issued MessageBox and to notify that MessageBox has been dismissed
    private Object syncObject = new Object();
    private boolean originatedFromMainThread = false;
    private int selection = -1;

    public static int messageBox(String title, String content, String[] buttons)
    {
        if (!DavaActivity.instance().isFinishing() && !DavaActivity.instance().isPaused())
        {
            final boolean inUIThread = DavaActivity.isUIThread();
            if (!inUIThread)
            {
                // Only one MessageBox is allowed to show at a time, so wait until other MessageBox will complete blocking calling thread.
                // If application shows MessageBox from UI thread then do not block thread and simply show modal dialog.
                try {
                    semaphore.acquire();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                    return -1;
                }
            }

            MessageBox msgbox = new MessageBox();
            msgbox.prepare(title, content, buttons);
            return inUIThread ? msgbox.showModalNonBlocking() : msgbox.showModalBlocking();
        }
        return -1;
    }

    private void prepare(String title, String content, String[] buttons)
    {
        originatedFromMainThread = DavaActivity.isNativeMainThread();

        Bundle args = new Bundle();
        args.putString("title", title);
        args.putString("content", content);
        args.putStringArrayList("buttons", new ArrayList<String>(Arrays.asList(buttons)));
        setArguments(args);
    }

    private int showModalBlocking()
    {
        DavaActivity.instance().runOnUiThread(new Runnable() {
            @Override public void run() {
                show(DavaActivity.instance().getFragmentManager(), "msgbox");
            }
        });

        synchronized(syncObject)
        {
            // Wait until user dismisses current MessageBox
            try {
                syncObject.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
                return 0;
            }
        }
        semaphore.release();
        return selection;
    }

    private int showModalNonBlocking()
    {
        show(DavaActivity.instance().getFragmentManager(), "msgbox");
        return -1;
    }

    // interface DialogInterface.OnClickListener
    @Override
    public void onClick(DialogInterface dialog, int which)
    {
        switch(which)
        {
        case DialogInterface.BUTTON_POSITIVE:
            selection = 0;
            break;
        case DialogInterface.BUTTON_NEUTRAL:
            selection = 1;
            break;
        case DialogInterface.BUTTON_NEGATIVE:
            selection = 2;
            break;
        default:
            selection = -1;
            break;
        }
    }

    // interface DialogInterface.OnDismissListener
    @Override
    public void onDismiss(DialogInterface dialog)
    {
        removeFromFragmentManager();
        synchronized(syncObject)
        {
            syncObject.notify();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        String title = getArguments().getString("title");
        String content = getArguments().getString("content");
        ArrayList<String> buttons = getArguments().getStringArrayList("buttons");

        AlertDialog.Builder builder = new AlertDialog.Builder(DavaActivity.instance());
        builder.setTitle(title);
        builder.setMessage(content);
        builder.setCancelable(false);
        int i = 0;
        for (String s : buttons)
        {
            switch (i)
            {
            case 0:
                builder.setPositiveButton(s, this);
                break;
            case 1:
                builder.setNeutralButton(s, this);
                break;
            case 2:
                builder.setNegativeButton(s, this);
                break;
            }
            i += 1;
        }

        Dialog dlg = builder.create();
        dlg.setCanceledOnTouchOutside(false);
        return dlg;
    }

    @Override
    public void onDestroyView()
    {
        super.onDestroyView();
        removeFromFragmentManager();
    }

    @Override public void onPause()
    {
        super.onPause();

        // Explicitly dismiss MessageBox if it has been shown from main C++ thread when application went to background
        // to avoid deadlock as Android implementation makes blocking call from UI thread to native thread but
        // native thread is blocked waiting MessageBox completion.
        if (originatedFromMainThread)
        {
            DavaLog.d(DavaActivity.LOG_TAG, "Force MessageBox dismissal as onPause has come");
            synchronized(syncObject)
            {
                selection = -1;
                syncObject.notify();
            }
            dismiss();
        }
    }

    private void removeFromFragmentManager()
    {
        // Explicitly remove Fragment which represents MessageBox as Fragment
        // stays in memory even after dismissing MessageBox
        FragmentManager fragman = DavaActivity.instance().getFragmentManager();
        Fragment me = fragman.findFragmentByTag("msgbox");
        if (me != null)
        {
            fragman.beginTransaction().remove(me).commit();
        }
    }
}
