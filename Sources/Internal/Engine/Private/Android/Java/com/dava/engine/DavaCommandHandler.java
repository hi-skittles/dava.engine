package com.dava.engine;

import android.os.Handler;
import android.os.Message;

class eHandlerCommand
{
    static final int QUIT = 1;
    static final int TRIGGER_PROCESS_EVENTS = 2;
    static final int UPDATE_GAMEPADS = 3;
}

final class DavaCommandHandler extends Handler
{
    public void sendCommand(int command, Object param, long delayMs)
    {
        Message msg = obtainMessage();
        msg.what = command;
        msg.obj = param;
        sendMessageDelayed(msg, delayMs);
    }

    public void sendQuit()
    {
        sendCommand(eHandlerCommand.QUIT, null, 0);
    }

    public void sendTriggerProcessEvents(DavaSurfaceView view)
    {
        sendCommand(eHandlerCommand.TRIGGER_PROCESS_EVENTS, view, 0);
    }

    public void sendUpdateGamepads(DavaGamepadManager manager, long delayMs)
    {
        sendCommand(eHandlerCommand.UPDATE_GAMEPADS, manager, delayMs);
    }

    @Override
    public void handleMessage(Message msg)
    {
        switch (msg.what)
        {
        case eHandlerCommand.QUIT:
            if (DavaActivity.instance() != null)
            {
                DavaActivity.instance().finish();
            }
            break;
        case eHandlerCommand.TRIGGER_PROCESS_EVENTS:
            DavaSurfaceView view = (DavaSurfaceView)msg.obj;
            view.processEvents();
            break;
        case eHandlerCommand.UPDATE_GAMEPADS:
            DavaGamepadManager manager = (DavaGamepadManager)msg.obj;
            manager.timeToUpdate();
            break;
        default:
            super.handleMessage(msg);
            break;
        }
    }
}
