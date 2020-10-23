#include "Classes/CommandLine/WinConsoleIOLocker.h"

#if defined(__DAVAENGINE_WIN32__)

//code from
//http://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <io.h> 
#include <fcntl.h> 
#include <stdio.h> 
#pragma comment(lib, "User32.lib")

namespace WinConsoleIO
{
struct IOHandle
{
    struct Stream
    {
        FILE oldStream;
        FILE* newStream = nullptr;
    };

    Stream stdoutStream;
    Stream stderrStream;
};

void sendEnterKey(void)
{
    INPUT ip;
    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    //Send the "Enter" key
    ip.ki.wVk = 0x0D;
    // virtual-key code for the "Enter" key
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Enter" key
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}

bool AttachToConsole(IOHandle& handle)
{
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        //redirect unbuffered STDOUT to the console
        handle.stdoutStream.oldStream = (*stdout);

        const HANDLE consoleHandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        const int fdOut = _open_osfhandle((intptr_t)consoleHandleOut, _O_TEXT);
        handle.stdoutStream.newStream = _fdopen(fdOut, "w");
        *stdout = *handle.stdoutStream.newStream;
        setvbuf(stdout, NULL, _IONBF, 0);

        //redirect unbuffered STDERR to the console
        handle.stderrStream.oldStream = (*stderr);
        const HANDLE consoleHandleError = GetStdHandle(STD_ERROR_HANDLE);
        const int fdError = _open_osfhandle((intptr_t)consoleHandleError, _O_TEXT);
        handle.stderrStream.newStream = _fdopen(fdError, "w");
        *stderr = *handle.stderrStream.newStream;
        setvbuf(stderr, NULL, _IONBF, 0);

        printf("\n"); //initial empty string. We need it because of console attachment

        return true;
    }

    return false;
}

void DetachFromConsole(IOHandle& handle)
{
    fflush(stdout);
    *stdout = handle.stdoutStream.oldStream;
    setvbuf(stdout, NULL, _IONBF, 0);
    fclose(handle.stdoutStream.newStream);
    handle.stdoutStream.newStream = nullptr;

    fflush(stderr);
    *stderr = handle.stderrStream.oldStream;
    setvbuf(stderr, NULL, _IONBF, 0);
    fclose(handle.stderrStream.newStream);
    handle.stderrStream.newStream = nullptr;

    if (GetConsoleWindow() == GetForegroundWindow())
    { // need to close console window without user interaction
        sendEnterKey();
    }
}

} //END of WinConsoleIO

WinConsoleIOLocker::WinConsoleIOLocker()
    : ioHandle(new WinConsoleIO::IOHandle())
{
    bool attached = WinConsoleIO::AttachToConsole(*ioHandle);
    if (!attached)
    {
        ioHandle.reset();
    }
}

WinConsoleIOLocker::~WinConsoleIOLocker()
{
    if (ioHandle)
    {
        WinConsoleIO::DetachFromConsole(*ioHandle);
    }
}

#endif //#if defined(__DAVAENGINE_WIN32__)
