#include "Logger/Logger.h"

#if defined(__DAVAENGINE_WINDOWS__)

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)
#include <objbase.h>
#include "Utils/UTF8Utils.h"
#endif

namespace DAVA
{

#if defined(__DAVAENGINE_WIN32__)
// Windows GUI application (those linked with /SUBSYSTEM:WINDOWS flag) should be
// explicitly attached to console to allow stdout output to be visible to user when
// application started from command prompt.
// https://stackoverflow.com/a/30102947
// https://docs.microsoft.com/en-us/windows/console/attachconsole
//
// Also note when cmd.exe starts GUI application it does not wait application termination.
// User can run another program, e.g. `dir`, and its output will mess with GUI console output.
void Win32AttachStdoutToConsole(bool attach)
{
// CONSOLE and _CONSOLE definitions usually are defined when application is linked with /SUBSYSTEM:CONSOLE flag
// and stdout already attached to existing or system created console.
#if !defined(CONSOLE) && !defined(_CONSOLE)
    static FILE* reopenedStdout = nullptr;
    if (attach)
    {
        if (reopenedStdout == nullptr)
        {
            // Attach stdout to console only if it is not associated with some file or pipe
            // to allow such constructions as `app.exe | findstr blabla` or `app.exe | sort`
            if (::GetStdHandle(STD_OUTPUT_HANDLE) == nullptr)
            {
                if (::AttachConsole(ATTACH_PARENT_PROCESS) || ::GetLastError() == ERROR_ACCESS_DENIED)
                {
                    reopenedStdout = freopen("CONOUT$", "w", stdout);
                }
            }
        }
    }
    else
    {
        if (reopenedStdout != nullptr)
        {
            fclose(reopenedStdout);
        }
    }
#endif
}
#endif

void Logger::PlatformLog(eLogLevel ll, const char8* text)
{
    ::OutputDebugStringA(text);

#if defined(__DAVAENGINE_WIN_UAP__)
    {
        static Windows::Foundation::Diagnostics::LoggingChannel ^ lc = []() {
            GUID rawguid;
            // {4bd2826e-54a1-4ba9-bf63-92b73ea1ac4a} is GUID of "Microsoft-Windows-Diagnostics-LoggingChannel"
            // Details: https://docs.microsoft.com/en-us/uwp/api/windows.foundation.diagnostics.loggingchannel#remarks
            HRESULT hr = IIDFromString(L"{4bd2826e-54a1-4ba9-bf63-92b73ea1ac4a}", &rawguid);
            DVASSERT(SUCCEEDED(hr));
            return ref new Windows::Foundation::Diagnostics::LoggingChannel("DAVALogProvider", nullptr, Platform::Guid(rawguid));
        }();

        Windows::Foundation::Diagnostics::LoggingLevel lv;

        switch (ll)
        {
        case DAVA::Logger::LEVEL_FRAMEWORK:
            lv = Windows::Foundation::Diagnostics::LoggingLevel::Verbose;
            break;
        case DAVA::Logger::LEVEL_DEBUG:
            lv = Windows::Foundation::Diagnostics::LoggingLevel::Verbose;
            break;
        case DAVA::Logger::LEVEL_INFO:
            lv = Windows::Foundation::Diagnostics::LoggingLevel::Information;
            break;
        case DAVA::Logger::LEVEL_WARNING:
            lv = Windows::Foundation::Diagnostics::LoggingLevel::Warning;
            break;
        case DAVA::Logger::LEVEL_ERROR:
            lv = Windows::Foundation::Diagnostics::LoggingLevel::Error;
            break;

        case DAVA::Logger::LEVEL__DISABLE:
        default:
            return;
        }

        using UTF8Utils::eSafeEncodeError;

        eSafeEncodeError encoderError = eSafeEncodeError::NONE;
        WideString wtext = UTF8Utils::SafeEncodeToWideString(text, encoderError);
        DVASSERT(encoderError == eSafeEncodeError::NONE, "Invalid UTF8 string.");
        // Platform::StringReference should prevent an extra copy here.
        // Details: https://docs.microsoft.com/en-us/cpp/cppcx/strings-c-cx#stringreference
        if (encoderError != eSafeEncodeError::STRING_NOT_ENCODED)
        {
            lc->LogMessage(Platform::StringReference(wtext.c_str(), wtext.size()), lv);
        }
    }
#endif
}
}
#endif
